#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

#include <mpi.h>

#include "fractal_land.hpp"
#include "ant.hpp"
#include "pheronome.hpp"
#include "rand_generator.hpp"

struct TimingStats
{
    double sum_ants   = 0.0;
    double sum_sync   = 0.0;
    double sum_evap   = 0.0;
    double sum_update = 0.0;

    double sum2_ants   = 0.0;
    double sum2_sync   = 0.0;
    double sum2_evap   = 0.0;
    double sum2_update = 0.0;

    std::size_t count = 0;
};

static void split_range(int n, int rank, int size, int& begin, int& end)
{
    int base = n / size;
    int rem  = n % size;
    begin = rank * base + std::min(rank, rem);
    end   = begin + base + (rank < rem ? 1 : 0);
}

static double mean(double s, std::size_t n)
{
    return (n == 0) ? 0.0 : s / static_cast<double>(n);
}

static double stddev(double s, double s2, std::size_t n)
{
    if (n == 0) return 0.0;
    double m = s / static_cast<double>(n);
    return std::sqrt(s2 / static_cast<double>(n) - m * m);
}

void advance_time_mpi(const fractal_land& land,
                      pheronome& phen,
                      const position_t& pos_nest,
                      const position_t& pos_food,
                      AntColony& local_ants,
                      std::size_t& global_food_quantity,
                      int rank, int nprocs,
                      double& t_ants,
                      double& t_sync,
                      double& t_evap,
                      double& t_update)
{
    using clock = std::chrono::high_resolution_clock;

    // ============================================================
    // 1) Déplacement local des fourmis
    // ============================================================
    auto ta1 = clock::now();

    std::vector<position_t> visited_positions;
    visited_positions.reserve(local_ants.size() * 2);

    std::size_t local_food_delta = 0;

    for (std::size_t i = 0; i < local_ants.size(); ++i) {
        local_ants.advance(i, phen, land, pos_food, pos_nest,
                            local_food_delta, visited_positions);
    }

    for (const auto& pos : visited_positions) {
        phen.mark_pheronome(pos);
    }

    auto ta2 = clock::now();
    double local_t_ants = std::chrono::duration<double, std::milli>(ta2 - ta1).count();

    // ============================================================
    // 2) Synchronisation MPI : max des phéromones + somme nourriture
    // ============================================================
    auto ts1 = clock::now();

    std::vector<double> reduced_buffer(phen.raw_buffer_count(), 0.0);

    MPI_Allreduce(
        phen.raw_buffer_data(),
        reduced_buffer.data(),
        static_cast<int>(phen.raw_buffer_count()),
        MPI_DOUBLE,
        MPI_MAX,
        MPI_COMM_WORLD
    );

    std::copy(reduced_buffer.begin(), reduced_buffer.end(), phen.raw_buffer_data());

    unsigned long long local_food_ull  = static_cast<unsigned long long>(local_food_delta);
    unsigned long long global_food_ull = 0ULL;

    MPI_Allreduce(
        &local_food_ull,
        &global_food_ull,
        1,
        MPI_UNSIGNED_LONG_LONG,
        MPI_SUM,
        MPI_COMM_WORLD
    );

    global_food_quantity += static_cast<std::size_t>(global_food_ull);

    auto ts2 = clock::now();
    double local_t_sync = std::chrono::duration<double, std::milli>(ts2 - ts1).count();

    // ============================================================
    // 3) Évaporation parallèle par blocs de lignes
    // ============================================================
    auto te1 = clock::now();

    int row_begin = 0, row_end = 0;
    split_range(static_cast<int>(phen.dim()), rank, nprocs, row_begin, row_end);

    phen.evaporate_rows(static_cast<std::size_t>(row_begin),
                        static_cast<std::size_t>(row_end));

    std::size_t stride = phen.stride();
    int sendcount = (row_end - row_begin) * static_cast<int>(stride) * 2;
    int senddisp  = (row_begin + 1) * static_cast<int>(stride) * 2;

    std::vector<int> recvcounts(nprocs), displs(nprocs);
    for (int p = 0; p < nprocs; ++p) {
        int b = 0, e = 0;
        split_range(static_cast<int>(phen.dim()), p, nprocs, b, e);
        recvcounts[p] = (e - b) * static_cast<int>(stride) * 2;
        displs[p]     = (b + 1) * static_cast<int>(stride) * 2;
    }

    std::vector<double> gathered(phen.raw_buffer_count(), 0.0);
    std::copy(phen.raw_buffer_data(),
              phen.raw_buffer_data() + phen.raw_buffer_count(),
              gathered.begin());

    MPI_Allgatherv(
        phen.raw_buffer_data() + senddisp,
        sendcount,
        MPI_DOUBLE,
        gathered.data(),
        recvcounts.data(),
        displs.data(),
        MPI_DOUBLE,
        MPI_COMM_WORLD
    );

    std::copy(gathered.begin(), gathered.end(), phen.raw_buffer_data());

    auto te2 = clock::now();
    double local_t_evap = std::chrono::duration<double, std::milli>(te2 - te1).count();

    // ============================================================
    // 4) Update local
    // ============================================================
    auto tu1 = clock::now();
    phen.update();
    auto tu2 = clock::now();
    double local_t_update = std::chrono::duration<double, std::milli>(tu2 - tu1).count();

    // ============================================================
    // 5) Le temps parallèle d'une phase = max sur les processus
    // ============================================================
    MPI_Allreduce(&local_t_ants,   &t_ants,   1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&local_t_sync,   &t_sync,   1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&local_t_evap,   &t_evap,   1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(&local_t_update, &t_update, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
}

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int rank = 0, nprocs = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    std::size_t seed = 2026;
    const int nb_ants = 5000;
    const int max_iter = 5000;
    const double eps = 0.8;
    const double alpha = 0.7;
    const double beta = 0.999;

    position_t pos_nest{256,256};
    position_t pos_food{500,500};

    // ============================================================
    // Génération identique de l'environnement sur tous les processus
    // ============================================================
    fractal_land land(8, 2, 1., 1024);

    double max_val = 0.0;
    double min_val = 0.0;
    for (fractal_land::dim_t i = 0; i < land.dimensions(); ++i) {
        for (fractal_land::dim_t j = 0; j < land.dimensions(); ++j) {
            max_val = std::max(max_val, land(i,j));
            min_val = std::min(min_val, land(i,j));
        }
    }

    double delta = max_val - min_val;
    for (fractal_land::dim_t i = 0; i < land.dimensions(); ++i) {
        for (fractal_land::dim_t j = 0; j < land.dimensions(); ++j) {
            land(i,j) = (land(i,j) - min_val) / delta;
        }
    }

    pheronome phen(land.dimensions(), pos_food, pos_nest, alpha, beta);

    // ============================================================
    // Répartition des fourmis : chaque processus garde une partie
    // ============================================================
    AntColony local_ants;
    AntColony::set_exploration_coef(eps);

    int ant_begin = 0, ant_end = 0;
    split_range(nb_ants, rank, nprocs, ant_begin, ant_end);
    local_ants.reserve(static_cast<std::size_t>(ant_end - ant_begin));

    auto gen_ant_pos = [&land, &seed]() {
        return rand_int32(0, land.dimensions() - 1, seed);
    };

    for (int i = 0; i < nb_ants; ++i) {
        position_t p{gen_ant_pos(), gen_ant_pos()};
        if (i >= ant_begin && i < ant_end) {
            local_ants.push_back(p, seed, 0);
        }
    }

    std::size_t food_quantity = 0;
    bool first_food_found = false;
    int first_food_iter = -1;

    std::ofstream csv;
    if (rank == 0) {
        csv.open("stats_fourmis_mpi.csv");
        csv << "iteration,temps_fourmis_ms,temps_sync_ms,temps_evap_ms,temps_update_ms,food_quantity\n";
    }

    TimingStats stats;

    for (std::size_t it = 1; it <= static_cast<std::size_t>(max_iter); ++it) {
        double t_ants = 0.0, t_sync = 0.0, t_evap = 0.0, t_update = 0.0;

        advance_time_mpi(land, phen, pos_nest, pos_food, local_ants,
                         food_quantity, rank, nprocs,
                         t_ants, t_sync, t_evap, t_update);

        if (rank == 0) {
            csv << it << ","
                << std::fixed << std::setprecision(6)
                << t_ants << ","
                << t_sync << ","
                << t_evap << ","
                << t_update << ","
                << food_quantity << "\n";
        }

        stats.sum_ants   += t_ants;
        stats.sum_sync   += t_sync;
        stats.sum_evap   += t_evap;
        stats.sum_update += t_update;

        stats.sum2_ants   += t_ants * t_ants;
        stats.sum2_sync   += t_sync * t_sync;
        stats.sum2_evap   += t_evap * t_evap;
        stats.sum2_update += t_update * t_update;

        stats.count++;

        if (!first_food_found && food_quantity > 0) {
            first_food_found = true;
            first_food_iter = static_cast<int>(it);
        }
    }

    if (rank == 0) {
        csv.close();
    } 
    MPI_Finalize();
    return 0;
}
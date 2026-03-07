#include <vector>
#include <iostream>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <cstdlib>

#include <mpi.h>

#ifdef _OPENMP
#include <omp.h>
#endif

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

void advance_time_hybrid(const fractal_land& land,
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
    // 1) Déplacement local des fourmis : OpenMP dans chaque processus
    // ============================================================
    auto ta1 = clock::now();

#ifdef _OPENMP
    int nth = omp_get_max_threads();
#else
    int nth = 1;
#endif

    std::vector<std::vector<position_t>> visited_by_thread(nth);
    std::size_t local_food_delta = 0;

#ifdef _OPENMP
#pragma omp parallel reduction(+:local_food_delta)
#endif
    {
#ifdef _OPENMP
        int tid = omp_get_thread_num();
#else
        int tid = 0;
#endif
        auto& visited = visited_by_thread[tid];
        visited.clear();
        visited.reserve(1024);

#ifdef _OPENMP
#pragma omp for schedule(static)
#endif
        for (std::size_t i = 0; i < local_ants.size(); ++i) {
            advance_ant_collect(i, local_ants, phen, land, pos_food, pos_nest,
                                local_food_delta, visited);
        }
    }

    // application locale séquentielle des marquages
    for (const auto& local_vec : visited_by_thread) {
        for (const auto& pos : local_vec) {
            phen.mark_pheronome(pos);
        }
    }

    auto ta2 = clock::now();
    double local_t_ants = std::chrono::duration<double, std::milli>(ta2 - ta1).count();

    // ============================================================
    // 2) Synchronisation MPI : fusion des phéromones + nourriture
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
    // 3) Évaporation hybride : chaque processus traite ses lignes,
    //    et OpenMP parallélise ces lignes localement
    // ============================================================
    auto te1 = clock::now();

    int row_begin = 0, row_end = 0;
    split_range(static_cast<int>(phen.dim()), rank, nprocs, row_begin, row_end);

    phen.evaporate_rows_parallel(static_cast<std::size_t>(row_begin),
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

    // temps parallèles = max sur les processus
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

    int threads_per_proc = 1;
    if (argc >= 2) {
        threads_per_proc = std::atoi(argv[1]);
        if (threads_per_proc <= 0) threads_per_proc = 1;
    }

#ifdef _OPENMP
    omp_set_num_threads(threads_per_proc);
#endif

    position_t pos_nest{256,256};
    position_t pos_food{500,500};

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
        csv.open("stats_fourmis_hybrid.csv");
        csv << "iteration,temps_fourmis_ms,temps_sync_ms,temps_evap_ms,temps_update_ms,food_quantity\n";
    }

    TimingStats stats;

    for (std::size_t it = 1; it <= static_cast<std::size_t>(max_iter); ++it) {
        double t_ants = 0.0, t_sync = 0.0, t_evap = 0.0, t_update = 0.0;

        advance_time_hybrid(land, phen, pos_nest, pos_food, local_ants,
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

        double mean_ants   = mean(stats.sum_ants, stats.count);
        double mean_sync   = mean(stats.sum_sync, stats.count);
        double mean_evap   = mean(stats.sum_evap, stats.count);
        double mean_update = mean(stats.sum_update, stats.count);

        double std_ants   = stddev(stats.sum_ants, stats.sum2_ants, stats.count);
        double std_sync   = stddev(stats.sum_sync, stats.sum2_sync, stats.count);
        double std_evap   = stddev(stats.sum_evap, stats.sum2_evap, stats.count);
        double std_update = stddev(stats.sum_update, stats.sum2_update, stats.count);

        double total_all = stats.sum_ants + stats.sum_sync + stats.sum_evap + stats.sum_update;

        std::cout << "La première nourriture est arrivée au nid à l'itération "
                  << first_food_iter << "\n\n";

        std::cout << "===== RÉSULTATS VERSION HYBRIDE MPI + OpenMP =====\n";
        std::cout << "Processus                    : " << nprocs << "\n";
        std::cout << "Threads / processus          : " << threads_per_proc << "\n";
        std::cout << "Nb itérations                : " << stats.count << "\n";
        std::cout << "Quantité finale nourriture   : " << food_quantity << "\n\n";

        std::cout << "--- temps_fourmis_ms ---\n";
        std::cout << "Temps total    : " << stats.sum_ants << " ms\n";
        std::cout << "Temps moyen    : " << mean_ants << " ms\n";
        std::cout << "Écart-type     : " << std_ants << " ms\n";
        std::cout << "Pourcentage    : " << (100.0 * stats.sum_ants / total_all) << " %\n\n";

        std::cout << "--- temps_sync_ms ---\n";
        std::cout << "Temps total    : " << stats.sum_sync << " ms\n";
        std::cout << "Temps moyen    : " << mean_sync << " ms\n";
        std::cout << "Écart-type     : " << std_sync << " ms\n";
        std::cout << "Pourcentage    : " << (100.0 * stats.sum_sync / total_all) << " %\n\n";

        std::cout << "--- temps_evap_ms ---\n";
        std::cout << "Temps total    : " << stats.sum_evap << " ms\n";
        std::cout << "Temps moyen    : " << mean_evap << " ms\n";
        std::cout << "Écart-type     : " << std_evap << " ms\n";
        std::cout << "Pourcentage    : " << (100.0 * stats.sum_evap / total_all) << " %\n\n";

        std::cout << "--- temps_update_ms ---\n";
        std::cout << "Temps total    : " << stats.sum_update << " ms\n";
        std::cout << "Temps moyen    : " << mean_update << " ms\n";
        std::cout << "Écart-type     : " << std_update << " ms\n";
        std::cout << "Pourcentage    : " << (100.0 * stats.sum_update / total_all) << " %\n\n";

        std::cout << "===== GLOBAL =====\n";
        std::cout << "Temps total des colonnes analysées : " << total_all << " ms\n";
    }

    MPI_Finalize();
    return 0;
}
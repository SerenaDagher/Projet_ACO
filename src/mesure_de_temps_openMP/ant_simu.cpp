#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <cstdlib>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "fractal_land.hpp"
#include "ant.hpp"
#include "pheronome.hpp"
#include "rand_generator.hpp"

void advance_time_openmp(const fractal_land& land,
                         pheronome& phen,
                         const position_t& pos_nest,
                         const position_t& pos_food,
                         AntColony& ants,
                         std::size_t& cpteur,
                         double& t_ants,
                         double& t_evap,
                         double& t_update)
{
    using clock = std::chrono::high_resolution_clock;

    auto t1 = clock::now();

#ifdef _OPENMP
    int nthreads = omp_get_max_threads();
#else
    int nthreads = 1;
#endif

    std::vector<std::vector<position_t>> visited_by_thread(nthreads);
    std::size_t delta_food = 0;

#ifdef _OPENMP
#pragma omp parallel reduction(+:delta_food)
#endif
    {
#ifdef _OPENMP
        int tid = omp_get_thread_num();
#else
        int tid = 0;
#endif
        auto& visited = visited_by_thread[tid];
        visited.clear();
        visited.reserve(2 * 5000 / nthreads);

#ifdef _OPENMP
#pragma omp for schedule(static)
#endif
        for (std::size_t i = 0; i < ants.size(); ++i) {
            ants.advance(i, phen, land, pos_food, pos_nest, delta_food, visited);
        }
    }

    for (const auto& local_vec : visited_by_thread) {
        for (const auto& pos : local_vec) {
            phen.mark_pheronome(pos);
        }
    }

    cpteur += delta_food;

    auto t2 = clock::now();

    phen.do_evaporation();
    auto t3 = clock::now();

    phen.update();
    auto t4 = clock::now();

    t_ants   = std::chrono::duration<double, std::milli>(t2 - t1).count();
    t_evap   = std::chrono::duration<double, std::milli>(t3 - t2).count();
    t_update = std::chrono::duration<double, std::milli>(t4 - t3).count();
}

int main(int argc, char* argv[])
{
    std::size_t seed = 2026;
    const int nb_ants = 5000;
    const int max_iter = 5000;
    const double eps = 0.8;
    const double alpha = 0.7;
    const double beta = 0.999;

    int nb_threads = 1;
    if (argc >= 2) {
        nb_threads = std::atoi(argv[1]);
        if (nb_threads <= 0) nb_threads = 1;
    }

#ifdef _OPENMP
    omp_set_num_threads(nb_threads);
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

    AntColony ants;
    ants.reserve(nb_ants);
    AntColony::set_exploration_coef(eps);

    auto gen_ant_pos = [&land, &seed]() {
        return rand_int32(0, land.dimensions() - 1, seed);
    };

    for (std::size_t i = 0; i < nb_ants; ++i) {
        ants.push_back(position_t{gen_ant_pos(), gen_ant_pos()}, seed, 0);
    }

    pheronome phen(land.dimensions(), pos_food, pos_nest, alpha, beta);

    std::size_t food_quantity = 0;

    std::ofstream csv("stats_fourmis.csv");
    csv << "iteration,temps_fourmis_ms,temps_evap_ms,temps_update_ms,food_quantity\n";

    for (std::size_t it = 1; it <= (std::size_t)max_iter; ++it) {
        double t_ants = 0.0, t_evap = 0.0, t_update = 0.0;

        advance_time_openmp(land, phen, pos_nest, pos_food,
                            ants, food_quantity,
                            t_ants, t_evap, t_update);

        csv << it << ","
            << std::fixed << std::setprecision(6)
            << t_ants << ","
            << t_evap << ","
            << t_update << ","
            << food_quantity << "\n";
    }

    csv.close();

    return 0;
}
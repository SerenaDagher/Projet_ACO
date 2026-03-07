#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <cmath>

#include "fractal_land.hpp"
#include "ant.hpp"
#include "pheronome.hpp"
#include "rand_generator.hpp"

struct TimingStats
{
    double sum_ants   = 0.0;
    double sum_evap   = 0.0;
    double sum_update = 0.0;

    double sum2_ants   = 0.0;
    double sum2_evap   = 0.0;
    double sum2_update = 0.0;

    std::size_t count = 0;
};

void advance_time_vectorized(const fractal_land& land,
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
    for (std::size_t i = 0; i < ants.size(); ++i) {
        advance_ant(i, ants, phen, land, pos_food, pos_nest, cpteur);
    }
    auto t2 = clock::now();

    phen.do_evaporation();
    auto t3 = clock::now();

    phen.update();
    auto t4 = clock::now();

    t_ants   = std::chrono::duration<double, std::milli>(t2 - t1).count();
    t_evap   = std::chrono::duration<double, std::milli>(t3 - t2).count();
    t_update = std::chrono::duration<double, std::milli>(t4 - t3).count();
}

int main(int nargs, char* argv[])
{
    std::size_t seed = 2026;
    const int nb_ants = 5000;
    const int max_iter = 5000;
    const double eps = 0.8;
    const double alpha = 0.7;
    const double beta = 0.999;

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
    bool not_food_in_nest = true;

    std::ofstream csv("stats_fourmis.csv");
    csv << "iteration,temps_fourmis_ms,temps_evap_ms,temps_update_ms,food_quantity\n";

    TimingStats stats;

    for (std::size_t it = 1; it <= (std::size_t)max_iter; ++it) {
        double t_ants = 0.0;
        double t_evap = 0.0;
        double t_update = 0.0;

        advance_time_vectorized(land, phen, pos_nest, pos_food,
                                ants, food_quantity,
                                t_ants, t_evap, t_update);

        csv << it << ","
            << std::fixed << std::setprecision(6)
            << t_ants << ","
            << t_evap << ","
            << t_update << ","
            << food_quantity << "\n";

        stats.sum_ants   += t_ants;
        stats.sum_evap   += t_evap;
        stats.sum_update += t_update;

        stats.sum2_ants   += t_ants * t_ants;
        stats.sum2_evap   += t_evap * t_evap;
        stats.sum2_update += t_update * t_update;

        stats.count++;

        if (not_food_in_nest && food_quantity > 0) {
            std::cout << "La première nourriture est arrivée au nid à l'itération "
                      << it << std::endl;
            not_food_in_nest = false;
        }
    }

    csv.close();

    auto mean = [](double s, std::size_t n) {
        return (n == 0) ? 0.0 : s / static_cast<double>(n);
    };

    auto stddev = [](double s, double s2, std::size_t n) {
        if (n == 0) return 0.0;
        double m = s / static_cast<double>(n);
        return std::sqrt(s2 / static_cast<double>(n) - m * m);
    };

    double mean_ants   = mean(stats.sum_ants, stats.count);
    double mean_evap   = mean(stats.sum_evap, stats.count);
    double mean_update = mean(stats.sum_update, stats.count);

    double std_ants   = stddev(stats.sum_ants, stats.sum2_ants, stats.count);
    double std_evap   = stddev(stats.sum_evap, stats.sum2_evap, stats.count);
    double std_update = stddev(stats.sum_update, stats.sum2_update, stats.count);

    double total_all = stats.sum_ants + stats.sum_evap + stats.sum_update;

    std::cout << "\n===== RÉSULTATS VERSION VECTORISÉE =====\n";
    std::cout << "Nb itérations                : " << stats.count << "\n";
    std::cout << "Quantité finale nourriture   : " << food_quantity << "\n\n";

    std::cout << "--- temps_fourmis_ms ---\n";
    std::cout << "Temps total    : " << stats.sum_ants << " ms\n";
    std::cout << "Temps moyen    : " << mean_ants << " ms\n";
    std::cout << "Écart-type     : " << std_ants << " ms\n";
    std::cout << "Pourcentage    : " << (100.0 * stats.sum_ants / total_all) << " %\n\n";

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

    return 0;
}
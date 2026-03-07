#include <vector>
#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <iomanip>
#include <omp.h>
#include "fractal_land.hpp"
#include "ant.hpp"
#include "pheronome.hpp"
#include "renderer.hpp"
#include "window.hpp"
#include "rand_generator.hpp"

struct TimingStats {
    double ant_move_time = 0.0;
    double pheromone_evaporation_time = 0.0;
    double pheromone_update_time = 0.0;
    double rendering_time = 0.0;
    
    size_t food_collected = 0;
    size_t first_food_iteration = 0;
    size_t total_iterations = 0;
    double total_wall_time = 0.0;
    
    int nb_ants = 0;
    int grid_size = 0;
    int nb_threads = 1;
    std::string mode = "vectorise_omp";
    
    void print() const {
        double total_compute = ant_move_time + pheromone_evaporation_time + pheromone_update_time;
        double overhead = total_wall_time - total_compute - rendering_time;
        
        std::cout << "\n============================================================" << std::endl;
        std::cout << "     STATISTIQUES DE PERFORMANCE - MODE VECTORISE + OMP" << std::endl;
        std::cout << "============================================================" << std::endl;
        std::cout << std::fixed << std::setprecision(6);
        
        std::cout << "  CONFIGURATION:" << std::endl;
        std::cout << "    Nombre de fourmis:         " << std::setw(10) << nb_ants << std::endl;
        std::cout << "    Taille de la grille:       " << grid_size << "x" << grid_size << std::endl;
        std::cout << "    Threads OpenMP:            " << std::setw(10) << nb_threads << std::endl;
        std::cout << "    Iterations totales:        " << std::setw(10) << total_iterations << std::endl;
        std::cout << "------------------------------------------------------------" << std::endl;
        
        std::cout << "  RESULTATS DE SIMULATION:" << std::endl;
        std::cout << "    Nourriture collectee:      " << std::setw(10) << food_collected << std::endl;
        std::cout << "    Premiere nourriture (iter):" << std::setw(10) << first_food_iteration << std::endl;
        std::cout << "------------------------------------------------------------" << std::endl;
        
        std::cout << "  TEMPS DE CALCUL (en secondes):" << std::endl;
        std::cout << "    1. Deplacement fourmis:    " << std::setw(10) << ant_move_time << " s  (" 
                  << std::setw(5) << std::setprecision(1) << (ant_move_time/total_compute*100) << "%)" << std::endl;
        std::cout << std::setprecision(6);
        std::cout << "    2. Evaporation pheromones: " << std::setw(10) << pheromone_evaporation_time << " s  (" 
                  << std::setw(5) << std::setprecision(1) << (pheromone_evaporation_time/total_compute*100) << "%)" << std::endl;
        std::cout << std::setprecision(6);
        std::cout << "    3. MAJ pheromones:         " << std::setw(10) << pheromone_update_time << " s  (" 
                  << std::setw(5) << std::setprecision(1) << (pheromone_update_time/total_compute*100) << "%)" << std::endl;
        std::cout << std::setprecision(6);
        std::cout << "    ---------------------------------------------------" << std::endl;
        std::cout << "    TOTAL CALCUL:              " << std::setw(10) << total_compute << " s  (100.0%)" << std::endl;
        std::cout << "" << std::endl;
        std::cout << "    Rendu graphique:           " << std::setw(10) << rendering_time << " s" << std::endl;
        std::cout << "    Overhead/autre:            " << std::setw(10) << overhead << " s" << std::endl;
        std::cout << "    ---------------------------------------------------" << std::endl;
        std::cout << "    TEMPS TOTAL (wall time):   " << std::setw(10) << total_wall_time << " s" << std::endl;
        std::cout << "------------------------------------------------------------" << std::endl;
        
        std::cout << "  METRIQUES DE PERFORMANCE:" << std::endl;
        std::cout << std::setprecision(3);
        std::cout << "    Temps moyen/iteration:     " << std::setw(10) << (total_compute/total_iterations*1000) << " ms" << std::endl;
        std::cout << std::setprecision(2);
        std::cout << "    Iterations/seconde:        " << std::setw(10) << (total_iterations/total_compute) << " iter/s" << std::endl;
        std::cout << std::setprecision(6);
        std::cout << "    Debit (fourmis*iter/s):    " << std::setw(10) << (nb_ants*total_iterations/total_compute) << std::endl;
        std::cout << "    Nourriture/seconde:        " << std::setw(10) << std::setprecision(3) << (food_collected/total_compute) << " unites/s" << std::endl;
        std::cout << "============================================================\n" << std::endl;
    }
    
    void export_csv(const std::string& filename) const {
        std::ofstream file(filename, std::ios::app);
        if (!file.is_open()) {
            std::cerr << "Erreur: impossible d'ouvrir " << filename << std::endl;
            return;
        }
        
        file.seekp(0, std::ios::end);
        if (file.tellp() == 0) {
            file << "mode,nb_fourmis,nb_threads,taille_grille,iterations,nourriture_collectee,premiere_nourriture_iter,"
                 << "temps_fourmis,temps_evaporation,temps_maj_pheromone,temps_total_calcul,"
                 << "temps_rendu,temps_wall,temps_moyen_iter_ms,iter_par_sec,debit_fourmis_iter\n";
        }
        
        double total_compute = ant_move_time + pheromone_evaporation_time + pheromone_update_time;
        
        file << std::fixed << std::setprecision(6);
        file << mode << ","
             << nb_ants << ","
             << nb_threads << ","
             << grid_size << ","
             << total_iterations << ","
             << food_collected << ","
             << first_food_iteration << ","
             << ant_move_time << ","
             << pheromone_evaporation_time << ","
             << pheromone_update_time << ","
             << total_compute << ","
             << rendering_time << ","
             << total_wall_time << ","
             << (total_compute/total_iterations*1000) << ","
             << (total_iterations/total_compute) << ","
             << (nb_ants*total_iterations/total_compute) << "\n";
        
        file.close();
        std::cout << "+ Statistiques exportees dans: " << filename << std::endl;
    }
};

void advance_time( const fractal_land& land, pheronome& phen,
                   const position_t& pos_nest, const position_t& pos_food,
                   AntColony& colony, std::size_t& cpteur, TimingStats& stats )
{
    auto t1 = std::chrono::high_resolution_clock::now();
    std::size_t local_cpteur = 0;
    #pragma omp parallel for reduction(+:local_cpteur) schedule(dynamic, 64)
    for ( std::size_t i = 0; i < colony.size(); ++i )
        advance_ant( i, colony, phen, land, pos_food, pos_nest, local_cpteur );
    cpteur += local_cpteur;
    auto t2 = std::chrono::high_resolution_clock::now();

    phen.do_evaporation();
    auto t3 = std::chrono::high_resolution_clock::now();

    phen.update();
    auto t4 = std::chrono::high_resolution_clock::now();

    stats.ant_move_time              += std::chrono::duration<double>(t2-t1).count();
    stats.pheromone_evaporation_time += std::chrono::duration<double>(t3-t2).count();
    stats.pheromone_update_time      += std::chrono::duration<double>(t4-t3).count();
}

int main(int nargs, char* argv[])
{
    bool benchmark_mode = false;
    int max_iterations = -1;
    int render_freq = 10;
    
    for (int i = 1; i < nargs; ++i) {
        std::string arg = argv[i];
        if (arg == "--benchmark") {
            benchmark_mode = true;
            max_iterations = 5000;
        } else if (arg == "--iterations" && i+1 < nargs) {
            max_iterations = std::atoi(argv[++i]);
            benchmark_mode = true;
        } else if (arg == "--render-freq" && i+1 < nargs) {
            render_freq = std::atoi(argv[++i]);
            if (render_freq < 1) render_freq = 1;
        } else if (arg == "--threads" && i+1 < nargs) {
            omp_set_num_threads(std::atoi(argv[++i]));
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --benchmark           Mode benchmark (pas d'affichage, 5000 iterations)\n";
            std::cout << "  --iterations <n>      Executer n iterations puis quitter\n";
            std::cout << "  --render-freq <n>     Afficher 1 frame sur n (defaut: 10)\n";
            std::cout << "  --threads <n>         Nombre de threads OpenMP (defaut: max disponible)\n";
            std::cout << "  --help                Afficher ce message\n";
            return 0;
        }
    }

    std::size_t seed = 2026;
    const int nb_ants = 5000;
    const double eps = 0.8;
    const double alpha = 0.7;
    const double beta = 0.999;
    position_t pos_nest{256,256};
    position_t pos_food{500,500};
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Nombre de fourmis: " << nb_ants << std::endl;
    std::cout << "  Taille de la carte: " << 512 << "x" << 512 << std::endl;
    std::cout << "  Representation: Vectorisee (SoA) + OpenMP" << std::endl;
    std::cout << "  Threads OpenMP: " << omp_get_max_threads() << std::endl;
    std::cout << "  Mode: " << (benchmark_mode ? "Benchmark" : "Interactif") << std::endl;
    if (max_iterations > 0)
        std::cout << "  Iterations max: " << max_iterations << std::endl;
    std::cout << std::endl;
    
    fractal_land land(8,2,1.,1024);
    double max_val = 0.0;
    double min_val = 0.0;
    for ( fractal_land::dim_t i = 0; i < land.dimensions(); ++i )
        for ( fractal_land::dim_t j = 0; j < land.dimensions(); ++j ) {
            max_val = std::max(max_val, land(i,j));
            min_val = std::min(min_val, land(i,j));
        }
    double delta = max_val - min_val;
    for ( fractal_land::dim_t i = 0; i < land.dimensions(); ++i )
        for ( fractal_land::dim_t j = 0; j < land.dimensions(); ++j )
            land(i,j) = (land(i,j)-min_val)/delta;

    AntColony::set_exploration_coef(eps);

    AntColony colony;
    colony.x.resize(nb_ants);
    colony.y.resize(nb_ants);
    colony.states.resize(nb_ants, 0);
    colony.seeds.resize(nb_ants);

    std::size_t init_seed = seed;
    auto gen_ant_pos = [&land, &init_seed]() { return rand_int32(0, land.dimensions()-1, init_seed); };
    for ( std::size_t i = 0; i < (std::size_t)nb_ants; ++i ) {
        colony.x[i]     = gen_ant_pos();
        colony.y[i]     = gen_ant_pos();
        colony.seeds[i] = init_seed + i;
    }

    pheronome phen(land.dimensions(), pos_food, pos_nest, alpha, beta);

    if (!benchmark_mode) {
        std::cout << "Note: le rendu graphique n'est pas disponible en mode vectorise." << std::endl;
        std::cout << "      Utiliser ant_simu pour la visualisation." << std::endl;
        std::cout << "      Passage automatique en mode benchmark." << std::endl;
        benchmark_mode = true;
        if (max_iterations < 0) max_iterations = 5000;
    }

    size_t food_quantity = 0;
    bool cont_loop = true;
    bool not_food_in_nest = true;
    std::size_t it = 0;
    TimingStats stats;
    
    std::cout << "Demarrage de la simulation..." << std::endl;
    auto sim_start = std::chrono::high_resolution_clock::now();
    
    while (cont_loop) {
        ++it;

        advance_time( land, phen, pos_nest, pos_food, colony, food_quantity, stats );

        if ( not_food_in_nest && food_quantity > 0 ) {
            std::cout << "+ La premiere nourriture est arrivee au nid a l'iteration " << it << std::endl;
            stats.first_food_iteration = it;
            not_food_in_nest = false;
        }

        if (max_iterations > 0 && static_cast<int>(it) >= max_iterations) {
            std::cout << "Nombre maximum d'iterations atteint (" << max_iterations << ")" << std::endl;
            cont_loop = false;
        }
    }
    
    auto sim_end = std::chrono::high_resolution_clock::now();
    double total_time = std::chrono::duration<double>(sim_end - sim_start).count();
    
    stats.total_wall_time      = total_time;
    stats.total_iterations     = it;
    stats.food_collected       = food_quantity;
    stats.first_food_iteration = (not_food_in_nest ? 0 : stats.first_food_iteration);
    stats.nb_ants              = nb_ants;
    stats.grid_size            = land.dimensions();
    stats.nb_threads           = omp_get_max_threads();
    stats.mode                 = "vectorise_omp";
    
    stats.print();
    stats.export_csv("stats_fourmis.csv");

    return 0;
}
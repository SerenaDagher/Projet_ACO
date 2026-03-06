#include <vector>
#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <iomanip>
#include "fractal_land.hpp"
#include "ant.hpp"
#include "pheronome.hpp"
# include "renderer.hpp"
# include "window.hpp"
# include "rand_generator.hpp"

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
    std::string mode = "sequentiel";
    
    void print() const {
        double total_compute = ant_move_time + pheromone_evaporation_time + pheromone_update_time;
        double overhead = total_wall_time - total_compute - rendering_time;
        
        std::cout << "\n============================================================" << std::endl;
        std::cout << "     STATISTIQUES DE PERFORMANCE - MODE " << std::left << std::setw(13) << (mode == "sequentiel" ? "SEQUENTIEL" : "PARALLELE") << std::endl;
        std::cout << "============================================================" << std::endl;
        std::cout << std::fixed << std::setprecision(6);
        
        // Configuration
        std::cout << "  CONFIGURATION:" << std::endl;
        std::cout << "    Nombre de fourmis:         " << std::setw(10) << nb_ants << std::endl;
        std::cout << "    Taille de la grille:       " << grid_size << "x" << grid_size << std::endl;
        std::cout << "    Iterations totales:        " << std::setw(10) << total_iterations << std::endl;
        std::cout << "------------------------------------------------------------" << std::endl;
        
        // Resultats de simulation
        std::cout << "  RESULTATS DE SIMULATION:" << std::endl;
        std::cout << "    Nourriture collectee:      " << std::setw(10) << food_collected << std::endl;
        std::cout << "    Premiere nourriture (iter):" << std::setw(10) << first_food_iteration << std::endl;
        std::cout << "------------------------------------------------------------" << std::endl;
        
        // Temps de calcul detailles
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
        
        // Metriques de performance
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
        
        // Verifier si le fichier est vide pour ecrire l'en-tete
        file.seekp(0, std::ios::end);
        if (file.tellp() == 0) {
            file << "mode,nb_fourmis,taille_grille,iterations,nourriture_collectee,premiere_nourriture_iter,"
                 << "temps_fourmis,temps_evaporation,temps_maj_pheromone,temps_total_calcul,"
                 << "temps_rendu,temps_wall,temps_moyen_iter_ms,iter_par_sec,debit_fourmis_iter\n";
        }
        
        double total_compute = ant_move_time + pheromone_evaporation_time + pheromone_update_time;
        
        file << std::fixed << std::setprecision(6);
        file << mode << ","
             << nb_ants << ","
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
                   std::vector<ant>& ants, std::size_t& cpteur, TimingStats& stats )
{
    auto t1 = std::chrono::high_resolution_clock::now();
    for ( size_t i = 0; i < ants.size(); ++i )
        ants[i].advance(phen, land, pos_food, pos_nest, cpteur);
    auto t2 = std::chrono::high_resolution_clock::now();
    
    phen.do_evaporation();
    auto t3 = std::chrono::high_resolution_clock::now();
    
    phen.update();
    auto t4 = std::chrono::high_resolution_clock::now();
    
    stats.ant_move_time += std::chrono::duration<double>(t2-t1).count();
    stats.pheromone_evaporation_time += std::chrono::duration<double>(t3-t2).count();
    stats.pheromone_update_time += std::chrono::duration<double>(t4-t3).count();
}

int main(int nargs, char* argv[])
{
    SDL_Init( SDL_INIT_VIDEO );
    std::size_t seed = 2026; // Graine pour la génération aléatoire ( reproductible )
    const int nb_ants = 5000; // Nombre de fourmis
    const double eps = 0.8;  // Coefficient d'exploration
    const double alpha=0.7; // Coefficient de chaos
    //const double beta=0.9999; // Coefficient d'évaporation
    const double beta=0.999; // Coefficient d'évaporation
    // Location du nid
    position_t pos_nest{256,256};
    // Location de la nourriture
    position_t pos_food{500,500};
    //const int i_food = 500, j_food = 500;    
    // Génération du territoire 512 x 512 ( 2*(2^8) par direction )
    fractal_land land(8,2,1.,1024);
    double max_val = 0.0;
    double min_val = 0.0;
    for ( fractal_land::dim_t i = 0; i < land.dimensions(); ++i )
        for ( fractal_land::dim_t j = 0; j < land.dimensions(); ++j ) {
            max_val = std::max(max_val, land(i,j));
            min_val = std::min(min_val, land(i,j));
        }
    double delta = max_val - min_val;
    /* On redimensionne les valeurs de fractal_land de sorte que les valeurs
    soient comprises entre zéro et un */
    for ( fractal_land::dim_t i = 0; i < land.dimensions(); ++i )
        for ( fractal_land::dim_t j = 0; j < land.dimensions(); ++j )  {
            land(i,j) = (land(i,j)-min_val)/delta;
        }
    // Définition du coefficient d'exploration de toutes les fourmis.
    ant::set_exploration_coef(eps);
    // On va créer des fourmis un peu partout sur la carte :
    std::vector<ant> ants;
    ants.reserve(nb_ants);
    auto gen_ant_pos = [&land, &seed] () { return rand_int32(0, land.dimensions()-1, seed); };
    for ( size_t i = 0; i < nb_ants; ++i )
        ants.emplace_back(position_t{gen_ant_pos(),gen_ant_pos()}, seed);
    // On crée toutes les fourmis dans la fourmilière.
    pheronome phen(land.dimensions(), pos_food, pos_nest, alpha, beta);

    Window win("Ant Simulation", 2*land.dimensions()+10, land.dimensions()+266);
    Renderer renderer( land, phen, pos_nest, pos_food, ants );
    // Compteur de la quantité de nourriture apportée au nid par les fourmis
    size_t food_quantity = 0;
    SDL_Event event;
    bool cont_loop = true;
    bool not_food_in_nest = true;
    std::size_t it = 0;
    TimingStats stats;
    
    auto sim_start = std::chrono::high_resolution_clock::now();
    
    while (cont_loop) {
        ++it;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                cont_loop = false;
        }
        advance_time( land, phen, pos_nest, pos_food, ants, food_quantity, stats );
        
        auto t1 = std::chrono::high_resolution_clock::now();
        renderer.display( win, food_quantity );
        win.blit();
        auto t2 = std::chrono::high_resolution_clock::now();
        stats.rendering_time += std::chrono::duration<double>(t2-t1).count();
        
        if ( not_food_in_nest && food_quantity > 0 ) {
            std::cout << "La première nourriture est arrivée au nid a l'iteration " << it << std::endl;
            stats.first_food_iteration = it;
            not_food_in_nest = false;
        }
        //SDL_Delay(10);
    }
    
    auto sim_end = std::chrono::high_resolution_clock::now();
    double total_time = std::chrono::duration<double>(sim_end - sim_start).count();
    
    // Compléter les statistiques
    stats.total_wall_time = total_time;
    stats.total_iterations = it;
    stats.food_collected = food_quantity;
    stats.first_food_iteration = (not_food_in_nest ? 0 : stats.first_food_iteration);
    stats.nb_ants = nb_ants;
    stats.grid_size = land.dimensions();
    stats.mode = "sequentiel";
    
    // Afficher les statistiques détaillées
    stats.print();
    
    // Exporter en CSV pour comparaison facile
    stats.export_csv("stats_fourmis.csv");
    
    SDL_Quit();
    return 0;
}
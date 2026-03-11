#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <fstream>
#include <iomanip>
#include "fractal_land.hpp"
#include "ant.hpp"
#include "pheronome.hpp"
#include "rand_generator.hpp"

void advance_time_vectorized( const fractal_land& land, pheronome& phen, 
                              const position_t& pos_nest, const position_t& pos_food,
                              AntColony& ants, std::size_t& cpteur,
                              double& t_ants, double& t_evap, double& t_update )
{
    using clock = std::chrono::high_resolution_clock;

    auto t1 = clock::now();
    for ( std::size_t i = 0; i < ants.size(); ++i )
        ants.advance_ant(i, phen, land, pos_food, pos_nest, cpteur);
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
    using clock = std::chrono::high_resolution_clock;

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
    AntColony::set_exploration_coef(eps);
    // On va créer des fourmis un peu partout sur la carte :
    AntColony ants;
    ants.reserve(nb_ants);
    auto gen_ant_pos = [&land, &seed] () { return rand_int32(0, land.dimensions()-1, seed); };
    for ( size_t i = 0; i < nb_ants; ++i )
        ants.push_back(position_t{gen_ant_pos(),gen_ant_pos()}, seed, 0);
    // On crée toutes les fourmis dans la fourmilière.
    pheronome phen(land.dimensions(), pos_food, pos_nest, alpha, beta);

    // Compteur de la quantité de nourriture apportée au nid par les fourmis
    size_t food_quantity = 0;
    bool not_food_in_nest = true;
    std::size_t it = 0;

    std::ofstream stats_file("stats_fourmis.csv");
    stats_file << "iteration,temps_total_ms,temps_fourmis_ms,temps_evap_ms,temps_update_ms,food_quantity\n";

    const int MAX_ITER = 5000;

    while (it < MAX_ITER) {
        ++it;
        auto iter_start = clock::now();

        double t_ants = 0.0, t_evap = 0.0, t_update = 0.0;

        advance_time_vectorized( land, phen, pos_nest, pos_food, ants, food_quantity,
                                 t_ants, t_evap, t_update );

        auto iter_end = clock::now();
        double t_total = std::chrono::duration<double, std::milli>(iter_end - iter_start).count();

        stats_file << it << ","
                   << std::fixed << std::setprecision(4)
                   << t_total << ","
                   << t_ants << ","
                   << t_evap << ","
                   << t_update << ","
                   << food_quantity << "\n";

        if ( not_food_in_nest && food_quantity > 0 ) {
            std::cout << "La première nourriture est arrivée au nid a l'iteration " << it << std::endl;
            not_food_in_nest = false;
        }
    }

    stats_file.close();
    return 0;
}
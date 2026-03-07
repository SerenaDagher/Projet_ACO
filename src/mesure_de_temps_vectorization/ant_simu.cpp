#include <vector>
#include <iostream>
#include <chrono>
#include <fstream>
#include <iomanip>
#include "fractal_land.hpp"
#include "pheronome.hpp"
#include "renderer.hpp"
#include "window.hpp"
#include "rand_generator.hpp"
#include "basic_types.hpp"

// ============================================================
//  Version vectorisée : Structure of Arrays (SoA)
//  Au lieu d'un vector<ant>, on utilise trois tableaux séparés :
//    - positions : coordonnées (x,y) de chaque fourmi
//    - states    : état chargé/non-chargé (0 ou 1)
//    - seeds     : graine aléatoire propre à chaque fourmi
// ============================================================

static double g_eps = 0.8;

// Avance toutes les fourmis d'un pas de temps (version SoA)
void advance_ants_soa(
    const fractal_land&       land,
    pheronome&                phen,
    const position_t&         pos_nest,
    const position_t&         pos_food,
    std::vector<position_t>&  positions,
    std::vector<int>&         states,
    std::vector<std::size_t>& seeds,
    std::size_t&              cpteur_food )
{
    const std::size_t nb_ants = positions.size();

    for ( std::size_t k = 0; k < nb_ants; ++k )
    {
        double consumed_time = 0.;

        while ( consumed_time < 1. )
        {
            int        ind_pher = states[k];
            double     choix    = rand_double( 0., 1., seeds[k] );
            position_t old_pos  = positions[k];
            position_t new_pos  = old_pos;

            double max_phen = std::max( { phen( new_pos.x-1, new_pos.y )[ind_pher],
                                          phen( new_pos.x+1, new_pos.y )[ind_pher],
                                          phen( new_pos.x,   new_pos.y-1 )[ind_pher],
                                          phen( new_pos.x,   new_pos.y+1 )[ind_pher] } );

            if ( ( choix > g_eps ) || ( max_phen <= 0. ) )
            {
                do {
                    new_pos = old_pos;
                    int d = rand_int32( 1, 4, seeds[k] );
                    if ( d == 1 ) new_pos.x -= 1;
                    if ( d == 2 ) new_pos.y -= 1;
                    if ( d == 3 ) new_pos.x += 1;
                    if ( d == 4 ) new_pos.y += 1;
                } while ( phen[new_pos][ind_pher] == -1 );
            }
            else
            {
                if      ( phen( new_pos.x-1, new_pos.y )[ind_pher] == max_phen ) new_pos.x -= 1;
                else if ( phen( new_pos.x+1, new_pos.y )[ind_pher] == max_phen ) new_pos.x += 1;
                else if ( phen( new_pos.x,   new_pos.y-1 )[ind_pher] == max_phen ) new_pos.y -= 1;
                else                                                                new_pos.y += 1;
            }

            consumed_time += land( new_pos.x, new_pos.y );
            phen.mark_pheronome( new_pos );
            positions[k] = new_pos;

            if ( new_pos.x == pos_nest.x && new_pos.y == pos_nest.y ) {
                if ( states[k] == 1 ) cpteur_food += 1;
                states[k] = 0;
            }
            if ( new_pos.x == pos_food.x && new_pos.y == pos_food.y ) {
                states[k] = 1;
            }
        }
    }
}

void advance_time(
    const fractal_land&       land,
    pheronome&                phen,
    const position_t&         pos_nest,
    const position_t&         pos_food,
    std::vector<position_t>&  positions,
    std::vector<int>&         states,
    std::vector<std::size_t>& seeds,
    std::size_t&              cpteur,
    double& t_ants, double& t_evap, double& t_update )
{
    using clock = std::chrono::high_resolution_clock;

    auto t1 = clock::now();
    advance_ants_soa( land, phen, pos_nest, pos_food, positions, states, seeds, cpteur );
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
    SDL_Init( SDL_INIT_VIDEO );
    std::size_t seed    = 2026;
    const int   nb_ants = 5000;
    const double eps    = 0.8;
    const double alpha  = 0.7;
    const double beta   = 0.999;

    g_eps = eps;

    position_t pos_nest{256, 256};
    position_t pos_food{500, 500};

    fractal_land land(8, 2, 1., 1024);

    double max_val = 0.0, min_val = 0.0;
    for ( fractal_land::dim_t i = 0; i < land.dimensions(); ++i )
        for ( fractal_land::dim_t j = 0; j < land.dimensions(); ++j ) {
            max_val = std::max(max_val, land(i,j));
            min_val = std::min(min_val, land(i,j));
        }
    double delta = max_val - min_val;
    for ( fractal_land::dim_t i = 0; i < land.dimensions(); ++i )
        for ( fractal_land::dim_t j = 0; j < land.dimensions(); ++j )
            land(i,j) = (land(i,j) - min_val) / delta;

    // --------------------------------------------------------
    //  Initialisation SoA : trois tableaux séparés
    // --------------------------------------------------------
    std::vector<position_t>  positions(nb_ants);
    std::vector<int>         states(nb_ants, 0);   // toutes non chargées
    std::vector<std::size_t> seeds(nb_ants);

    for ( int i = 0; i < nb_ants; ++i ) {
        std::size_t local_seed = seed + i;
        positions[i].x = rand_int32(0, land.dimensions()-1, local_seed);
        positions[i].y = rand_int32(0, land.dimensions()-1, local_seed);
        seeds[i]       = local_seed;
    }

    pheronome phen(land.dimensions(), pos_food, pos_nest, alpha, beta);

    Window   win("Ant Simulation (vectorized)", 2*land.dimensions()+10, land.dimensions()+266);
    Renderer renderer( land, phen, pos_nest, pos_food, positions, states );

    std::size_t food_quantity    = 0;
    SDL_Event   event;
    bool        cont_loop        = true;
    bool        not_food_in_nest = true;
    std::size_t it               = 0;

    std::ofstream stats_file("stats_fourmis_vectorized.csv");
    stats_file << "iteration,temps_total_ms,temps_fourmis_ms,temps_evap_ms,"
                  "temps_update_ms,temps_render_ms,food_quantity\n";

    using clock = std::chrono::high_resolution_clock;
    const int MAX_ITER = 5000;

    while ( cont_loop && it < MAX_ITER ) {
        ++it;
        auto iter_start = clock::now();

        while ( SDL_PollEvent(&event) )
            if ( event.type == SDL_QUIT ) cont_loop = false;

        double t_ants = 0., t_evap = 0., t_update = 0.;

        advance_time( land, phen, pos_nest, pos_food,
                      positions, states, seeds,
                      food_quantity, t_ants, t_evap, t_update );

        auto render_start = clock::now();
        renderer.display( win, food_quantity );
        win.blit();
        auto render_end = clock::now();

        auto iter_end = clock::now();

        double t_render = std::chrono::duration<double, std::milli>(render_end - render_start).count();
        double t_total  = std::chrono::duration<double, std::milli>(iter_end - iter_start).count();

        stats_file << it << ","
                   << std::fixed << std::setprecision(3)
                   << t_total  << ","
                   << t_ants   << ","
                   << t_evap   << ","
                   << t_update << ","
                   << t_render << ","
                   << food_quantity << "\n";

        if ( not_food_in_nest && food_quantity > 0 ) {
            std::cout << "La premiere nourriture est arrivee au nid a l'iteration " << it << std::endl;
            not_food_in_nest = false;
        }
    }

    stats_file.close();
    SDL_Quit();
    return 0;
}
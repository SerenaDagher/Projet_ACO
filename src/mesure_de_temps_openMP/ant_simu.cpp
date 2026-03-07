#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <fstream>
#include <iomanip>
#include "fractal_land.hpp"
#include "ant.hpp"
#include "pheronome.hpp"
#include "renderer.hpp"
#include "window.hpp"
#include "rand_generator.hpp"

void advance_time( const fractal_land& land, pheronome& phen, 
                   const position_t& pos_nest, const position_t& pos_food,
                   std::vector<ant>& ants, std::size_t& cpteur,
                   double& t_ants, double& t_evap, double& t_update )
{
    using clock = std::chrono::high_resolution_clock;

    auto t1 = clock::now();
    for ( size_t i = 0; i < ants.size(); ++i )
        ants[i].advance(phen, land, pos_food, pos_nest, cpteur);
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
    std::size_t seed = 2026;
    const int nb_ants = 5000;
    const double eps = 0.8;
    const double alpha = 0.7;
    const double beta = 0.999;

    position_t pos_nest{256,256};
    position_t pos_food{500,500};

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

    ant::set_exploration_coef(eps);

    std::vector<ant> ants;
    ants.reserve(nb_ants);
    auto gen_ant_pos = [&land, &seed] () { return rand_int32(0, land.dimensions()-1, seed); };
    for ( size_t i = 0; i < nb_ants; ++i )
        ants.emplace_back(position_t{gen_ant_pos(),gen_ant_pos()}, seed);

    pheronome phen(land.dimensions(), pos_food, pos_nest, alpha, beta);

    Window win("Ant Simulation", 2*land.dimensions()+10, land.dimensions()+266);
    Renderer renderer( land, phen, pos_nest, pos_food, ants );

    size_t food_quantity = 0;
    SDL_Event event;
    bool cont_loop = true;
    bool not_food_in_nest = true;
    std::size_t it = 0;

    std::ofstream stats_file("stats_fourmis.csv");
    stats_file << "iteration,temps_total_ms,temps_fourmis_ms,temps_evap_ms,temps_update_ms,temps_render_ms,food_quantity\n";

    using clock = std::chrono::high_resolution_clock;

    const int MAX_ITER = 5000;

    while (cont_loop && it < MAX_ITER) {
        ++it;
        auto iter_start = clock::now();

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                cont_loop = false;
        }

        double t_ants = 0.0, t_evap = 0.0, t_update = 0.0;

        advance_time( land, phen, pos_nest, pos_food, ants, food_quantity,
                      t_ants, t_evap, t_update );

        auto render_start = clock::now();
        renderer.display( win, food_quantity );
        win.blit();
        auto render_end = clock::now();

        auto iter_end = clock::now();

        double t_render = std::chrono::duration<double, std::milli>(render_end - render_start).count();
        double t_total  = std::chrono::duration<double, std::milli>(iter_end - iter_start).count();

        stats_file << it << ","
                   << std::fixed << std::setprecision(3)
                   << t_total << ","
                   << t_ants << ","
                   << t_evap << ","
                   << t_update << ","
                   << t_render << ","
                   << food_quantity << "\n";

        if ( not_food_in_nest && food_quantity > 0 ) {
            std::cout << "La première nourriture est arrivée au nid a l'iteration " << it << std::endl;
            not_food_in_nest = false;
        }
    }

    stats_file.close();
    SDL_Quit();
    return 0;
}
#include "ant.hpp"
#include <iostream>
#include <algorithm>
#include "rand_generator.hpp"

double AntColony::m_eps = 0.;

void AntColony::advance(std::size_t idx,
                         const pheronome& phen,
                         const fractal_land& land,
                         const position_t& pos_food,
                         const position_t& pos_nest,
                         std::size_t& cpteur_food,
                         std::vector<position_t>& visited_positions)
{
    auto ant_choice = [&]() mutable { return rand_double(0., 1., seeds[idx]); };
    auto dir_choice = [&]() mutable { return rand_int32(1, 4, seeds[idx]); };

    double consumed_time = 0.;

    while (consumed_time < 1.) {
        int        ind_pher    = states[idx];
        double     choix       = ant_choice();
        position_t old_pos_ant = { x[idx], y[idx] };
        position_t new_pos_ant = old_pos_ant;

        double max_phen = std::max({
            phen(new_pos_ant.x - 1, new_pos_ant.y)[ind_pher],
            phen(new_pos_ant.x + 1, new_pos_ant.y)[ind_pher],
            phen(new_pos_ant.x, new_pos_ant.y - 1)[ind_pher],
            phen(new_pos_ant.x, new_pos_ant.y + 1)[ind_pher]
        });

        if ((choix > AntColony::m_eps) || (max_phen <= 0.)) {
            do {
                new_pos_ant = old_pos_ant;
                int d = dir_choice();
                if (d == 1) new_pos_ant.x -= 1;
                if (d == 2) new_pos_ant.y -= 1;
                if (d == 3) new_pos_ant.x += 1;
                if (d == 4) new_pos_ant.y += 1;
            } while (phen[new_pos_ant][ind_pher] == -1);
        } else {
            if (phen(new_pos_ant.x - 1, new_pos_ant.y)[ind_pher] == max_phen)
                new_pos_ant.x -= 1;
            else if (phen(new_pos_ant.x + 1, new_pos_ant.y)[ind_pher] == max_phen)
                new_pos_ant.x += 1;
            else if (phen(new_pos_ant.x, new_pos_ant.y - 1)[ind_pher] == max_phen)
                new_pos_ant.y -= 1;
            else
                new_pos_ant.y += 1;
        }

        consumed_time += land(new_pos_ant.x, new_pos_ant.y);

        x[idx] = new_pos_ant.x;
        y[idx] = new_pos_ant.y;

        visited_positions.push_back(new_pos_ant);

        if (new_pos_ant == pos_nest) {
            if (states[idx] == 1) cpteur_food += 1;
            states[idx] = 0;
        }

        if (new_pos_ant == pos_food) {
            states[idx] = 1;
        }
    }
}
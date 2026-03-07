#include "ant.hpp"
#include <iostream>
#include <algorithm>
#include "rand_generator.hpp"

double ant::m_eps = 0.;
double AntColony::m_eps = 0.;

// ============================================================
// Version originale objet
// ============================================================

void ant::advance(pheronome& phen, const fractal_land& land,
                  const position_t& pos_food, const position_t& pos_nest,
                  std::size_t& cpteur_food)
{
    auto ant_choice = [this]() mutable { return rand_double(0., 1., this->m_seed); };
    auto dir_choice = [this]() mutable { return rand_int32(1, 4, this->m_seed); };

    double consumed_time = 0.;

    while (consumed_time < 1.) {
        int        ind_pher    = (is_loaded() ? 1 : 0);
        double     choix       = ant_choice();
        position_t old_pos_ant = get_position();
        position_t new_pos_ant = old_pos_ant;

        double max_phen = std::max({
            phen(new_pos_ant.x - 1, new_pos_ant.y)[ind_pher],
            phen(new_pos_ant.x + 1, new_pos_ant.y)[ind_pher],
            phen(new_pos_ant.x, new_pos_ant.y - 1)[ind_pher],
            phen(new_pos_ant.x, new_pos_ant.y + 1)[ind_pher]
        });

        if ((choix > m_eps) || (max_phen <= 0.)) {
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
        phen.mark_pheronome(new_pos_ant);
        m_position = new_pos_ant;

        if (get_position() == pos_nest) {
            if (is_loaded()) cpteur_food += 1;
            unset_loaded();
        }

        if (get_position() == pos_food) {
            set_loaded();
        }
    }
}

// ============================================================
// Version vectorisée OpenMP-safe
// ============================================================

void advance_ant_collect(std::size_t idx,
                         AntColony& colony,
                         const pheronome& phen,
                         const fractal_land& land,
                         const position_t& pos_food,
                         const position_t& pos_nest,
                         std::size_t& cpteur_food,
                         std::vector<position_t>& visited_positions)
{
    auto ant_choice = [&]() mutable { return rand_double(0., 1., colony.seeds[idx]); };
    auto dir_choice = [&]() mutable { return rand_int32(1, 4, colony.seeds[idx]); };

    double consumed_time = 0.;

    while (consumed_time < 1.) {
        int        ind_pher    = colony.states[idx];
        double     choix       = ant_choice();
        position_t old_pos_ant = { colony.x[idx], colony.y[idx] };
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

        colony.x[idx] = new_pos_ant.x;
        colony.y[idx] = new_pos_ant.y;

        visited_positions.push_back(new_pos_ant);

        if (new_pos_ant == pos_nest) {
            if (colony.states[idx] == 1) cpteur_food += 1;
            colony.states[idx] = 0;
        }

        if (new_pos_ant == pos_food) {
            colony.states[idx] = 1;
        }
    }
}



#ifndef _ANT_HPP_
#define _ANT_HPP_

#include <utility>
#include <vector>
#include "pheronome.hpp"
#include "fractal_land.hpp"
#include "basic_types.hpp"

struct AntColony
{
    std::vector<int> x;
    std::vector<int> y;
    std::vector<int> states;
    std::vector<std::size_t> seeds;

    static double m_eps;

    std::size_t size() const { return x.size(); }

    void reserve(std::size_t n)
    {
        x.reserve(n);
        y.reserve(n);
        states.reserve(n);
        seeds.reserve(n);
    }

    void push_back(const position_t& pos, std::size_t seed, int state = 0)
    {
        x.push_back(pos.x);
        y.push_back(pos.y);
        states.push_back(state);
        seeds.push_back(seed);
    }

    static void set_exploration_coef(double eps) { m_eps = eps; }

    void advance(std::size_t idx,
                         const pheronome& phen,
                         const fractal_land& land,
                         const position_t& pos_food,
                         const position_t& pos_nest,
                         std::size_t& cpteur_food,
                         std::vector<position_t>& visited_positions);
    };

#endif
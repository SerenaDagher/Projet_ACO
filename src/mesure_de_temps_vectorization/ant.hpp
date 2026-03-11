#pragma once

#include <vector>
#include <cstddef>
#include "basic_types.hpp"
#include "fractal_land.hpp"
#include "pheronome.hpp"

class ant
{
public:
    ant(position_t pos = {0,0}, std::size_t seed = 0)
        : m_position(pos), m_seed(seed), m_loaded(false) {}

    inline position_t get_position() const { return m_position; }
    inline bool is_loaded() const { return m_loaded; }
    inline void set_loaded() { m_loaded = true; }
    inline void unset_loaded() { m_loaded = false; }

    static inline void set_exploration_coef(double eps) { m_eps = eps; }

    void advance(pheronome& phen,
                 const fractal_land& land,
                 const position_t& pos_food,
                 const position_t& pos_nest,
                 std::size_t& cpteur_food);

private:
    position_t   m_position;
    std::size_t  m_seed;
    bool         m_loaded;

    static double m_eps;
};

// Représentation vectorisée des fourmis

struct AntColony
{
    std::vector<int> x;
    std::vector<int> y;
    std::vector<int> states;       
    std::vector<std::size_t> seeds; 

    static double m_eps;

    inline std::size_t size() const { return x.size(); }

    inline void reserve(std::size_t n)
    {
        x.reserve(n);
        y.reserve(n);
        states.reserve(n);
        seeds.reserve(n);
    }

    inline void push_back(position_t pos, std::size_t seed, int state = 0)
    {
        x.push_back(pos.x);
        y.push_back(pos.y);
        states.push_back(state);
        seeds.push_back(seed);
    }

    static inline void set_exploration_coef(double eps)
    {
        m_eps = eps;
    }
};

// Avance une seule fourmi vectorisée d’indice idx
void advance_ant(std::size_t idx,
                 AntColony& colony,
                 pheronome& phen,
                 const fractal_land& land,
                 const position_t& pos_food,
                 const position_t& pos_nest,
                 std::size_t& cpteur_food);
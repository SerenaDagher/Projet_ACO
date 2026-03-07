#ifndef _ANT_HPP_
#define _ANT_HPP_

#include <utility>
#include <vector>
#include "pheronome.hpp"
#include "fractal_land.hpp"
#include "basic_types.hpp"

class ant
{
public:
    enum state { unloaded = 0, loaded = 1 };

    ant(const position_t& pos, std::size_t seed)
        : m_seed(seed), m_state(unloaded), m_position(pos)
    {}

    ant(const ant& a) = default;
    ant(ant&& a) = default;
    ~ant() = default;

    void set_loaded() { m_state = loaded; }
    void unset_loaded() { m_state = unloaded; }

    bool is_loaded() const { return m_state == loaded; }
    const position_t& get_position() const { return m_position; }
    static void set_exploration_coef(double eps) { m_eps = eps; }

    void advance(pheronome& phen, const fractal_land& land,
                 const position_t& pos_food, const position_t& pos_nest,
                 std::size_t& cpteur_food);

private:
    static double m_eps;
    std::size_t m_seed;
    state m_state;
    position_t m_position;
};

// ============================================================
// Version vectorisée : Structure of Arrays
// ============================================================

struct AntColony
{
    std::vector<int> x;
    std::vector<int> y;
    std::vector<int> states;              // 0 = unloaded, 1 = loaded
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
};

/**
 * Fait avancer une fourmi vectorisée d'indice idx.
 * Les positions visitées sont stockées dans visited_positions,
 * puis les phéromones seront marqués après la boucle locale.
 */
void advance_ant_collect(std::size_t idx,
                         AntColony& colony,
                         const pheronome& phen,
                         const fractal_land& land,
                         const position_t& pos_food,
                         const position_t& pos_nest,
                         std::size_t& cpteur_food,
                         std::vector<position_t>& visited_positions);

#endif
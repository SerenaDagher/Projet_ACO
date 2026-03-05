// ant.hpp
#ifndef _ANT_HPP_
# define _ANT_HPP_
# include <utility>
# include "pheronome.hpp"
# include "fractal_land.hpp"
# include "basic_types.hpp"

class ant
{
public:
    /**
     * Une fourmi peut être dans deux états possibles : chargée ( elle porte de la nourriture ) ou non chargée
     */
    enum state { unloaded = 0, loaded = 1 };
    ant(const position_t& pos, std::size_t seed ) : m_state(unloaded), m_position(pos)
    {} 
    ant(const ant& a) = default;
    ant( ant&& a ) = default;
    ~ant() = default;

    void set_loaded() { m_state = loaded; }
    void unset_loaded() { m_state = unloaded; }

    bool is_loaded() const { return m_state == loaded; }
    const position_t& get_position() const { return m_position; }
    static void set_exploration_coef(double eps) { m_eps = eps; }

    void advance( pheronome& phen, const fractal_land& land,
                  const position_t& pos_food, const position_t& pos_nest, std::size_t& cpteur_food );

private:
    static double m_eps; // Coefficient d'exploration commun à toutes les fourmis.
    std::size_t m_seed;
    state m_state;
    position_t m_position;
};

// ============================================================
// VERSION VECTORISÉE : Structure of Arrays (SoA)
// ============================================================

/**
 * @brief Représentation vectorisée d'une colonie de fourmis (Structure of Arrays)
 * @details Au lieu d'un tableau d'objets ant (Array of Structures),
 *          on utilise des tableaux séparés pour chaque champ.
 *          Cela améliore la localité mémoire et prépare la parallélisation OpenMP.
 */
struct AntColony {
    std::vector<int>         x;       ///< Coordonnée x de chaque fourmi
    std::vector<int>         y;       ///< Coordonnée y de chaque fourmi
    std::vector<int>         states;  ///< État de chaque fourmi : 0 = non chargée, 1 = chargée
    std::vector<std::size_t> seeds;   ///< Graine aléatoire de chaque fourmi

    std::size_t size() const { return x.size(); }

    static double m_eps; ///< Coefficient d'exploration commun à toutes les fourmis
    static void set_exploration_coef(double eps) { m_eps = eps; }
};

/**
 * @brief Fait avancer la fourmi d'indice idx dans la colonie vectorisée
 * @details Logique identique à ant::advance(), mais opère sur les tableaux SoA
 *          via l'indice idx. Prête pour une parallélisation OpenMP sur la boucle appelante.
 */
void advance_ant( std::size_t idx, AntColony& colony,
                  pheronome& phen, const fractal_land& land,
                  const position_t& pos_food, const position_t& pos_nest,
                  std::size_t& cpteur_food );

#endif
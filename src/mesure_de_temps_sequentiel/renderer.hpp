#pragma once
#include "fractal_land.hpp"
#include "ant.hpp"
#include "pheronome.hpp"
#include "window.hpp"

class Renderer
{
public:
    Renderer(  const fractal_land& land, const pheronome& phen, 
               const position_t& pos_nest, const position_t& pos_food,
               const std::vector<ant>& ants );

    Renderer(const Renderer& ) = delete;
    ~Renderer();

    void display( Window& win, std::size_t const& compteur );
private:
    fractal_land const& m_ref_land;
    SDL_Texture* m_land{ nullptr }; 
    const pheronome& m_ref_phen;
    const position_t& m_pos_nest;
    const position_t& m_pos_food;
    const std::vector<ant>& m_ref_ants;
    std::vector<std::size_t> m_curve;    
};


// ============================================================
// VERSION VECTORISÉE : Renderer pour AntColony (SoA)
// ============================================================

/**
 * @brief Renderer utilisant la représentation vectorisée AntColony
 * @details Même comportement visuel que Renderer, mais prend une AntColony
 *          au lieu d'un std::vector<ant>. Les positions sont lues directement
 *          depuis les tableaux colony.x et colony.y.
 */
class RendererV
{
public:
    RendererV( const fractal_land& land, const pheronome& phen,
               const position_t& pos_nest, const position_t& pos_food,
               const AntColony& colony );

    RendererV(const RendererV& ) = delete;
    ~RendererV();

    void display( Window& win, std::size_t const& compteur );

private:
    fractal_land const&      m_ref_land;
    SDL_Texture*             m_land{ nullptr };
    const pheronome&         m_ref_phen;
    const position_t&        m_pos_nest;
    const position_t&        m_pos_food;
    const AntColony&         m_ref_colony;  ///< Référence vers la colonie vectorisée
    std::vector<std::size_t> m_curve;
};
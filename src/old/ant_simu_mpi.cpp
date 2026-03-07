#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <omp.h>
#include <mpi.h>
#include "fractal_land.hpp"
#include "ant.hpp"
#include "pheronome.hpp"
#include "renderer.hpp"
#include "window.hpp"
#include "rand_generator.hpp"

struct TimingStats {
    double ant_move_time = 0.0;
    double pheromone_sync_time = 0.0;
    double pheromone_evaporation_time = 0.0;
    double pheromone_update_time = 0.0;

    size_t food_collected = 0;
    size_t first_food_iteration = 0;
    size_t total_iterations = 0;
    double total_wall_time = 0.0;

    int nb_ants = 0;
    int grid_size = 0;
    int nb_procs = 1;
    int nb_threads = 1;
    int rank = 0;
    std::string mode = "mpi";

    void print() const {
        if (rank != 0) return;
        double total_compute = ant_move_time + pheromone_sync_time
                             + pheromone_evaporation_time + pheromone_update_time;
        double overhead = total_wall_time - total_compute;

        std::cout << "\n============================================================" << std::endl;
        std::cout << "     STATISTIQUES DE PERFORMANCE - MODE MPI (strategie 1)" << std::endl;
        std::cout << "============================================================" << std::endl;
        std::cout << std::fixed << std::setprecision(6);

        std::cout << "  CONFIGURATION:" << std::endl;
        std::cout << "    Nombre de fourmis:         " << std::setw(10) << nb_ants << std::endl;
        std::cout << "    Taille de la grille:       " << grid_size << "x" << grid_size << std::endl;
        std::cout << "    Nombre de processus MPI:   " << std::setw(10) << nb_procs << std::endl;
        std::cout << "    Threads OMP par proc:      " << std::setw(10) << nb_threads << std::endl;
        std::cout << "    Total coeurs utilises:     " << std::setw(10) << nb_procs * nb_threads << std::endl;
        std::cout << "    Iterations totales:        " << std::setw(10) << total_iterations << std::endl;
        std::cout << "------------------------------------------------------------" << std::endl;

        std::cout << "  RESULTATS DE SIMULATION:" << std::endl;
        std::cout << "    Nourriture collectee:      " << std::setw(10) << food_collected << std::endl;
        std::cout << "    Premiere nourriture (iter):" << std::setw(10) << first_food_iteration << std::endl;
        std::cout << "------------------------------------------------------------" << std::endl;

        std::cout << "  TEMPS DE CALCUL (en secondes):" << std::endl;
        std::cout << "    1. Deplacement fourmis:    " << std::setw(10) << ant_move_time << " s  ("
                  << std::setw(5) << std::setprecision(1) << (ant_move_time/total_compute*100) << "%)" << std::endl;
        std::cout << std::setprecision(6);
        std::cout << "    2. Sync pheromones (MPI):  " << std::setw(10) << pheromone_sync_time << " s  ("
                  << std::setw(5) << std::setprecision(1) << (pheromone_sync_time/total_compute*100) << "%)" << std::endl;
        std::cout << std::setprecision(6);
        std::cout << "    3. Evaporation:            " << std::setw(10) << pheromone_evaporation_time << " s  ("
                  << std::setw(5) << std::setprecision(1) << (pheromone_evaporation_time/total_compute*100) << "%)" << std::endl;
        std::cout << std::setprecision(6);
        std::cout << "    4. MAJ pheromones:         " << std::setw(10) << pheromone_update_time << " s  ("
                  << std::setw(5) << std::setprecision(1) << (pheromone_update_time/total_compute*100) << "%)" << std::endl;
        std::cout << std::setprecision(6);
        std::cout << "    ---------------------------------------------------" << std::endl;
        std::cout << "    TOTAL CALCUL:              " << std::setw(10) << total_compute << " s" << std::endl;
        std::cout << "    Overhead/comm:             " << std::setw(10) << overhead << " s" << std::endl;
        std::cout << "    TEMPS TOTAL (wall time):   " << std::setw(10) << total_wall_time << " s" << std::endl;
        std::cout << "------------------------------------------------------------" << std::endl;

        std::cout << "  METRIQUES DE PERFORMANCE:" << std::endl;
        std::cout << std::setprecision(3);
        std::cout << "    Temps moyen/iteration:     " << std::setw(10) << (total_compute/total_iterations*1000) << " ms" << std::endl;
        std::cout << std::setprecision(2);
        std::cout << "    Iterations/seconde:        " << std::setw(10) << (total_iterations/total_compute) << " iter/s" << std::endl;
        std::cout << "============================================================\n" << std::endl;
    }

    void export_csv(const std::string& filename) const {
        if (rank != 0) return;
        std::ofstream file(filename, std::ios::app);
        if (!file.is_open()) {
            std::cerr << "Erreur: impossible d'ouvrir " << filename << std::endl;
            return;
        }
        file.seekp(0, std::ios::end);
        if (file.tellp() == 0) {
            file << "mode,nb_fourmis,nb_procs,nb_threads,total_coeurs,taille_grille,iterations,"
                 << "nourriture_collectee,premiere_nourriture_iter,"
                 << "temps_fourmis,temps_sync_mpi,temps_evaporation,temps_maj,"
                 << "temps_wall,temps_moyen_iter_ms,iter_par_sec\n";
        }
        double total_compute = ant_move_time + pheromone_sync_time
                             + pheromone_evaporation_time + pheromone_update_time;
        file << std::fixed << std::setprecision(6);
        file << mode << ","
             << nb_ants << ","
             << nb_procs << ","
             << nb_threads << ","
             << nb_procs * nb_threads << ","
             << grid_size << ","
             << total_iterations << ","
             << food_collected << ","
             << first_food_iteration << ","
             << ant_move_time << ","
             << pheromone_sync_time << ","
             << pheromone_evaporation_time << ","
             << pheromone_update_time << ","
             << total_wall_time << ","
             << (total_compute/total_iterations*1000) << ","
             << (total_iterations/total_compute) << "\n";
        file.close();
        std::cout << "+ Statistiques exportees dans: " << filename << std::endl;
    }
};

// ============================================================
// Calcule le découpage de lignes pour le proc rank
// Retourne i_start et i_end dans les coordonnées du buffer
// (donc déjà décalées de +1 pour les cellules fantômes)
// ============================================================
static void get_row_range( int rank, int nb_procs, std::size_t dim,
                            std::size_t& i_start, std::size_t& i_end,
                            int& count, int& offset )
{
    std::size_t chunk = (dim + nb_procs - 1) / nb_procs;
    i_start = static_cast<std::size_t>(rank) * chunk;          // 0-based dans [0, dim[
    i_end   = std::min(i_start + chunk, dim);
    // En coordonnées buffer (+1 pour les cellules fantômes)
    i_start += 1;
    i_end   += 1;
    // Nombre de doubles dans la tranche : nb_lignes * stride * 2
    // (calculé par l'appelant qui connaît stride)
    count  = static_cast<int>(i_end - i_start);
    offset = static_cast<int>(i_start) - 1; // offset en lignes dans [0, dim[
}

// ============================================================
// advance_time - stratégie 1 MPI
//
// Schéma par itération :
//   1. Chaque proc avance ses fourmis → marque son buffer pheronome local
//   2. MPI_Allreduce(MAX) sur le buffer → tous les procs ont le même état
//   3. Chaque proc évapore SA tranche de lignes dans son buffer local
//   4. MPI_Allgatherv des tranches évaporées → reconstruction du buffer complet
//   5. phen.update() → swap buffer → carte active à jour
// ============================================================
void advance_time( const fractal_land& land, pheronome& phen,
                   const position_t& pos_nest, const position_t& pos_food,
                   AntColony& colony, std::size_t& cpteur,
                   TimingStats& stats,
                   int rank, int nb_procs,
                   std::vector<double>& sync_buf,
                   const std::vector<int>& evap_counts,  // nb doubles par proc
                   const std::vector<int>& evap_displs ) // offset en doubles par proc
{
    const std::size_t stride   = phen.get_stride();
    const std::size_t dim      = phen.get_dim();
    const std::size_t buf_size = stride * stride * 2;

    // ----------------------------------------------------------
    // 1. Déplacement des fourmis locales (OMP inside)
    //    On sépare le calcul pur de la comm MPI (Allreduce food)
    // ----------------------------------------------------------
    double t1 = MPI_Wtime();

    std::size_t local_food = 0;
    #pragma omp parallel for reduction(+:local_food) schedule(dynamic, 64)
    for ( std::size_t i = 0; i < colony.size(); ++i )
        advance_ant( i, colony, phen, land, pos_food, pos_nest, local_food );

    double t2 = MPI_Wtime(); // fin calcul fourmis pur

    // Agrégation du compteur nourriture — comm MPI, compté dans sync
    std::size_t global_food = 0;
    MPI_Allreduce(&local_food, &global_food, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
    cpteur += global_food;

    double t3 = MPI_Wtime(); // fin sync phéromones + food

    // ----------------------------------------------------------
    // 2. Synchronisation des phéromones après déplacement
    //    MPI_Allreduce(MAX) : on prend le max car l'ordre des
    //    fourmis est arbitraire entre les procs.
    //    copy_buffer_to/from fait partie de la sync (préparation
    //    du message MPI)
    // ----------------------------------------------------------
    phen.copy_buffer_to(sync_buf.data());
    MPI_Allreduce(MPI_IN_PLACE, sync_buf.data(), static_cast<int>(buf_size),
                  MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    phen.copy_buffer_from(sync_buf.data());

    double t4 = MPI_Wtime(); // fin sync phéromones

    // ----------------------------------------------------------
    // 3. Évaporation : chaque proc traite uniquement sa tranche
    //    de lignes. Calcul pur, mesuré séparément.
    // ----------------------------------------------------------
    std::size_t i_start, i_end;
    int row_count, row_offset;
    get_row_range(rank, nb_procs, dim, i_start, i_end, row_count, row_offset);

    #pragma omp parallel for schedule(static)
    for ( std::size_t i = i_start; i < i_end; ++i )
        for ( std::size_t j = 1; j <= dim; ++j )
            phen.evaporate_cell(i, j);

    double t5 = MPI_Wtime(); // fin évaporation calcul pur

    // ----------------------------------------------------------
    // 4. Reconstruction du buffer après évaporation via Allgatherv
    //    copy_buffer_to est préparation du message → compté dans sync
    // ----------------------------------------------------------
    phen.copy_buffer_to(sync_buf.data());
    MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                   sync_buf.data(), evap_counts.data(), evap_displs.data(),
                   MPI_DOUBLE, MPI_COMM_WORLD);
    phen.copy_buffer_from(sync_buf.data());

    double t6 = MPI_Wtime(); // fin sync après évaporation

    // ----------------------------------------------------------
    // 5. Update : swap buffer → carte active, reset food/nest
    // ----------------------------------------------------------
    phen.update();

    double t7 = MPI_Wtime();

    // ant_move_time     = calcul fourmis pur (sans comm)
    // pheromone_sync    = comm MPI food + comm MPI phéromones + comm MPI après évap
    // evaporation_time  = calcul évaporation pur (sans comm)
    // update_time       = swap buffer
    stats.ant_move_time              += t2 - t1;
    stats.pheromone_sync_time        += (t3 - t2) + (t4 - t3) + (t6 - t5);
    stats.pheromone_evaporation_time += t5 - t4;
    stats.pheromone_update_time      += t7 - t6;
}

int main(int nargs, char* argv[])
{
    MPI_Init(&nargs, &argv);

    int rank, nb_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nb_procs);

    int max_iterations = 5000;

    for (int i = 1; i < nargs; ++i) {
        std::string arg = argv[i];
        if (arg == "--iterations" && i+1 < nargs) {
            max_iterations = std::atoi(argv[++i]);
        } else if (arg == "--threads" && i+1 < nargs) {
            omp_set_num_threads(std::atoi(argv[++i]));
        } else if (arg == "--help") {
            if (rank == 0) {
                std::cout << "Usage: mpirun -np <N> " << argv[0] << " [options]\n";
                std::cout << "  --iterations <n>   Executer n iterations (defaut: 5000)\n";
                std::cout << "  --threads <n>      Threads OMP par processus (defaut: max)\n";
                std::cout << "\n";
                std::cout << "Exemples benchmark hybride MPI+OMP (16 coeurs totaux) :\n";
                std::cout << "  mpirun -np 1  " << argv[0] << " --iterations 5000 --threads 16\n";
                std::cout << "  mpirun -np 2  " << argv[0] << " --iterations 5000 --threads 8\n";
                std::cout << "  mpirun -np 4  " << argv[0] << " --iterations 5000 --threads 4\n";
                std::cout << "  mpirun -np 8  " << argv[0] << " --iterations 5000 --threads 2\n";
                std::cout << "  mpirun -np 16 " << argv[0] << " --iterations 5000 --threads 1\n";
            }
            MPI_Finalize();
            return 0;
        }
    }

    std::size_t seed = 2026;
    const int nb_ants = 5000;
    const double eps   = 0.8;
    const double alpha = 0.7;
    const double beta  = 0.999;
    position_t pos_nest{256,256};
    position_t pos_food{500,500};

    if (rank == 0) {
        std::cout << "Configuration MPI (strategie 1):" << std::endl;
        std::cout << "  Nombre de processus MPI: " << nb_procs << std::endl;
        std::cout << "  Threads OMP par proc:    " << omp_get_max_threads() << std::endl;
        std::cout << "  Total coeurs utilises:   " << nb_procs * omp_get_max_threads() << std::endl;
        std::cout << "  Nombre de fourmis:       " << nb_ants << std::endl;
        std::cout << "  Taille de la carte:      512x512" << std::endl;
        std::cout << "  Iterations max:          " << max_iterations << std::endl;
        std::cout << std::endl;
    }

    // ----------------------------------------------------------
    // Génération du territoire (même graine sur tous les procs)
    // ----------------------------------------------------------
    fractal_land land(8,2,1.,1024);
    double max_val = 0.0, min_val = 0.0;
    for ( fractal_land::dim_t i = 0; i < land.dimensions(); ++i )
        for ( fractal_land::dim_t j = 0; j < land.dimensions(); ++j ) {
            max_val = std::max(max_val, land(i,j));
            min_val = std::min(min_val, land(i,j));
        }
    double delta = max_val - min_val;
    for ( fractal_land::dim_t i = 0; i < land.dimensions(); ++i )
        for ( fractal_land::dim_t j = 0; j < land.dimensions(); ++j )
            land(i,j) = (land(i,j)-min_val)/delta;

    AntColony::set_exploration_coef(eps);

    // ----------------------------------------------------------
    // Découpage des fourmis : proc k gère les fourmis [local_start, local_start+local_count[
    // ----------------------------------------------------------
    int chunk_ants  = nb_ants / nb_procs;
    int extra_ants  = nb_ants % nb_procs;
    int local_start = rank * chunk_ants + std::min(rank, extra_ants);
    int local_count = chunk_ants + (rank < extra_ants ? 1 : 0);

    AntColony colony;
    colony.x.resize(local_count);
    colony.y.resize(local_count);
    colony.states.resize(local_count, 0);
    colony.seeds.resize(local_count);

    // Générer la séquence complète (même sur tous les procs) et ne garder que la tranche locale
    std::size_t init_seed = seed;
    auto gen_pos = [&land, &init_seed]() {
        return rand_int32(0, land.dimensions()-1, init_seed);
    };
    for ( int i = 0; i < nb_ants; ++i ) {
        int px = gen_pos();
        int py = gen_pos();
        if ( i >= local_start && i < local_start + local_count ) {
            int li = i - local_start;
            colony.x[li]     = px;
            colony.y[li]     = py;
            colony.seeds[li] = init_seed + i;
        }
    }

    // ----------------------------------------------------------
    // Carte de phéromones complète sur chaque proc
    // ----------------------------------------------------------
    pheronome phen(land.dimensions(), pos_food, pos_nest, alpha, beta);

    const std::size_t stride   = phen.get_stride();
    const std::size_t dim      = phen.get_dim();
    const std::size_t buf_size = stride * stride * 2;

    // ----------------------------------------------------------
    // Précalcul des counts/displs pour MPI_Allgatherv (évaporation)
    // Chaque proc envoie sa tranche de lignes : nb_lignes * stride * 2 doubles
    // ----------------------------------------------------------
    std::vector<int> evap_counts(nb_procs), evap_displs(nb_procs);
    for (int p = 0; p < nb_procs; ++p) {
        std::size_t is, ie;
        int rc, ro;
        get_row_range(p, nb_procs, dim, is, ie, rc, ro);
        evap_counts[p] = static_cast<int>((ie - is) * stride * 2);
        evap_displs[p] = static_cast<int>((is - 1 + 1) * stride * 2); // offset dans le buffer plat
        // Note : is est déjà +1 (cellules fantômes), donc offset réel = (is-1)*stride*2
        evap_displs[p] = static_cast<int>((is - 1) * stride * 2);
    }

    std::vector<double> sync_buf(buf_size, 0.0);

    size_t food_quantity = 0;
    bool cont_loop = true;
    bool not_food_in_nest = true;
    std::size_t it = 0;
    TimingStats stats;
    stats.rank     = rank;
    stats.nb_procs = nb_procs;

    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0)
        std::cout << "Demarrage de la simulation..." << std::endl;
    auto sim_start = std::chrono::high_resolution_clock::now();

    while (cont_loop) {
        ++it;

        advance_time( land, phen, pos_nest, pos_food,
                      colony, food_quantity, stats,
                      rank, nb_procs,
                      sync_buf, evap_counts, evap_displs );

        if ( rank == 0 && not_food_in_nest && food_quantity > 0 ) {
            std::cout << "+ Premiere nourriture au nid a l'iteration " << it << std::endl;
            stats.first_food_iteration = it;
            not_food_in_nest = false;
        }

        if ( static_cast<int>(it) >= max_iterations ) {
            if (rank == 0)
                std::cout << "Nombre maximum d'iterations atteint (" << max_iterations << ")" << std::endl;
            cont_loop = false;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    auto sim_end = std::chrono::high_resolution_clock::now();

    // Agréger les temps : on prend le MAX sur tous les procs (le plus lent = goulot)
    double local_times[4] = {
        stats.ant_move_time,
        stats.pheromone_sync_time,
        stats.pheromone_evaporation_time,
        stats.pheromone_update_time
    };
    double global_times[4];
    MPI_Reduce(local_times, global_times, 4, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        stats.ant_move_time              = global_times[0];
        stats.pheromone_sync_time        = global_times[1];
        stats.pheromone_evaporation_time = global_times[2];
        stats.pheromone_update_time      = global_times[3];
        stats.total_wall_time  = std::chrono::duration<double>(sim_end - sim_start).count();
        stats.total_iterations = it;
        stats.food_collected   = food_quantity;
        stats.first_food_iteration = (not_food_in_nest ? 0 : stats.first_food_iteration);
        stats.nb_ants          = nb_ants;
        stats.grid_size        = land.dimensions();
        stats.nb_threads       = omp_get_max_threads();
        stats.mode             = "mpi";
        stats.print();
        stats.export_csv("stats_fourmis.csv");
    }

    MPI_Finalize();
    return 0;
}
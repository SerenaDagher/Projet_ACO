#include <mpi.h>

#include <vector>
#include <iostream>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstdlib>

#include "basic_types.hpp"
#include "fractal_land.hpp"
#include "ant.hpp"
#include "pheronome_local.hpp"
#include "rand_generator.hpp"

struct TimingStats
{
    double sum_border  = 0.0;
    double sum_move    = 0.0;
    double sum_migrate = 0.0;
    double sum_evap    = 0.0;

    double sum2_border  = 0.0;
    double sum2_move    = 0.0;
    double sum2_migrate = 0.0;
    double sum2_evap    = 0.0;

    std::size_t count = 0;
};

static void split_range(int n, int rank, int size, int& begin, int& end)
{
    int base = n / size;
    int rem  = n % size;
    begin = rank * base + std::min(rank, rem);
    end   = begin + base + (rank < rem ? 1 : 0);
}

static int owner_of_row(int y_global, int nrows, int nprocs)
{
    for (int p = 0; p < nprocs; ++p) {
        int b = 0, e = 0;
        split_range(nrows, p, nprocs, b, e);
        if (y_global >= b && y_global < e) return p;
    }
    return -1;
}

static int global_to_local_y(int y_global, int y_begin)
{
    return (y_global - y_begin) + 1;
}

static double get_phero_value(const LocalPheromoneGrid& phen,
                              int x,
                              int y_global,
                              int y_begin,
                              int y_end,
                              int ind_pher,
                              int ny)
{
    if (x < 0 || x >= phen.nx()) return -1.0;
    if (y_global < 0 || y_global >= ny) return -1.0;

    int y_local = global_to_local_y(y_global, y_begin);

    if (y_local < 0 || y_local > phen.local_ny() + 1) return -1.0;

    const PheroCell& c = phen.at(x, y_local);
    return (ind_pher == 0) ? c.v1 : c.v2;
}

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int rank = 0, nprocs = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    using clock = std::chrono::high_resolution_clock;

    std::uint64_t seed = 2026;
    const int nb_ants = 5000;
    const int max_iter = 5000;
    const double eps = 0.8;
    const double alpha = 0.7;
    const double beta = 0.999;

    position_t pos_nest{256,256};
    position_t pos_food{500,500};

    fractal_land land(8, 2, 1., 1024);

    double max_val = 0.0;
    double min_val = 0.0;
    for (fractal_land::dim_t i = 0; i < land.dimensions(); ++i) {
        for (fractal_land::dim_t j = 0; j < land.dimensions(); ++j) {
            max_val = std::max(max_val, land(i,j));
            min_val = std::min(min_val, land(i,j));
        }
    }

    double delta = max_val - min_val;
    for (fractal_land::dim_t i = 0; i < land.dimensions(); ++i) {
        for (fractal_land::dim_t j = 0; j < land.dimensions(); ++j) {
            land(i,j) = (land(i,j) - min_val) / delta;
        }
    }

    const int nx = static_cast<int>(land.dimensions());
    const int ny = static_cast<int>(land.dimensions());

    int y_begin = 0, y_end = 0;
    split_range(ny, rank, nprocs, y_begin, y_end);
    const int local_ny = y_end - y_begin;

    LocalPheromoneGrid phen(nx, local_ny, alpha, beta);

    if (pos_food.y >= y_begin && pos_food.y < y_end) {
        int yl = global_to_local_y(pos_food.y, y_begin);
        phen.at(pos_food.x, yl).v1 = 1.0;
    }
    if (pos_nest.y >= y_begin && pos_nest.y < y_end) {
        int yl = global_to_local_y(pos_nest.y, y_begin);
        phen.at(pos_nest.x, yl).v2 = 1.0;
    }

    AntColonyMPI colony;

    auto gen_ant_pos = [&land, &seed]() {
        return rand_int32(0, static_cast<int>(land.dimensions()) - 1, reinterpret_cast<std::size_t&>(seed));
    };

    for (int i = 0; i < nb_ants; ++i) {
        int x = gen_ant_pos();
        int y = gen_ant_pos();
        int owner = owner_of_row(y, ny, nprocs);
        if (owner == rank) {
            colony.ants.push_back({x, y, 0, seed + static_cast<std::uint64_t>(i)});
        }
    }

    MPI_Datatype MPI_ANT;
    {
        AntMPI dummy;
        int lengths[4] = {1, 1, 1, 1};
        MPI_Aint displs[4];
        MPI_Aint base;

        MPI_Get_address(&dummy, &base);
        MPI_Get_address(&dummy.x, &displs[0]);
        MPI_Get_address(&dummy.y_global, &displs[1]);
        MPI_Get_address(&dummy.state, &displs[2]);
        MPI_Get_address(&dummy.seed, &displs[3]);

        for (int i = 0; i < 4; ++i) displs[i] -= base;

        MPI_Datatype types[4] = {
            MPI_INT, MPI_INT, MPI_INT, MPI_UNSIGNED_LONG_LONG
        };

        MPI_Type_create_struct(4, lengths, displs, types, &MPI_ANT);
        MPI_Type_commit(&MPI_ANT);
    }

    std::size_t local_food_quantity = 0;
    std::size_t global_food_quantity = 0;

    int first_food_iter_local = -1;
    int first_food_iter_global = -1;

    TimingStats stats;

    std::ofstream csv;
    if (rank == 0) {
        csv.open("stats_fourmis_mpi_domain.csv");
        csv << "iteration,temps_border_ms,temps_move_ms,temps_migrate_ms,temps_evap_ms,food_quantity\n";
    }

    for (int it = 1; it <= max_iter; ++it) {
        
        auto tb1 = clock::now();

        int up   = (rank > 0) ? rank - 1 : MPI_PROC_NULL;
        int down = (rank < nprocs - 1) ? rank + 1 : MPI_PROC_NULL;

        std::vector<double> send_first, recv_from_up;
        std::vector<double> send_last,  recv_from_down;

        phen.pack_row(1, send_first);
        phen.pack_row(local_ny, send_last);

        recv_from_up.resize(send_first.size(), 0.0);
        recv_from_down.resize(send_last.size(), 0.0);

        MPI_Sendrecv(send_first.data(), static_cast<int>(send_first.size()), MPI_DOUBLE,
                     up, 100,
                     recv_from_down.data(), static_cast<int>(recv_from_down.size()), MPI_DOUBLE,
                     down, 100,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_Sendrecv(send_last.data(), static_cast<int>(send_last.size()), MPI_DOUBLE,
                     down, 101,
                     recv_from_up.data(), static_cast<int>(recv_from_up.size()), MPI_DOUBLE,
                     up, 101,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (up != MPI_PROC_NULL) {
            phen.unpack_row(0, recv_from_up);
        }
        if (down != MPI_PROC_NULL) {
            phen.unpack_row(local_ny + 1, recv_from_down);
        }

        auto tb2 = clock::now();
        double local_t_border = std::chrono::duration<double, std::milli>(tb2 - tb1).count();
        
        auto tm1 = clock::now();

        phen.copy_data_to_buffer();

        std::vector<std::pair<int,int>> visited_local_cells;
        visited_local_cells.reserve(colony.ants.size() * 2);

        std::vector<std::vector<AntMPI>> outgoing(static_cast<std::size_t>(nprocs));
        std::vector<AntMPI> staying;
        staying.reserve(colony.ants.size());

        std::size_t food_delta_this_iter = 0;

        for (auto ant : colony.ants) {
            auto ant_choice = [&]() mutable {
                return rand_double(0., 1., reinterpret_cast<std::size_t&>(ant.seed));
            };
            auto dir_choice = [&]() mutable {
                return rand_int32(1, 4, reinterpret_cast<std::size_t&>(ant.seed));
            };

            double consumed_time = 0.0;
            bool migrated = false;

            while (consumed_time < 1.0) {
                int ind_pher = ant.state;
                double choix = ant_choice();

                int old_x = ant.x;
                int old_y = ant.y_global;
                int new_x = old_x;
                int new_y = old_y;

                double max_phen = std::max({
                    get_phero_value(phen, old_x - 1, old_y,     y_begin, y_end, ind_pher, ny),
                    get_phero_value(phen, old_x + 1, old_y,     y_begin, y_end, ind_pher, ny),
                    get_phero_value(phen, old_x,     old_y - 1, y_begin, y_end, ind_pher, ny),
                    get_phero_value(phen, old_x,     old_y + 1, y_begin, y_end, ind_pher, ny)
                });

                if ((choix > eps) || (max_phen <= 0.0)) {
                    do {
                        new_x = old_x;
                        new_y = old_y;
                        int d = dir_choice();
                        if (d == 1) new_x -= 1;
                        if (d == 2) new_y -= 1;
                        if (d == 3) new_x += 1;
                        if (d == 4) new_y += 1;
                    } while (new_x < 0 || new_x >= nx || new_y < 0 || new_y >= ny);
                } else {
                    if (get_phero_value(phen, old_x - 1, old_y, y_begin, y_end, ind_pher, ny) == max_phen)
                        new_x -= 1;
                    else if (get_phero_value(phen, old_x + 1, old_y, y_begin, y_end, ind_pher, ny) == max_phen)
                        new_x += 1;
                    else if (get_phero_value(phen, old_x, old_y - 1, y_begin, y_end, ind_pher, ny) == max_phen)
                        new_y -= 1;
                    else
                        new_y += 1;
                }

                consumed_time += land(new_x, new_y);

                ant.x = new_x;
                ant.y_global = new_y;

                if (ant.x == pos_nest.x && ant.y_global == pos_nest.y) {
                    if (ant.state == 1) {
                        food_delta_this_iter += 1;
                    }
                    ant.state = 0;
                }

                if (ant.x == pos_food.x && ant.y_global == pos_food.y) {
                    ant.state = 1;
                }

                int new_owner = owner_of_row(ant.y_global, ny, nprocs);

                if (new_owner == rank) {
                    int yl = global_to_local_y(ant.y_global, y_begin);
                    if (yl >= 1 && yl <= local_ny) {
                        visited_local_cells.emplace_back(ant.x, yl);
                    }
                } else {
                    outgoing[static_cast<std::size_t>(new_owner)].push_back(ant);
                    migrated = true;
                    break;
                }
            }

            if (!migrated) {
                staying.push_back(ant);
            }
        }

        for (const auto& cell : visited_local_cells) {
            phen.mark_from_current(cell.first, cell.second);
        }

        auto tm2 = clock::now();
        double local_t_move = std::chrono::duration<double, std::milli>(tm2 - tm1).count();
        
        auto tg1 = clock::now();

        std::vector<int> sendcounts(nprocs, 0), recvcounts(nprocs, 0);
        for (int p = 0; p < nprocs; ++p) {
            sendcounts[p] = static_cast<int>(outgoing[static_cast<std::size_t>(p)].size());
        }

        MPI_Alltoall(sendcounts.data(), 1, MPI_INT,
                     recvcounts.data(), 1, MPI_INT,
                     MPI_COMM_WORLD);

        std::vector<int> sdispls(nprocs, 0), rdispls(nprocs, 0);
        for (int p = 1; p < nprocs; ++p) {
            sdispls[p] = sdispls[p - 1] + sendcounts[p - 1];
            rdispls[p] = rdispls[p - 1] + recvcounts[p - 1];
        }

        int total_send = 0, total_recv = 0;
        for (int p = 0; p < nprocs; ++p) {
            total_send += sendcounts[p];
            total_recv += recvcounts[p];
        }

        std::vector<AntMPI> sendbuf(static_cast<std::size_t>(total_send));
        std::vector<AntMPI> recvbuf(static_cast<std::size_t>(total_recv));

        for (int p = 0; p < nprocs; ++p) {
            std::copy(outgoing[static_cast<std::size_t>(p)].begin(),
                      outgoing[static_cast<std::size_t>(p)].end(),
                      sendbuf.begin() + sdispls[p]);
        }

        MPI_Alltoallv(sendbuf.data(), sendcounts.data(), sdispls.data(), MPI_ANT,
                      recvbuf.data(), recvcounts.data(), rdispls.data(), MPI_ANT,
                      MPI_COMM_WORLD);

        staying.insert(staying.end(), recvbuf.begin(), recvbuf.end());
        colony.ants.swap(staying);

        auto tg2 = clock::now();
        double local_t_migrate = std::chrono::duration<double, std::milli>(tg2 - tg1).count();

        // ========================================================
        // 4) Évaporation locale + swap + ancrage
        // ========================================================
        auto te1 = clock::now();

        phen.evaporate_buffer();
        phen.swap_buffers();

        if (pos_food.y >= y_begin && pos_food.y < y_end) {
            int yl = global_to_local_y(pos_food.y, y_begin);
            phen.at(pos_food.x, yl).v1 = 1.0;
        }
        if (pos_nest.y >= y_begin && pos_nest.y < y_end) {
            int yl = global_to_local_y(pos_nest.y, y_begin);
            phen.at(pos_nest.x, yl).v2 = 1.0;
        }

        auto te2 = clock::now();
        double local_t_evap = std::chrono::duration<double, std::milli>(te2 - te1).count();

        local_food_quantity += food_delta_this_iter;

        std::size_t global_food_now = 0;
        MPI_Allreduce(&local_food_quantity, &global_food_now, 1,
                      MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);
        global_food_quantity = global_food_now;

        int local_has_food = (food_delta_this_iter > 0) ? it : max_iter + 1;
        int global_first_candidate = max_iter + 1;
        MPI_Allreduce(&local_has_food, &global_first_candidate, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
        if (first_food_iter_global < 0 && global_first_candidate <= max_iter) {
            first_food_iter_global = global_first_candidate;
        }

        double t_border = 0.0, t_move = 0.0, t_migrate = 0.0, t_evap = 0.0;
        MPI_Allreduce(&local_t_border,  &t_border,  1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        MPI_Allreduce(&local_t_move,    &t_move,    1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        MPI_Allreduce(&local_t_migrate, &t_migrate, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        MPI_Allreduce(&local_t_evap,    &t_evap,    1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

        if (rank == 0) {
            csv << it << ","
                << std::fixed << std::setprecision(6)
                << t_border << ","
                << t_move << ","
                << t_migrate << ","
                << t_evap << ","
                << global_food_quantity << "\n";
        }

        stats.sum_border  += t_border;
        stats.sum_move    += t_move;
        stats.sum_migrate += t_migrate;
        stats.sum_evap    += t_evap;

        stats.sum2_border  += t_border * t_border;
        stats.sum2_move    += t_move * t_move;
        stats.sum2_migrate += t_migrate * t_migrate;
        stats.sum2_evap    += t_evap * t_evap;

        stats.count++;
    }

    if (rank == 0) {
        csv.close();
    }

    MPI_Type_free(&MPI_ANT);
    MPI_Finalize();
    return 0;
}
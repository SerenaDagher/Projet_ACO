#ifndef _PHERONOME_LOCAL_HPP_
#define _PHERONOME_LOCAL_HPP_

#include <vector>
#include <array>
#include <algorithm>
#include <cassert>
#include <cstddef>

struct PheroCell
{
    double v1 = 0.0;
    double v2 = 0.0;
};

class LocalPheromoneGrid
{
public:
    LocalPheromoneGrid(int nx, int local_ny, double alpha, double beta)
        : m_nx(nx),
          m_local_ny(local_ny),
          m_alpha(alpha),
          m_beta(beta),
          m_stride(nx),
          m_data((local_ny + 2) * nx),
          m_buffer((local_ny + 2) * nx)
    {}

    int nx() const { return m_nx; }
    int local_ny() const { return m_local_ny; }

    PheroCell& at(int x, int y_local_with_ghost)
    {
        return m_data[y_local_with_ghost * m_stride + x];
    }

    const PheroCell& at(int x, int y_local_with_ghost) const
    {
        return m_data[y_local_with_ghost * m_stride + x];
    }

    PheroCell& buf(int x, int y_local_with_ghost)
    {
        return m_buffer[y_local_with_ghost * m_stride + x];
    }

    const PheroCell& buf(int x, int y_local_with_ghost) const
    {
        return m_buffer[y_local_with_ghost * m_stride + x];
    }

    void copy_data_to_buffer()
    {
        m_buffer = m_data;
    }

    void evaporate_buffer()
    {
        for (int y = 1; y <= m_local_ny; ++y) {
            for (int x = 0; x < m_nx; ++x) {
                buf(x, y).v1 *= m_beta;
                buf(x, y).v2 *= m_beta;
            }
        }
    }

    void mark_from_current(int x, int y_local)
    {
        assert(y_local >= 1 && y_local <= m_local_ny);
        assert(x >= 0 && x < m_nx);

        auto get_v1 = [&](int xx, int yy) -> double {
            if (xx < 0 || xx >= m_nx || yy < 0 || yy > m_local_ny + 1) return -1.0;
            return at(xx, yy).v1;
        };

        auto get_v2 = [&](int xx, int yy) -> double {
            if (xx < 0 || xx >= m_nx || yy < 0 || yy > m_local_ny + 1) return -1.0;
            return at(xx, yy).v2;
        };

        double left_v1  = std::max(get_v1(x - 1, y_local), 0.0);
        double right_v1 = std::max(get_v1(x + 1, y_local), 0.0);
        double up_v1    = std::max(get_v1(x, y_local - 1), 0.0);
        double down_v1  = std::max(get_v1(x, y_local + 1), 0.0);

        double left_v2  = std::max(get_v2(x - 1, y_local), 0.0);
        double right_v2 = std::max(get_v2(x + 1, y_local), 0.0);
        double up_v2    = std::max(get_v2(x, y_local - 1), 0.0);
        double down_v2  = std::max(get_v2(x, y_local + 1), 0.0);

        buf(x, y_local).v1 =
            m_alpha * std::max({left_v1, right_v1, up_v1, down_v1}) +
            (1.0 - m_alpha) * 0.25 * (left_v1 + right_v1 + up_v1 + down_v1);

        buf(x, y_local).v2 =
            m_alpha * std::max({left_v2, right_v2, up_v2, down_v2}) +
            (1.0 - m_alpha) * 0.25 * (left_v2 + right_v2 + up_v2 + down_v2);
    }

    void swap_buffers()
    {
        m_data.swap(m_buffer);
    }

    void pack_row(int y_local, std::vector<double>& out) const
    {
        out.resize(static_cast<std::size_t>(2 * m_nx));
        for (int x = 0; x < m_nx; ++x) {
            out[2 * x]     = at(x, y_local).v1;
            out[2 * x + 1] = at(x, y_local).v2;
        }
    }

    void unpack_row(int y_local, const std::vector<double>& in)
    {
        for (int x = 0; x < m_nx; ++x) {
            at(x, y_local).v1 = in[2 * x];
            at(x, y_local).v2 = in[2 * x + 1];
        }
    }

private:
    int m_nx;
    int m_local_ny;
    double m_alpha;
    double m_beta;
    int m_stride;
    std::vector<PheroCell> m_data;
    std::vector<PheroCell> m_buffer;
};

#endif
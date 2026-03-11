#ifndef _ANT_HPP_
#define _ANT_HPP_

#include <vector>
#include <cstdint>

struct AntMPI
{
    int x;
    int y_global;
    int state;
    std::uint64_t seed;
};

struct AntColonyMPI
{
    std::vector<AntMPI> ants;
};

#endif
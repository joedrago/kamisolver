#include "Puzzle.h"


#pragma once

#include <vector>

class Puzzle;
class Move;

class Solver
{
public:

    struct Move
    {
        int x;
        int y;
        char color;

        Move() {}
        Move(int xx, int yy, int c) : x(xx), y(yy), color(c) {}
    };

    struct Metrics
    {
        int copies;
    };

    Solver();
    bool solve(Puzzle const & puzzle, int limit, std::vector<Move> & moves);
    Metrics const & metrics() const { return metrics_;  }

private:

    int prioritize(int nodeCount);

    Metrics metrics_;
};



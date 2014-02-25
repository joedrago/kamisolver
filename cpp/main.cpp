#include "Solver.h"
#include <cstdlib>
#include <cstdio>

static char const * label(int color);

int main(int argc, char **argv)
{
    if ((argc < 3) || (argc > 4))
    {
        printf("Syntax: kamisolver filename turns [verboseDepth]");
        return 1;
    }

    char const * filename = argv[1];
    int turns = atoi(argv[2]);
    int verboseDepth = (argc > 3) ? atoi(argv[3]) : 0;

    Puzzle * puzzle = Puzzle::create(filename);
    if (!puzzle)
    {
        return 1;
    }

    std::vector<Solver::Move> moves;
    moves.reserve(turns);

    Solver solver;
    if (solver.solve(*puzzle, turns, moves))
    {
        Solver::Metrics metrics = solver.metrics();
        printf("Solved with %d turns in %d attempts!\n", moves.size(), metrics.copies);
        for (unsigned i = 0; i < moves.size(); ++i)
        {
            Solver::Move const & move = moves[i];
            printf("%2d: %-7s (%d,%d)\n", i + 1, label(move.color), move.x, move.y);
        }
    }
    else
    {
        printf("Cannot be solved with %d turns!\n", turns);
    }

    delete puzzle;

    return 0;
}

static char const * label(int color)
{
    switch (color + 'A')
    {
    case 'R': return "Red";
    case 'G': return "Green";
    case 'B': return "Blue";
    case 'K': return "Black";
    case 'O': return "Orange";
    case 'W': return "White";
    case 'N': return "Brown";
    case 'Y': return "Yellow";
    }
    return "Unknown";
}



#include "Solver.h"
#include "Puzzle.h"
#include "Node.h"

#include <memory>
#include <vector>
#include <cstdlib>

#define DEBUGGING 0
#if !DEBUGGING
#define printf(...)
#endif

struct Try
{
    Solver::Move move;                      // The record of the move
    int priority;                   // Priority of the try
    std::shared_ptr<Node> node;     // The node being changed

    Try(Solver::Move const & m, int p, std::shared_ptr<Node> const & n) : move(m), priority(p), node(n) {}
    static bool sortByPriority(Try const & a, Try const & b)
    {
        return (a.priority > b.priority);  // Sort highest to lowest
    }
};

static int indentCount = 0;
#if DEBUGGING
char * indent()
{
    static char buffer[256];
    static int const INDENT_SIZE = 4;
    memset(buffer, ' ', indentCount*INDENT_SIZE);
    buffer[indentCount*INDENT_SIZE] = 0;
    return buffer;
}
#endif

Solver::Solver()
{
    metrics_.copies = 0;
}

bool Solver::solve(Puzzle const & puzzle, int limit, std::vector<Move> & moves)
{
    Puzzle::NodeList const & nodes = puzzle.nodes();

    // If there is 1 node, the puzzle is solved
    if (nodes.size() == 0)
    {
        return true;
    }

    // If there are not enough moves to remove all but 1 color, then the puzzle cannot be solved
    if (limit < puzzle.colors() - 1)
    {
        return false;
    }

    // Generate a try for each color of node adjacent to each node

    std::vector<Try> tries;

    for (Puzzle::NodeList::const_iterator n = nodes.begin(); n != nodes.end(); ++n)
    {
        int nodesByColor[Node::MAX_COLORS];
        memset(nodesByColor, 0, sizeof(nodesByColor));

        std::shared_ptr<Node> node = n->lock();

        // Find all adjacent colors
        Node::NodeList adjacentNodes = node->adjacentNodes();
        for (Node::NodeList::iterator a = adjacentNodes.begin(); a != adjacentNodes.end(); ++a)
        {
            std::shared_ptr<Node> & adjacent = *a;
            ++nodesByColor[adjacent->color()];
        }

        // Add a try for each color adjacent to this node

        for (int i = 0; i < Node::MAX_COLORS; ++i)
        {
            if (nodesByColor[i] > 0)
            {
                int priority = prioritize(nodesByColor[i]);
                tries.push_back(Try(Move(node->x(), node->y(), i), priority, node));
            }
        }
    }

    // Sort by priority (highest to lowest)

    std::sort(tries.begin(), tries.end(), Try::sortByPriority);

    // Try each try

    for (std::vector<Try>::iterator t = tries.begin(); t != tries.end(); ++t)
    {
        moves.push_back(t->move);
        printf("%s%2d: Trying %c at (%d,%d)\n", indent(), moves.size(), t->move.color + 'A', t->move.x, t->move.y);
        ++indentCount;
        ++metrics_.copies;
        Puzzle child(puzzle, t->node, t->move.color);
        bool solved = solve(child, limit - 1, moves);  // recursive
        printf("%s%s\n", indent(), solved ? "Solved" : "Failed");
        --indentCount;
        if (solved)
        {
            return true;
        }

        moves.resize(moves.size() - 1); // That didn't work. Forget that move.
    }

    return false;
}

int Solver::prioritize(int nodeCount)
{
    return nodeCount;
}
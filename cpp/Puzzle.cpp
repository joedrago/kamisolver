#include "Puzzle.h"

#include "Node.h"

#include <algorithm>
#include <functional>
#include <string>
#include <memory>

#define DEBUGGING 0
#if !DEBUGGING
#define printf(...)
#endif

Puzzle::Puzzle(char const * grid)
{
    int colors[Node::MAX_COLORS];
    memset(colors, 0, sizeof(colors));

    // Create the nodes

    {
        std::vector<std::shared_ptr<Node> > previousRow(WIDTH); // Note: it is important that this goes out of scope so that nodes delete themselves
        int id = 0;

        for (int r = 0; r < HEIGHT; ++r)
        {
            std::shared_ptr<Node> previousNode;

            for (int c = 0; c < WIDTH; ++c)
            {
                int color = grid[r*WIDTH + c];

                std::shared_ptr<Node> newNode = std::shared_ptr<Node>(new Node(c, r, color, id++));
                if (previousNode)
                {
                    connect(newNode, previousNode);
                }
                if (previousRow[c])
                {
                    connect(newNode, previousRow[c]);
                }
                previousNode = newNode;
                previousRow[c] = newNode;
                add(newNode);

                ++colors[color];    // Bump the count for this color
            }
        }
    }

    // Normalize the graph (for each node, remove all adjacent nodes with the same color)

    normalize();

    // Determine all the colors in the puzzle

    countColors();
}

Puzzle::Puzzle(Puzzle const & parent, std::shared_ptr<Node> const & node, int color)
{
    *this = parent;

    // Apply the move (the node in the parent puzzle should still point to the node in this puzzle)
    {
        std::shared_ptr<Node> nodeToChange = node->clone(); // Note: it is important that this goes out of scope so that nodes delete themselves
        nodeToChange->changeColor(color);
    }
    removeCoalescedNodes();
    countColors();
}

Puzzle::Puzzle(Puzzle const & src)
{
    *this = src;
}

Puzzle & Puzzle::operator =(Puzzle const & src)
{
    if (this != &src)
    {
        // Make copies of the nodes
        std::vector<std::shared_ptr<Node> > newNodes;
        newNodes.reserve(src.nodes_.size());
        for (NodeList::const_iterator i = src.nodes_.begin(); i != src.nodes_.end(); ++i)
        {
            newNodes.push_back(i->lock()->clone());
        }

        // Link them up
        nodes_.clear();
        nodes_.reserve(src.nodes_.size());
        for (NodeList::const_iterator i = src.nodes_.begin(); i != src.nodes_.end(); ++i)
        {
            std::shared_ptr<Node> node = i->lock();
            std::shared_ptr<Node> newNode = node->clone();
            Node::NodeList const & adjacentNodes = node->adjacentNodes();
            for (Node::NodeList::const_iterator a = adjacentNodes.begin(); a != adjacentNodes.end(); ++a)
            {
                std::shared_ptr<Node> aNode = *a;
                newNode->connect(aNode->clone());
            }
            nodes_.push_back(newNode);
        }
        colors_ = src.colors_;
    }
    return *this;
}

Puzzle::~Puzzle()
{
    for (NodeList::iterator i = nodes_.begin(); i != nodes_.end(); ++i)
    {
        std::shared_ptr<Node> node = i->lock();
        if (node)
        {
            node->release();
        }
    }
}

void Puzzle::add(std::shared_ptr<Node> & node)
{
    nodes_.push_back(node);
}

void Puzzle::connect(std::shared_ptr<Node> & a, std::shared_ptr<Node> & b)
{
    a->connect(b);
    b->connect(a);
}


int Puzzle::countColors()
{
    int seen[Node::MAX_COLORS];
    memset(seen, 0, sizeof(seen));

    for (NodeList::iterator i = nodes_.begin(); i != nodes_.end(); ++i)
    {
        std::shared_ptr<Node> node = i->lock();
        ++seen[node->color()];
    }

    colors_ = 0;
    for (int c = 0; c < Node::MAX_COLORS; ++c)
    {
        if (seen[c] > 0)
        {
            ++colors_;
        }
    }

    return colors_;
}

void Puzzle::normalize()
{
    for (NodeList::iterator i = nodes_.begin(); i != nodes_.end(); ++i)
    {
        std::shared_ptr<Node> shared = i->lock();
        if (shared)
        {
            shared->coalesce();
        }
    }
    removeCoalescedNodes();
}

void Puzzle::removeCoalescedNodes()
{
    nodes_.erase(std::remove_if(nodes_.begin(), nodes_.end(), std::mem_fun_ref(&std::weak_ptr<Node>::expired)), nodes_.end());
}

Puzzle * Puzzle::create(char const * filename)
{

    FILE * f = fopen(filename, "r");
    if (f == NULL)
    {
        printf("Can't open '%s'\n", filename);
        return NULL;
    }

    printf("Reading %s\n", filename);

    char lineBuffer[256];

    // Load the puzzle

    char * grid = new char[WIDTH*HEIGHT];
    for (int i = 0; i < HEIGHT; ++i)
    {
        fgets(lineBuffer, sizeof(lineBuffer), f);
        for (int j = 0; j < WIDTH; ++j)
        {
            grid[i*WIDTH + j] = lineBuffer[j] - 'A';
        }
    }

    fclose(f);

    // Offset colors

    Puzzle * puzzle = new Puzzle(grid);

    puzzle->dump((std::string(filename) + ".dot").c_str());

    return puzzle;
}

void Puzzle::dump(char const * filename)
{
    FILE *f = fopen(filename, "w");
    if (f)
    {
        fprintf(f, "graph G {\n");
        fprintf(f, "overlap=false;\n");

        for (NodeList::iterator i = nodes_.begin(); i != nodes_.end(); ++i)
        {
            std::shared_ptr<Node> node = i->lock();
            node->dump(f, WIDTH, HEIGHT);
        }

        fprintf(f, "}\n");
        fclose(f);
    }
}



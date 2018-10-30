#include "Node.h"

#define DEBUGGING 0
#if !DEBUGGING
#define printf(...)
#endif

extern char * indent();

static void colorToHex(int color, char const *& fillColor, char const *& textColor);

Node::Node(int x, int y, int color, int id)
: x_(x)
, y_(y)
, color_(color)
, id_(id)
{
    printf("%sN%p: new %c at (%d,%d)\n", indent(), this, color_ + 'A', x_, y_);
}

Node::~Node()
{
    for (NodeList::iterator i = adjacentNodes_.begin(); i != adjacentNodes_.end(); ++i)
    {
        i->reset();
    }

    printf("%sN%p: deleted %c at (%d,%d)\n", indent(), this, color_ + 'A', x_, y_);
}

void Node::connect(std::shared_ptr<Node> const & other)
{
    adjacentNodes_.push_back(other);
}


void  Node::disconnect(std::shared_ptr<Node> const & other)
{
    // Remove the other node from the adjacent nodes list
    // Note: faster than remove()
    NodeList::iterator i = std::find(adjacentNodes_.begin(), adjacentNodes_.end(), other);
    if (i != adjacentNodes_.end())
    {
        // Swap the node to erase with the last node and then drop the last node
        std::swap(*i, adjacentNodes_.back());
        adjacentNodes_.resize(adjacentNodes_.size() - 1);
    }
}

void Node::release()
{
    adjacentNodes_.clear();
}

bool Node::connectedTo(std::shared_ptr<Node> const & node) const
{
    return std::find(adjacentNodes_.begin(), adjacentNodes_.end(), node) != adjacentNodes_.end();
}

std::shared_ptr<Node> Node::clone() const
{
    std::shared_ptr<Node> shared = clone_.lock();
    if (!shared)
    {
        shared = std::shared_ptr<Node>(new Node(x_, y_, color_, id_));
        clone_ = shared;
    }
    return shared;
}

void Node::changeColor(int color)
{
    printf("%sN%08x: Changing %c at (%d,%d) to %c\n", indent(), this, color_ + 'A', x_, y_, color + 'A');
    color_ = color;
    coalesce();
}

void Node::coalesce()
{
    printf("%sCoalescing %c at (%d,%d)\n", indent(), color() + 'A', x(), y());
    std::shared_ptr<Node> sharedThis = shared_from_this();

    NodeList adjacents = adjacentNodes_; // Copy the list so we can modify it as we iterate through it
    for (NodeList::iterator i = adjacents.begin(); i != adjacents.end(); ++i)
    {
        std::shared_ptr<Node> const & aNode = *i;
        if (color_ == aNode->color())
        {
            // Disconnect the adjacent node from everyone and connect this node to its adjacent nodes
            for (NodeList::iterator i2 = aNode->adjacentNodes_.begin(); i2 != aNode->adjacentNodes_.end(); ++i2)
            {
                std::shared_ptr<Node> const & aaNode = *i2;
                printf("%sDisconnecting %c at (%d,%d) from %c at (%d,%d)\n",
                    indent(),
                    aNode->color() + 'A',
                    aNode->x(),
                    aNode->y(),
                    aaNode->color() + 'A',
                    aaNode->x(),
                    aaNode->y());

                // Disconnect the adjacent-adjacent node (which might be this node) from the adjacent node
                aaNode->disconnect(aNode);

                // If the adjacent-adjacent node is not this node and is not already connected to this node,
                // then connect it
                if (aaNode.get() != this && !connectedTo(aaNode))
                {
                    printf("%sConnecting %c at (%d,%d) to %c at (%d,%d)\n",
                        indent(),
                        aaNode->color() + 'A',
                        aaNode->x(),
                        aaNode->y(),
                        color() + 'A',
                        x(),
                        y());
                    connect(aaNode);
                    aaNode->connect(sharedThis);
                }
            }
            // At this point, the adjacent node is only referenced by the copied list and will be deleted when
            // the list goes out of scope
        }
    }
}

void Node::dump(FILE * f, int width, int height)
{
    char const * fillColor;
    char const * textColor;

    colorToHex(color_, fillColor, textColor);
    fprintf(f, "N%p [style=filled; fillcolor=\"%s\"; fontcolor=\"%s\"; label=\"%d, %d\"; pos=\"%f,%f\" ];\n",
        this,
        fillColor,
        textColor,
        x_,
        y_,
        x_*0.5f,
        (height - 1 - y_)*0.5f);
    for (NodeList::iterator i = adjacentNodes_.begin(); i != adjacentNodes_.end(); ++i)
    {
        std::shared_ptr<Node> const & node = *i;
        if (id() <= node->id())
        {
            fprintf(f, "N%p -- N%p; ", this, node.get());
        }
    }
    fprintf(f, "\n");
}

static void colorToHex(int color, char const *& fillColor, char const *& textColor)
{
    switch (color + 'A')
    {
    case 'R':
        fillColor = "#FF8080";
        textColor = "#000000";
        break;
    case 'G':
        fillColor = "#70E070";
        textColor = "#000000";
        break;
    case 'B':
        fillColor = "#8080ff";
        textColor = "#000000";
        break;
    case 'K':
        fillColor = "#606060";
        textColor = "#ffffff";
        break;
    case 'O':
        fillColor = "#FFA080";
        textColor = "#000000";
        break;
    case 'W':
        fillColor = "#f8f8f8";
        textColor = "#000000";
        break;
    case 'N':
        fillColor = "#805040";
        textColor = "#ffffff";
        break;
    case 'Y':
        fillColor = "#F0F078";
        textColor = "#000000";
        break;
    default:
        fillColor = "#FF00FF";
        textColor = "#000000";
        break;
    }
}

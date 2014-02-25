#pragma once

#include <vector>
#include <memory>

class Node;

class Puzzle
{
public:

    typedef std::vector< std::weak_ptr<Node> > NodeList;
    static int const WIDTH = 16;
    static int const HEIGHT = 10;

    static Puzzle * create(char const * filename);

    Puzzle(char const * grid);
    Puzzle(Puzzle const & parent, std::shared_ptr<Node> const & node, int color);
    Puzzle(Puzzle const &);
    Puzzle & operator =(Puzzle const &);
    virtual ~Puzzle();

    void add(std::shared_ptr<Node> & node);
    void connect(std::shared_ptr<Node> & a, std::shared_ptr<Node> & b);

    NodeList const & nodes() const  { return nodes_; }
    int colors() const              { return colors_; }

    void dump(char const * filename);

private:

    int countColors();
    void normalize();
    void removeCoalescedNodes();

    NodeList nodes_;
    int colors_;
};

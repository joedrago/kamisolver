#pragma once

#include <memory>
#include <functional>
#include <algorithm>
#include <vector>
#include <stdio.h>

class Node : public std::enable_shared_from_this<Node>
{
public:

    typedef std::vector<std::shared_ptr<Node> > NodeList;
    static int const MAX_COLORS = 26;

    Node(int x, int y, int color, int id);
    ~Node();

    void connect(std::shared_ptr<Node> const & other);
    void disconnect(std::shared_ptr<Node> const & other);
    void release();
    bool connectedTo(std::shared_ptr<Node> const & node) const;

    void changeColor(int color);
    void coalesce();

    int x() const                           { return x_; }
    int y() const                           { return y_; }
    int color() const                       { return color_; }
    int id() const                          { return id_; }
    NodeList const & adjacentNodes() const  { return adjacentNodes_; }
    std::shared_ptr<Node> clone() const;

    void dump(FILE * f, int width, int height);

private:

    // noncopyable
    Node(Node const & src);
    Node & operator =(Node const &);

    int x_;
    int y_;
    int color_;
    int id_;
    NodeList adjacentNodes_;
    mutable std::weak_ptr<Node> clone_;
};

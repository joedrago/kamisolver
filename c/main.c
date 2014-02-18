#include "dyn.h"
#include <stdlib.h>
#include <stdio.h>

// -----------------------------------------------------------------------
// Node

typedef struct Node
{
    int id;
    char x;
    char y;
    char color;
    int *connections;
} Node;

Node *nodeCreate(int id, char x, char y, char color)
{
    Node *node = (Node *)calloc(1, sizeof(Node));
    node->id = id;
    node->x = x;
    node->y = y;
    node->color = color;
    daCreate(&node->connections, sizeof(int));
    return node;
}

void nodeDestroy(Node *node)
{
    daDestroy(&node->connections, NULL);
    free(node);
}

// -----------------------------------------------------------------------
// Solver

typedef struct Solver
{
    char *filename;
    int verboseDepth;
    Node **nodes;
} Solver;

Solver *solverCreate(const char *filename, int verboseDepth)
{
    Solver *solver = (Solver *)calloc(1, sizeof(Solver));
    dsCopy(&solver->filename, filename);
    solver->verboseDepth = verboseDepth;
    return solver;
}

void solverDestroy(Solver *solver)
{
    dsDestroy(&solver->filename);
    free(solver);
}

// -----------------------------------------------------------------------

int main(int argc, char **argv)
{
    return 0;
}
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "dyn.h"

#ifdef WIN32
#include <windows.h>
#endif

// -----------------------------------------------------------------------
// Helpers

unsigned int elapsedMS(unsigned int *lastMS)
{
#ifdef WIN32
    unsigned int now = GetTickCount();
#else
#error please define now in milliseconds here
#endif
    unsigned int elapsed;
    if(*lastMS == 0)
    {
        *lastMS = now;
        return 0;
    }
    elapsed = now - *lastMS;
    *lastMS = now;
    return elapsed;
}

const char *colorToLabel(char color)
{
    switch(color)
    {
        case 'R': return "Red";
        case 'G': return "Green";
        case 'B': return "Blue";
        case 'K': return "Black";
        case 'O': return "Orange";
        case 'W': return "White";
        case 'N': return "Brown";
        case 'Y': return "Yellow";
    };
    return "Unknown";
}

void colorToHex(char color, const char **fillColor, const char **textColor)
{
    switch(color)
    {
        case 'R':
            *fillColor = "#CA3736";
            *textColor = "#000000";
            break;
        case 'G':
            *fillColor = "#52C1A5";
            *textColor = "#000000";
            break;
        case 'B':
            *fillColor = "#0000ff";
            *textColor = "#000000";
            break;
        case 'K':
            *fillColor = "#1E140F";
            *textColor = "#ffffff";
            break;
        case 'O':
            *fillColor = "#ED6C3A";
            *textColor = "#000000";
            break;
        case 'W':
            *fillColor = "#ffffff";
            *textColor = "#000000";
            break;
        case 'N':
            *fillColor = "#9B7065";
            *textColor = "#ffffff";
            break;
        case 'Y':
            *fillColor = "#FFFF00";
            *textColor = "#000000";
            break;
        default:
            *fillColor = "#000000";
            *textColor = "#ffffff";
            break;
    }
}

// -----------------------------------------------------------------------
// Node

#define MAX_CONNECTIONS 160

typedef struct Node
{
    int id;
    char x;
    char y;
    char color;
    int connections[MAX_CONNECTIONS];
    int connectionCount;
} Node;

Node *nodeCreate(int id, char x, char y, char color)
{
    Node *node = (Node *)calloc(1, sizeof(Node));
    node->id = id;
    node->x = x;
    node->y = y;
    node->color = color;
    node->connectionCount = 0;
    return node;
}

void nodeDestroy(Node *node)
{
    free(node);
}

// -----------------------------------------------------------------------
// NodeList

typedef struct NodeList
{
    Node **nodes;
} NodeList;

NodeList *nodeListCreate()
{
    NodeList *nodeList = (NodeList *)calloc(1, sizeof(NodeList));
    return nodeList;
}

void nodeListDestroy(NodeList *nodeList)
{
    daDestroy(&nodeList->nodes, nodeDestroy);
    free(nodeList);
}

void nodeListAdd(NodeList *nodeList, Node *node)
{
    daPush(&nodeList->nodes, node);
}

Node *nodeListGet(NodeList *nodeList, int id, int *retIndex)
{
    int i;
    for(i = 0; i < daSize(&nodeList->nodes); ++i)
    {
        if(nodeList->nodes[i] && nodeList->nodes[i]->id == id)
        {
            if(retIndex)
                *retIndex = i;
            return nodeList->nodes[i];
        }
    }
    return NULL;
}

void nodeListConnect(NodeList *nodeList, Node *node, int id)
{
    Node *otherNode = nodeListGet(nodeList, id, NULL);
    int i;

    // nodeList doesn't track this ID, forget it
    if(!otherNode)
        return;

    // Don't bother if we're already connected
    for(i = 0; i < node->connectionCount; ++i)
    {
        if(node->connections[i] == id)
            return;
    }

    // Add connections
    node->connections[node->connectionCount++] = otherNode->id;
    otherNode->connections[otherNode->connectionCount++] = node->id;
}

void nodeListDisconnect(NodeList *nodeList, Node *node, int id)
{
    Node *otherNode = nodeListGet(nodeList, id, NULL);
    int foundIndex = -1;
    int otherFoundIndex = -1;
    int i;

    // nodeList doesn't track this ID, forget it
    if(!otherNode)
        return;

    // Don't bother if we're already disconnected
    for(i = 0; i < node->connectionCount; ++i)
    {
        if(node->connections[i] == id)
        {
            foundIndex = i;
            break;
        }
    }

    for(i = 0; i < otherNode->connectionCount; ++i)
    {
        if(otherNode->connections[i] == node->id)
        {
            otherFoundIndex = i;
            break;
        }
    }

    // Remove connections
    if(foundIndex != -1)
    {
        --node->connectionCount;
        memmove(&node->connections[foundIndex], &node->connections[foundIndex+1], sizeof(int) * (node->connectionCount - foundIndex));
    }
    if(otherFoundIndex != -1)
    {
        --otherNode->connectionCount;
        memmove(&otherNode->connections[otherFoundIndex], &otherNode->connections[otherFoundIndex+1], sizeof(int) * (otherNode->connectionCount - otherFoundIndex));
    }
}

NodeList *nodeListClone(NodeList *nodeList)
{
    NodeList *clonedList = nodeListCreate();
    int nodeIndex;
    for(nodeIndex = 0; nodeIndex < daSize(&nodeList->nodes); ++nodeIndex)
    {
        Node *node = nodeList->nodes[nodeIndex];
        Node *clonedNode = nodeCreate(node->id, node->x, node->y, node->color);
        clonedNode->connectionCount = node->connectionCount;
        memcpy(clonedNode->connections, node->connections, clonedNode->connectionCount * sizeof(int));
        nodeListAdd(clonedList, clonedNode);
    }
    return clonedList;
}

void nodeListConsume(NodeList *nodeList, Node *node, Node *eatme)
{
    int connIndex;
    int tempConnections[160];
    int connectionCount = eatme->connectionCount;
    if(connectionCount == 0)
        return;
    memcpy(tempConnections, eatme->connections, connectionCount * sizeof(int));
    for(connIndex = 0; connIndex < connectionCount; ++connIndex)
    {
        int connID = tempConnections[connIndex];
        nodeListDisconnect(nodeList, eatme, connID);
        if(node->id != connID)
        {
            nodeListConnect(nodeList, node, connID);
        }
    }
}

void nodeListCoalesce(NodeList *nodeList)
{
    int nodeIndex;
    int tempConnections[160];
    for(nodeIndex = 0; nodeIndex < daSize(&nodeList->nodes); ++nodeIndex)
    {
        Node *node = nodeList->nodes[nodeIndex];
        int connIndex;
        int keepEating = 1;
        if(!node)
            continue;
        while(keepEating)
        {
            int connectionCount = node->connectionCount;
            if(connectionCount == 0)
                break;
            memcpy(tempConnections, node->connections, connectionCount * sizeof(int));

            keepEating = 0;
            for(connIndex = 0; connIndex < connectionCount; ++connIndex)
            {
                int connID = tempConnections[connIndex];
                int globalConnIndex;
                Node *connNode = nodeListGet(nodeList, connID, &globalConnIndex);
                if(!connNode)
                    continue;
                if((node->id < connID) && (node->color == connNode->color))
                {
                    keepEating = 1;
                    nodeListConsume(nodeList, node, connNode);
                    nodeDestroy(connNode);
                    nodeList->nodes[globalConnIndex] = NULL;
                }
            }
        }
    }
    daSquash(&nodeList->nodes);
}

int nodeListCountColors(NodeList *nodeList)
{
    char colorSeen[26] = {0};
    int nodeIndex;
    int colorCount = 0;
    int i;
    for(nodeIndex = 0; nodeIndex < daSize(&nodeList->nodes); ++nodeIndex)
    {
        Node *node = nodeList->nodes[nodeIndex];
        colorSeen[node->color - 'A'] = 1;
    }
    for(i = 0; i < 26; ++i)
    {
        if(colorSeen[i])
            ++colorCount;
    }
    return colorCount;
}

int nodeListAdjacent(NodeList *nodeList, Node *node, char color)
{
    int connIndex;
    for(connIndex = 0; connIndex < node->connectionCount; ++connIndex)
    {
        Node *connNode = nodeListGet(nodeList, node->connections[connIndex], NULL);
        if(connNode && (connNode->color == color))
            return 1;
    }
    return 0;
}

void nodeListDump(NodeList *nodeList, const char *filename)
{
    FILE *f = fopen(filename, "w");
    if(f)
    {
        int nodeIndex;
        int connIndex;
        const char *fillColor;
        const char *textColor;

        fprintf(f, "graph G {\n");
        fprintf(f, "overlap=false;\n");

        for(nodeIndex = 0; nodeIndex < daSize(&nodeList->nodes); ++nodeIndex)
        {
            Node *node = nodeList->nodes[nodeIndex];
            colorToHex(node->color, &fillColor, &textColor);
            fprintf(f, "N%d [style=filled; fillcolor=\"%s\"; fontcolor=\"%s\"; label=\"%d, %d\" ];\n",
                node->id,
                fillColor,
                textColor,
                node->x,
                node->y);

            for(connIndex = 0; connIndex < node->connectionCount; ++connIndex)
            {
                int connID = node->connections[connIndex];
                if(connID > node->id)
                {
                    fprintf(f, "N%d -- N%d;\n", node->id, connID);
                }
            }
        }

        fprintf(f, "}\n");
        fclose(f);
    }
}

// -----------------------------------------------------------------------
// Solver

typedef struct Move
{
    int x;
    int y;
    char color;
} Move;

Move *moveCreate(int x, int y, char color)
{
    Move *move = (Move *)calloc(1, sizeof(Move));
    move->x = x;
    move->y = y;
    move->color = color;
    return move;
}

void moveDestroy(Move *move)
{
    free(move);
}

// -----------------------------------------------------------------------
// Solver

typedef struct Solver
{
    char *filename;
    int verboseDepth;
    NodeList *nodeList;
    Move **moves;
    char *colors;
    int totalMoves;
    int attempts;
    unsigned int timer;
} Solver;

Solver *solverDestroy(Solver *solver);

Solver *solverCreate(const char *filename, int verboseDepth)
{
    Solver *solver = (Solver *)calloc(1, sizeof(Solver));
    dsCopy(&solver->filename, filename);
    solver->verboseDepth = verboseDepth;
    daCreate(&solver->colors, sizeof(char));
    solver->nodeList = nodeListCreate();

    printf("Reading %s\n", solver->filename);
    {
        int i;
        Node *node;
        int currentID = 1;
        int lineNo = 0;
        FILE *f = fopen(solver->filename, "r");
        char lineBuffer[128];
        char colorSeen[26] = {0};
        int lineNodes[16] = {0};
        int prevNode = 0;
        if(!f)
            return solverDestroy(solver);

        while(fgets(lineBuffer, sizeof(lineBuffer), f))
        {
            int len = strlen(lineBuffer);
            if(len < 16)
                continue;

            prevNode = 0;
            for(i = 0; i < 16; ++i)
            {
                char color = lineBuffer[i];
                if((color < 'A') || (color > 'Z'))
                    return solverDestroy(solver);

                colorSeen[color - 'A'] = 1;
                node = nodeCreate(currentID++, i, lineNo, color);
                nodeListAdd(solver->nodeList, node);
                if(prevNode)
                    nodeListConnect(solver->nodeList, node, prevNode);
                if(lineNodes[i])
                    nodeListConnect(solver->nodeList, node, lineNodes[i]);
                prevNode = node->id;
                lineNodes[i] = node->id;
            }
            lineNo++;
        }

        fclose(f);

        for(i = 0; i < 26; ++i)
        {
            if(colorSeen[i])
                daPushU8(&solver->colors, i + 'A');
        }

        nodeListCoalesce(solver->nodeList);
    }

    return solver;
}

Solver *solverDestroy(Solver *solver)
{
    dsDestroy(&solver->filename);
    nodeListDestroy(solver->nodeList);
    daDestroy(&solver->colors, NULL);
    daDestroy(&solver->moves, moveDestroy);
    free(solver);
    return NULL;
}

int solverRecursiveSolve(Solver *solver, NodeList *nodeList, int remainingMoves)
{
    int colorCount;
    int depth = solver->totalMoves - remainingMoves;
    int nodeIndex;
    int colorIndex;
    int x, y;
    int solved;
    NodeList *clonedList;
    Node *clonedNode;

    if(daSize(&nodeList->nodes) == 1)
        return 1;

    if(remainingMoves < 0)
        return 0;

    colorCount = nodeListCountColors(nodeList);
    if((colorCount - 1) > remainingMoves)
        return 0;

    for(nodeIndex = 0; nodeIndex < daSize(&nodeList->nodes); ++nodeIndex)
    {
        Node *node = nodeList->nodes[nodeIndex];
        if(depth <= solver->verboseDepth)
        {
            int i;
            for(i = 0; i < depth; ++i)
                printf("  ");
            printf("%d: (%d / %d)\n", depth, nodeIndex+1, (int)daSize(&nodeList->nodes));
        }
        for(colorIndex = 0; colorIndex < daSize(&solver->colors); ++colorIndex)
        {
            char color = solver->colors[colorIndex];
            if((node->color == color) || (!nodeListAdjacent(nodeList, node, color)))
                continue;

            ++solver->attempts;
            if((solver->attempts % 100000) == 0)
                printf("                                                              attempts: %d (%ums)\n", solver->attempts, elapsedMS(&solver->timer));

            clonedList = nodeListClone(nodeList);
            clonedNode = clonedList->nodes[nodeIndex];
            x = clonedNode->x;
            y = clonedNode->y;
            clonedNode->color = color;
            nodeListCoalesce(clonedList);
            solved = solverRecursiveSolve(solver, clonedList, remainingMoves - 1);
            nodeListDestroy(clonedList);

            if(solved)
            {
                Move *move = moveCreate(x, y, color);
                daUnshift(&solver->moves, move);
                return 1;
            }
        }
    }
    return 0;
}

void solverSolve(Solver *solver, int steps)
{
    printf("Node count: %d\n", (int)daSize(&solver->nodeList->nodes));
    //nodeListDump(solver->nodeList, "out.txt");

    solver->attempts = 0;
    solver->totalMoves = steps;
    elapsedMS(&solver->timer);
    if(solverRecursiveSolve(solver, solver->nodeList, steps))
    {
        int moveIndex;
        for(moveIndex = 0; moveIndex < daSize(&solver->moves); ++moveIndex)
        {
            Move *move = solver->moves[moveIndex];
            printf("MOVE %d: change (%d,%d) to %s\n", moveIndex+1, move->x, move->y, colorToLabel(move->color));
        }
    }
}

// -----------------------------------------------------------------------

int main(int argc, char **argv)
{
    if((argc < 3) || (argc > 4))
    {
        printf("Syntax: kamisolver filename steps [verboseDepth]");
        return 1;
    }
    else
    {
        const char *filename = argv[1];
        int steps = atoi(argv[2]);
        int verboseDepth = 0;
        Solver *solver;
        if(argc > 3)
        {
            verboseDepth = atoi(argv[3]);
        }
        solver = solverCreate(filename, verboseDepth);
        if(solver)
        {
            solverSolve(solver, steps);
            solverDestroy(solver);
        }
    }
    return 0;
}
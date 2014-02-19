#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <string.h>
#include <stdio.h>

#include "dyn.h"

// -----------------------------------------------------------------------
// Helpers

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
    printf("adding node %d to nodelist\n", node->id);
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
    for(i = 0; i < daSize(&node->connections); ++i)
    {
        if(node->connections[i] == id)
            return;
    }

    // Add connections
    daPushU32(&node->connections, otherNode->id);
    daPushU32(&otherNode->connections, node->id);
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
    for(i = 0; i < daSize(&node->connections); ++i)
    {
        if(node->connections[i] == id)
        {
            foundIndex = i;
            break;
        }
    }

    for(i = 0; i < daSize(&otherNode->connections); ++i)
    {
        if(otherNode->connections[i] == node->id)
        {
            otherFoundIndex = i;
            break;
        }
    }

    // Remove connections
    if(foundIndex != -1)
        daErase(&node->connections, foundIndex);
    if(otherFoundIndex != -1)
        daErase(&otherNode->connections, otherFoundIndex);
}

void nodeListConsume(NodeList *nodeList, Node *node, Node *eatme)
{
    int connIndex;
    int tempConnections[160];
    int connectionSize = daSize(&eatme->connections);
    if(connectionSize == 0)
        return;
    memcpy(tempConnections, eatme->connections, connectionSize * sizeof(int));
    for(connIndex = 0; connIndex < connectionSize; ++connIndex)
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
            int connectionSize = daSize(&node->connections);
            if(connectionSize == 0)
                break;
            memcpy(tempConnections, node->connections, connectionSize * sizeof(int));

            keepEating = 0;
            for(connIndex = 0; connIndex < connectionSize; ++connIndex)
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
        fprintf(f, "overlap=prism;\n");

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

            for(connIndex = 0; connIndex < daSize(&node->connections); ++connIndex)
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

typedef struct Solver
{
    char *filename;
    int verboseDepth;
    NodeList *nodeList;
    char *colors;
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
    free(solver);
    return NULL;
}

void solverSolve(Solver *solver, int steps, Node **nodes)
{
    int i;
    for(i = 0; i < daSize(&solver->colors); ++i)
    {
        printf("Color: %c\n", solver->colors[i]);
    }
    printf("Node count: %d\n", (int)daSize(&solver->nodeList->nodes));
    nodeListDump(solver->nodeList, "out.txt");
}

// -----------------------------------------------------------------------

int main(int argc, char **argv)
{
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    {
        Solver *solver = solverCreate("B6-4.txt", 0);
        if(solver)
        {
            solverSolve(solver, 4, NULL);
            solverDestroy(solver);
        }
    }
    return 0;
}
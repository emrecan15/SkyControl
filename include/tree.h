//
// Created by Emre on 9.05.2026.
//

#ifndef SKYCONTROL_TREE_H
#define SKYCONTROL_TREE_H

#include "flight.h"

typedef struct TreeNode {
    Flight flight;
    struct TreeNode* left;
    struct TreeNode* right;
} TreeNode;

TreeNode* insertFlight(TreeNode* root, Flight f);
TreeNode* searchFlight(TreeNode* root, int flightId);
TreeNode* deleteFlight(TreeNode* root, int flightId);
void printInorder(const TreeNode* root);
void freeTree(TreeNode* root);

#endif //SKYCONTROL_TREE_H

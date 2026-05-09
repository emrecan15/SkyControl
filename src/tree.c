/*
 * tree.c
 * Implements a Binary Search Tree (BST) used as the Flight Archive.
 * Flights are keyed by their unique ID, enabling O(log n) search on average.
 * An inorder traversal produces a sorted list of all completed flights.
 */

#include <stdio.h>
#include <stdlib.h>
#include "../include/tree.h"
#include "../include/flight.h"

/*
 * insertFlight - Recursively inserts a flight into the BST.
 * Duplicate IDs are silently ignored (BST invariant: left < root < right).
 */
TreeNode *insertFlight(TreeNode *root, Flight f) {
    if (root == NULL) {
        TreeNode *newNode = (TreeNode *)malloc(sizeof(TreeNode));
        if (newNode == NULL) {
            printf("Memory allocation failed (Tree)!\n");
            return NULL;
        }
        newNode->flight = f;
        newNode->left   = NULL;
        newNode->right  = NULL;
        return newNode;
    }

    if (f.id < root->flight.id) {
        root->left  = insertFlight(root->left,  f);
    } else if (f.id > root->flight.id) {
        root->right = insertFlight(root->right, f);
    }

    return root;
}

/*
 * searchFlight - Recursively searches for a flight by ID.
 * Returns a pointer to the matching node, or NULL if not found.
 */
TreeNode *searchFlight(TreeNode *root, int flightId) {
    if (root == NULL || root->flight.id == flightId) {
        return root;
    }
    if (flightId < root->flight.id) {
        return searchFlight(root->left,  flightId);
    }
    return searchFlight(root->right, flightId);
}

/*
 * getMinValueNode - Returns the leftmost (minimum) node in a subtree.
 * Used by deleteFlight to find the inorder successor.
 */
static TreeNode *getMinValueNode(TreeNode *node) {
    TreeNode *current = node;
    while (current && current->left != NULL) {
        current = current->left;
    }
    return current;
}

/*
 * deleteFlight - Removes a flight from the BST by ID.
 * Handles three cases: leaf node, one child, two children (inorder successor).
 * Called during an undo operation to remove a flight from the archive.
 */
TreeNode *deleteFlight(TreeNode *root, int flightId) {
    if (root == NULL) return root;

    if (flightId < root->flight.id) {
        root->left  = deleteFlight(root->left,  flightId);
    } else if (flightId > root->flight.id) {
        root->right = deleteFlight(root->right, flightId);
    } else {
        if (root->left == NULL) {
            TreeNode *temp = root->right;
            free(root);
            return temp;
        } else if (root->right == NULL) {
            TreeNode *temp = root->left;
            free(root);
            return temp;
        }
        const TreeNode *temp = getMinValueNode(root->right);
        root->flight         = temp->flight;
        root->right          = deleteFlight(root->right, temp->flight.id);
    }
    return root;
}

/*
 * printInorder - Prints all archived flights in ascending Flight ID order
 * by performing an inorder (left → root → right) traversal.
 */
void printInorder(const TreeNode *root) {
    if (root != NULL) {
        printInorder(root->left);
        printFlightInfo(root->flight);
        printInorder(root->right);
    }
}

/* Recursively frees all nodes in the BST (post-order traversal). */
void freeTree(TreeNode *root) {
    if (root != NULL) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}
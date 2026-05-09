/*
 * stack.c
 * Implements a linked-list stack used as the Command Log.
 * Each entry records the flight that was processed and the runway it was assigned to,
 * enabling the controller to undo the last runway assignment (LIFO).
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/stack.h"

/* Initializes an empty stack. */
void initStack(Stack *s) {
    s->top  = NULL;
    s->size = 0;
}

/* Returns true if the stack contains no commands. */
bool isStackEmpty(const Stack *s) {
    return s->top == NULL;
}

/*
 * push - Pushes a Command onto the top of the stack.
 * Called immediately after a flight is assigned to a runway.
 */
void push(Stack *s, Command cmd) {
    StackNode *newNode = (StackNode *)malloc(sizeof(StackNode));
    if (newNode == NULL) {
        printf("Memory allocation failed (Stack)!\n");
        return;
    }
    newNode->command = cmd;
    newNode->next    = s->top;
    s->top           = newNode;
    s->size++;
}

/*
 * pop - Removes and returns the most recent Command (LIFO).
 * Returns a sentinel Command (id = -1) if the stack is empty.
 */
Command pop(Stack *s) {
    Command emptyCmd;
    emptyCmd.flight.id          = -1;
    emptyCmd.flight.fuel        = -1;
    emptyCmd.flight.type        = LANDING;
    emptyCmd.flight.status      = NORMAL;
    emptyCmd.flight.arrivalStep = 0;
    emptyCmd.assignedRunway     = -1;

    if (isStackEmpty(s)) {
        printf("No operation to undo (Stack is empty).\n");
        return emptyCmd;
    }

    StackNode *temp = s->top;
    Command    cmd  = temp->command;

    s->top = s->top->next;
    free(temp);
    s->size--;

    return cmd;
}

/* Prints the full command history from top (most recent) to bottom. */
void printStack(const Stack *s) {
    printf("--- Command History (Size: %d) ---\n", s->size);
    if (isStackEmpty(s)) {
        printf("History is empty.\n");
        return;
    }
    StackNode *current = s->top;
    while (current != NULL) {
        printf("Flight ID: %d -> Assigned to Runway %d\n",
               current->command.flight.id,
               current->command.assignedRunway);
        current = current->next;
    }
    printf("----------------------------------\n");
}

/* Releases all nodes in the stack. */
void freeStack(Stack *s) {
    while (!isStackEmpty(s)) {
        pop(s);
    }
}
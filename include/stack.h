//
// Created by Emre on 9.05.2026.
//

#ifndef SKYCONTROL_STACK_H
#define SKYCONTROL_STACK_H

#include "flight.h"

typedef struct {
    Flight flight;
    int assignedRunway; // 1: Runway Alpha (Landing), 2: Runway Bravo (Take-off)
} Command;


typedef struct StackNode {
    Command command;
    struct StackNode* next;
} StackNode;


typedef struct Stack {
    StackNode* top;
    int size;
} Stack;

void initStack(Stack* s);
bool isStackEmpty(const Stack* s);
void push(Stack* s, Command cmd);
Command pop(Stack* s);
void printStack(const Stack* s);
void freeStack(Stack* s);

#endif //SKYCONTROL_STACK_H

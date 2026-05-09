//
// Created by Emre on 9.05.2026.
//

#ifndef SKYCONTROL_QUEUE_H
#define SKYCONTROL_QUEUE_H

#include "../include/flight.h"

typedef struct Node {
  Flight flight;
  struct Node *next;
} Node;

typedef struct Queue {
  Node *front;
  Node *rear;
  int   size;
} Queue;

void   initQueue(Queue *q);
bool   isEmpty(const Queue *q);
void   enqueueTakeoff(Queue *q, Flight f);
void   enqueueLanding(Queue *q, Flight f);
Flight dequeue(Queue *q);
void   printQueue(const Queue *q, const char *queueName);
void   applyFuelDecay(const Queue *q, int decayAmount);  /* bonus: fuel decay per time step */
void   freeQueue(Queue *q);

#endif //SKYCONTROL_QUEUE_H
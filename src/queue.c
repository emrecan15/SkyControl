/*
 * queue.c
 * Implements two queues for the runway management system:
 *   - Landing Queue  : priority-based insertion (emergency first, then lowest fuel)
 *   - Takeoff Queue  : standard FIFO
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../include/queue.h"

/* Initializes an empty queue. */
void initQueue(Queue *q) {
    q->front = NULL;
    q->rear  = NULL;
    q->size  = 0;
}

/* Returns true if the queue has no elements. */
bool isEmpty(const Queue *q) {
    return q->front == NULL;
}

/*
 * enqueueTakeoff - Appends a flight to the rear of the takeoff queue (FIFO).
 */
void enqueueTakeoff(Queue *q, Flight f) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        return;
    }
    newNode->flight = f;
    newNode->next   = NULL;

    if (isEmpty(q)) {
        q->front = newNode;
        q->rear  = newNode;
    } else {
        q->rear->next = newNode;
        q->rear       = newNode;
    }
    q->size++;
}

/*
 * hasHigherPriority - Determines whether newFlight should be served before currentFlight.
 * Priority rules:
 *   1. EMERGENCY status beats NORMAL status.
 *   2. Among flights with equal status, lower fuel level means higher urgency.
 */
static int hasHigherPriority(Flight newFlight, Flight currentFlight) {
    if (newFlight.status == EMERGENCY && currentFlight.status == NORMAL) return 1;
    if (newFlight.status == currentFlight.status && newFlight.fuel < currentFlight.fuel) return 1;
    return 0;
}

/*
 * enqueueLanding - Inserts a flight into the landing queue in priority order.
 * Walks the list to find the correct position so the front always holds
 * the highest-priority flight.
 */
void enqueueLanding(Queue *q, Flight f) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (!newNode) {
        printf("Memory allocation failed\n");
        return;
    }
    newNode->flight = f;
    newNode->next   = NULL;

    if (isEmpty(q) || hasHigherPriority(f, q->front->flight)) {
        newNode->next = q->front;
        q->front      = newNode;
        if (q->rear == NULL) {
            q->rear = newNode;
        }
    } else {
        Node *current = q->front;
        while (current->next != NULL && !hasHigherPriority(f, current->next->flight)) {
            current = current->next;
        }
        newNode->next = current->next;
        current->next = newNode;
        if (newNode->next == NULL) {
            q->rear = newNode;
        }
    }
    q->size++;
}

/*
 * dequeue - Removes and returns the front flight.
 * Returns a sentinel flight (id = -1) if the queue is empty.
 */
Flight dequeue(Queue *q) {
    Flight emptyFlight = {-1, -1, LANDING, NORMAL, 0};
    if (isEmpty(q)) return emptyFlight;

    Node  *temp = q->front;
    Flight f    = temp->flight;
    q->front    = q->front->next;

    if (q->front == NULL) {
        q->rear = NULL;
    }

    free(temp);
    q->size--;
    return f;
}

/* Prints every flight in the queue with a header showing its name and current size. */
void printQueue(const Queue *q, const char *queueName) {
    printf("--- %s (Size: %d) ---\n", queueName, q->size);
    if (isEmpty(q)) {
        printf("Queue is empty.\n");
        return;
    }
    Node *current = q->front;
    while (current != NULL) {
        printFlightInfo(current->flight);
        current = current->next;
    }
    printf("---------------------------\n");
}

/*
 * applyFuelDecay - Decreases fuel of every landing flight in the queue by decayAmount.
 * If a flight's fuel drops to zero or below, it is automatically upgraded to EMERGENCY
 * status so it will be prioritized in the next time step.
 */
void applyFuelDecay(const Queue *q, int decayAmount) {
    Node *current = q->front;
    while (current != NULL) {
        current->flight.fuel -= decayAmount;
        if (current->flight.fuel <= 0) {
            current->flight.fuel   = 1;        /* keep fuel at minimum to avoid invalid state */
            current->flight.status = EMERGENCY; /* critically low fuel triggers emergency */
        }
        current = current->next;
    }
}

/* Releases all nodes in the queue. */
void freeQueue(Queue *q) {
    while (!isEmpty(q)) {
        dequeue(q);
    }
}
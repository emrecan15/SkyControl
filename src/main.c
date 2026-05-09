/*
 * main.c
 * SkyControl Aviation Systems – Dual-Runway Management Simulator
 *
 * Entry point and simulation controller. Coordinates three data structures:
 *   - Queue  : holds pending landing and takeoff requests
 *   - Stack  : command log for undo functionality
 *   - BST    : archive of completed flights for search and sorted reporting
 *
 * Bonus features implemented:
 *   - Fuel decay    : landing queue fuel decreases each time step; critical
 *                     flights are automatically upgraded to EMERGENCY status.
 *   - Random flights: menu option generates a flight with randomized attributes.
 *   - Statistics    : tracks total flights processed and average wait time.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "../include/flight.h"
#include "../include/queue.h"
#include "../include/stack.h"
#include "../include/tree.h"

#define CONGESTION_THRESHOLD 5
#define FUEL_DECAY_PER_STEP  5   /* fuel units lost by each landing flight per time step */

/* ── Statistics ──────────────────────────────────────────────────────────── */

/*
 * Stats - Accumulates data across the simulation session.
 * totalWaitSteps : sum of (processedStep - arrivalStep) for every flight served.
 * totalProcessed : number of flights that have been assigned to a runway.
 */
typedef struct {
    int totalWaitSteps;
    int totalProcessed;
} Stats;

/* Updates statistics when a flight leaves a queue and is assigned to a runway. */
static void recordProcessed(Stats *stats, Flight f, int currentStep) {
    stats->totalWaitSteps += (currentStep - f.arrivalStep);
    stats->totalProcessed++;
}

/* Prints a statistics summary to stdout. */
static void printStats(const Stats *stats) {
    printf("\n--- SIMULATION STATISTICS ---\n");
    printf("Total flights processed : %d\n", stats->totalProcessed);
    if (stats->totalProcessed > 0) {
        double avg = (double)stats->totalWaitSteps / stats->totalProcessed;
        printf("Average wait time       : %.2f time step(s)\n", avg);
    } else {
        printf("Average wait time       : N/A\n");
    }
    printf("-----------------------------\n");
}

/* ── Input helpers ───────────────────────────────────────────────────────── */

/*
 * readInt - Safely reads a single integer from stdin using strtol.
 * Avoids the conversion-error warnings produced by scanf("%d").
 * Returns 1 on success, 0 if the input is not a valid integer.
 */
static int readInt(int *out) {
    char buf[32];
    if (scanf("%31s", buf) != 1) return 0;
    char *end;
    long val = strtol(buf, &end, 10);
    if (*end != '\0') return 0;
    *out = (int)val;
    return 1;
}

/*
 * clearInputBuffer - Discards all characters remaining in stdin up to
 * and including the next newline. Prevents stale input from affecting
 * subsequent reads after an invalid entry.
 */
static void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { (void)c; }
}

/* ── Menu ────────────────────────────────────────────────────────────────── */

/* Prints the main menu to stdout. */
void displayMenu(void) {
    printf("\n==========================================\n");
    printf("  SKYCONTROL AVIATION SYSTEMS - TERMINAL\n");
    printf("==========================================\n");
    printf("1. Add New Flight\n");
    printf("2. Process Next Time Step (Simulation)\n");
    printf("3. Display Queues & System Status\n");
    printf("4. Undo Last Assignment (Command Log)\n");
    printf("5. Search Completed Flights Archive\n");
    printf("6. Add Random Flight (Bonus)\n");
    printf("7. Show Statistics (Bonus)\n");
    printf("8. Exit Simulation\n");
    printf("==========================================\n");
    printf("Select operation: ");
}

/* ── Flight management ───────────────────────────────────────────────────── */

/*
 * handleAddFlight - Prompts the controller for flight details and adds the
 * new flight to the appropriate queue.
 * Input is validated before the flight is created:
 *   - type   : must be 0 (Landing) or 1 (Takeoff)
 *   - status : must be 0 (Normal)  or 1 (Emergency)
 *   - fuel   : must be in the range [1, 100]
 */
void handleAddFlight(Queue *landingQueue, Queue *takeoffQueue,
                     int *flightCounter, int currentStep) {
    int typeChoice   = 0;
    int statusChoice = 0;
    int fuel         = 0;

    printf("Flight Type (0: Landing, 1: Takeoff): ");
    readInt(&typeChoice);
    if (typeChoice != 0 && typeChoice != 1) {
        printf("[ERROR] Invalid flight type. Must be 0 or 1.\n");
        return;
    }

    printf("Status (0: Normal, 1: Emergency): ");
    readInt(&statusChoice);
    if (statusChoice != 0 && statusChoice != 1) {
        printf("[ERROR] Invalid status. Must be 0 or 1.\n");
        return;
    }

    printf("Fuel Level (1-100): ");
    readInt(&fuel);
    if (fuel < 1 || fuel > 100) {
        printf("[ERROR] Invalid fuel level. Must be between 1 and 100.\n");
        return;
    }

    Flight newFlight = createFlight(*flightCounter, fuel,
                                    typeChoice   == 0 ? LANDING   : TAKEOFF,
                                    statusChoice == 0 ? NORMAL    : EMERGENCY,
                                    currentStep);
    (*flightCounter)++;

    if (newFlight.type == LANDING) {
        enqueueLanding(landingQueue, newFlight);
        printf("[SUCCESS] Flight ID %d added to Landing Queue.\n", newFlight.id);
    } else {
        enqueueTakeoff(takeoffQueue, newFlight);
        printf("[SUCCESS] Flight ID %d added to Takeoff Queue.\n", newFlight.id);
    }
}

/*
 * handleRandomFlight - Bonus feature: generates a flight with randomized
 * type, status, and fuel level and adds it to the appropriate queue.
 */
void handleRandomFlight(Queue *landingQueue, Queue *takeoffQueue,
                        int *flightCounter, int currentStep) {
    FlightType type   = (rand() % 2 == 0) ? LANDING  : TAKEOFF;
    Status     status = (rand() % 5 == 0) ? EMERGENCY : NORMAL; /* ~20% emergency chance */
    int        fuel   = (rand() % 100) + 1;                     /* 1-100 */

    Flight f = createFlight(*flightCounter, fuel, type, status, currentStep);
    (*flightCounter)++;

    if (f.type == LANDING) {
        enqueueLanding(landingQueue, f);
    } else {
        enqueueTakeoff(takeoffQueue, f);
    }

    printf("[RANDOM] Flight ID %d generated -> Type: %s | Status: %s | Fuel: %d\n",
           f.id,
           f.type   == LANDING   ? "LANDING"   : "TAKEOFF",
           f.status == EMERGENCY ? "EMERGENCY" : "NORMAL",
           f.fuel);
}

/* ── Runway processing ───────────────────────────────────────────────────── */

/*
 * processEmergency - Emergency Protocol:
 *   Runway 1 (Alpha) is reserved for the emergency landing at the front of
 *   the landing queue. Runway 2 (Bravo) handles the next takeoff if available.
 */
void processEmergency(Queue *landingQueue, Queue *takeoffQueue,
                      Stack *commandLog, TreeNode **flightArchive,
                      Stats *stats, int currentStep) {
    printf("[ALERT] EMERGENCY PROTOCOL ACTIVATED!\n");

    Flight  f   = dequeue(landingQueue);
    Command cmd = { f, 1 };
    push(commandLog, cmd);
    *flightArchive = insertFlight(*flightArchive, f);
    recordProcessed(stats, f, currentStep);
    printf("Runway 1 (Alpha): EMERGENCY LANDING for Flight %d\n", f.id);

    if (!isEmpty(takeoffQueue)) {
        Flight  t    = dequeue(takeoffQueue);
        Command cmd2 = { t, 2 };
        push(commandLog, cmd2);
        *flightArchive = insertFlight(*flightArchive, t);
        recordProcessed(stats, t, currentStep);
        printf("Runway 2 (Bravo): Takeoff for Flight %d\n", t.id);
    } else {
        printf("Runway 2 (Bravo): Idle.\n");
    }
}

/*
 * processCongested - Congestion Mode (queue size >= CONGESTION_THRESHOLD):
 *   The dedicated-runway rule is lifted. Both runways serve whichever queue
 *   is larger first, maximizing throughput until the backlog clears.
 */
void processCongested(Queue *landingQueue, Queue *takeoffQueue,
                      Stack *commandLog, TreeNode **flightArchive,
                      Stats *stats, int currentStep) {
    printf("[ALERT] CONGESTION MODE ACTIVATED! (Dynamic Routing)\n");

    if (landingQueue->size >= takeoffQueue->size) {
        if (!isEmpty(landingQueue)) {
            Flight  f1 = dequeue(landingQueue);
            Command c1 = { f1, 1 };
            push(commandLog, c1);
            *flightArchive = insertFlight(*flightArchive, f1);
            recordProcessed(stats, f1, currentStep);
            printf("Runway 1 (Alpha): Landing for Flight %d\n", f1.id);
        }
        if (!isEmpty(landingQueue)) {
            Flight  f2 = dequeue(landingQueue);
            Command c2 = { f2, 2 };
            push(commandLog, c2);
            *flightArchive = insertFlight(*flightArchive, f2);
            recordProcessed(stats, f2, currentStep);
            printf("Runway 2 (Bravo): Landing for Flight %d\n", f2.id);
        } else if (!isEmpty(takeoffQueue)) {
            Flight  t1 = dequeue(takeoffQueue);
            Command c3 = { t1, 2 };
            push(commandLog, c3);
            *flightArchive = insertFlight(*flightArchive, t1);
            recordProcessed(stats, t1, currentStep);
            printf("Runway 2 (Bravo): Takeoff for Flight %d\n", t1.id);
        }
    } else {
        if (!isEmpty(takeoffQueue)) {
            Flight  t1 = dequeue(takeoffQueue);
            Command c1 = { t1, 1 };
            push(commandLog, c1);
            *flightArchive = insertFlight(*flightArchive, t1);
            recordProcessed(stats, t1, currentStep);
            printf("Runway 1 (Alpha): Takeoff for Flight %d\n", t1.id);
        }
        if (!isEmpty(takeoffQueue)) {
            Flight  t2 = dequeue(takeoffQueue);
            Command c2 = { t2, 2 };
            push(commandLog, c2);
            *flightArchive = insertFlight(*flightArchive, t2);
            recordProcessed(stats, t2, currentStep);
            printf("Runway 2 (Bravo): Takeoff for Flight %d\n", t2.id);
        }
    }
}

/*
 * processStandard - Standard Operations:
 *   Runway 1 (Alpha) handles one landing; Runway 2 (Bravo) handles one takeoff.
 *   Each runway is marked Idle if its queue is empty.
 */
void processStandard(Queue *landingQueue, Queue *takeoffQueue,
                     Stack *commandLog, TreeNode **flightArchive,
                     Stats *stats, int currentStep) {
    printf("[INFO] STANDARD OPERATIONS\n");

    if (!isEmpty(landingQueue)) {
        Flight  f  = dequeue(landingQueue);
        Command c1 = { f, 1 };
        push(commandLog, c1);
        *flightArchive = insertFlight(*flightArchive, f);
        recordProcessed(stats, f, currentStep);
        printf("Runway 1 (Alpha): Landing for Flight %d\n", f.id);
    } else {
        printf("Runway 1 (Alpha): Idle.\n");
    }

    if (!isEmpty(takeoffQueue)) {
        Flight  t  = dequeue(takeoffQueue);
        Command c2 = { t, 2 };
        push(commandLog, c2);
        *flightArchive = insertFlight(*flightArchive, t);
        recordProcessed(stats, t, currentStep);
        printf("Runway 2 (Bravo): Takeoff for Flight %d\n", t.id);
    } else {
        printf("Runway 2 (Bravo): Idle.\n");
    }
}

/*
 * handleTimeStep - Advances the simulation by one time step.
 * Steps performed each tick:
 *   1. Apply fuel decay to all waiting landing flights.
 *   2. Determine operational mode and assign runways accordingly.
 * Mode priority: Emergency > Congestion > Standard.
 */
void handleTimeStep(Queue *landingQueue, Queue *takeoffQueue,
                    Stack *commandLog, TreeNode **flightArchive,
                    Stats *stats, int *currentStep) {
    (*currentStep)++;
    printf("\n--- PROCESSING TIME STEP %d ---\n", *currentStep);

    /* Bonus: reduce fuel for every landing flight still waiting in the queue */
    applyFuelDecay(landingQueue, FUEL_DECAY_PER_STEP);

    bool isEmergency = (!isEmpty(landingQueue) &&
                        landingQueue->front->flight.status == EMERGENCY);
    bool isCongested = (landingQueue->size >= CONGESTION_THRESHOLD ||
                        takeoffQueue->size >= CONGESTION_THRESHOLD);

    if (isEmergency) {
        processEmergency(landingQueue, takeoffQueue, commandLog, flightArchive, stats, *currentStep);
    } else if (isCongested) {
        processCongested(landingQueue, takeoffQueue, commandLog, flightArchive, stats, *currentStep);
    } else {
        processStandard(landingQueue, takeoffQueue, commandLog, flightArchive, stats, *currentStep);
    }
}

/* ── Undo & Search ───────────────────────────────────────────────────────── */

/*
 * handleUndo - Reverts the most recent runway assignment.
 * Pops the last Command from the stack, removes the flight from the archive,
 * and re-inserts it into its original queue so it can be reassigned.
 * Statistics are also rolled back by one entry.
 */
void handleUndo(Queue *landingQueue, Queue *takeoffQueue,
                Stack *commandLog, TreeNode **flightArchive, Stats *stats) {
    if (!isStackEmpty(commandLog)) {
        Command lastCmd = pop(commandLog);
        *flightArchive  = deleteFlight(*flightArchive, lastCmd.flight.id);

        /* roll back the stat entry recorded when this flight was processed */
        if (stats->totalProcessed > 0) {
            stats->totalProcessed--;
        }

        if (lastCmd.flight.type == LANDING) {
            enqueueLanding(landingQueue, lastCmd.flight);
        } else {
            enqueueTakeoff(takeoffQueue, lastCmd.flight);
        }
        printf("[UNDO] Flight %d reverted to queue and removed from archive.\n",
               lastCmd.flight.id);
    } else {
        printf("[UNDO] No operations to undo.\n");
    }
}

/*
 * handleSearch - Searches the flight archive by Flight ID.
 * After displaying the result, prints the entire archive in sorted order
 * via an inorder BST traversal.
 */
void handleSearch(TreeNode *flightArchive) {
    int searchId = 0;
    printf("Enter Flight ID to search: ");
    readInt(&searchId);

    const TreeNode *result = searchFlight(flightArchive, searchId);
    if (result != NULL) {
        printf("\n[FOUND] Flight Details:\n");
        printFlightInfo(result->flight);
    } else {
        printf("\n[NOT FOUND] Flight %d does not exist in the archive.\n", searchId);
    }

    printf("\n--- COMPLETE FLIGHT ARCHIVE (SORTED) ---\n");
    printInorder(flightArchive);
}

/* ── Entry point ─────────────────────────────────────────────────────────── */

/*
 * main - Initializes all data structures and runs the interactive menu loop.
 * Cleans up all allocated memory before exiting.
 */
int main(void) {
    srand((unsigned int)time(NULL)); /* seed RNG for random flight generation */

    Queue    landingQueue;
    Queue    takeoffQueue;
    Stack    commandLog;
    TreeNode *flightArchive = NULL;
    Stats    stats          = { 0, 0 };

    initQueue(&landingQueue);
    initQueue(&takeoffQueue);
    initStack(&commandLog);

    int choice        = 0;
    int flightCounter = 1;
    int currentStep   = 0;  /* incremented inside handleTimeStep */

    while (1) {
        displayMenu();
        if (readInt(&choice) != 1) {
            clearInputBuffer();
            printf("Invalid input. Please enter a number.\n");
            continue;
        }

        if (choice == 1) {
            handleAddFlight(&landingQueue, &takeoffQueue, &flightCounter, currentStep);
        } else if (choice == 2) {
            handleTimeStep(&landingQueue, &takeoffQueue, &commandLog,
                           &flightArchive, &stats, &currentStep);
        } else if (choice == 3) {
            printf("\n");
            printQueue(&landingQueue, "Active Landing Queue");
            printQueue(&takeoffQueue, "Active Takeoff Queue");
            printStack(&commandLog);
        } else if (choice == 4) {
            handleUndo(&landingQueue, &takeoffQueue, &commandLog, &flightArchive, &stats);
        } else if (choice == 5) {
            handleSearch(flightArchive);
        } else if (choice == 6) {
            handleRandomFlight(&landingQueue, &takeoffQueue, &flightCounter, currentStep);
        } else if (choice == 7) {
            printStats(&stats);
        } else if (choice == 8) {
            printf("Initiating shutdown sequence. Cleaning up memory...\n");
            break;
        } else {
            printf("Invalid selection. Please choose a valid menu option.\n");
        }
    }

    freeQueue(&landingQueue);
    freeQueue(&takeoffQueue);
    freeStack(&commandLog);
    freeTree(flightArchive);

    printf("System offline.\n");
    return 0;
}
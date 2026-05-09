/*
* flight.c
 * Implements flight creation and display utilities.
 */

#include <stdio.h>
#include "../include/flight.h"

/*
 * createFlight - Initializes a Flight struct.
 * currentStep is stored as arrivalStep for wait-time statistics.
 * Input validation is performed by the caller before this function is invoked.
 */
Flight createFlight(int id, int fuel, FlightType type, Status status, int currentStep) {
    Flight flight;
    flight.id          = id;
    flight.fuel        = fuel;
    flight.type        = type;
    flight.status      = status;
    flight.arrivalStep = currentStep;
    return flight;
}

/*
 * printFlightInfo - Prints a single flight's details in a formatted table row.
 */
void printFlightInfo(Flight f) {
    printf("Flight ID: %-3d | Fuel: %-3d | Type: %-7s | Status: %-9s\n",
           f.id,
           f.fuel,
           f.type == LANDING ? "LANDING" : "TAKE-OFF",
           f.status == EMERGENCY ? "EMERGENCY" : "NORMAL");
}
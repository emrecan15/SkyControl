//
// Created by Emre on 5.05.2026.
//

#ifndef SKYCONTROL_FLIGHT_H
#define SKYCONTROL_FLIGHT_H

typedef enum {
    LANDING,
    TAKEOFF
} FlightType;

typedef enum {
    NORMAL,
    EMERGENCY
} Status;

typedef struct Flight {
    int id;
    int fuel;
    FlightType type;
    Status status;
    int arrivalStep;  /* time step at which this flight joined the queue (for wait-time stats) */
} Flight;

Flight createFlight(int id, int fuel, FlightType type, Status status, int currentStep);
void   printFlightInfo(Flight f);

#endif //SKYCONTROL_FLIGHT_H
#include <stdio.h>
#include "../include/flight.h"
int main(void) {

    Flight flight;
    flight.id=1;
    flight.fuel=50;
    flight.status=EMERGENCY;
    flight.type=TAKEOFF;

    printf("%d\n",flight.id);
    printf("%d\n",flight.fuel);
    printf("%d\n",flight.status);
    printf("%d\n",flight.type);

    return 0;
}

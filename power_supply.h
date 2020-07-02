#include "power_system.h"

#ifndef POWER_SUPPLY_H
#define POWER_SUPPLY_H

typedef struct
{
    int conn_sock;
    int msqid;
    powsys_t *powsys;
} power_supply_t;

power_supply_t *make_power_supply(int conn_sock, int shmid_system, int msqid);
void start_power_supply(power_supply_t *powsup);

#endif // POWER_SUPPLY_H
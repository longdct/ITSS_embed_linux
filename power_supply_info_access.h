#include "equipment.h"
#include "power_system.h"

#ifndef POWER_SUPPLY_INFO_ACCESS_H
#define POWER_SUPPLY_INFO_ACCESS_H

typedef struct
{
    int msqid;
    equip_t *equipment;
    power_system_t *powsys;
} power_supply_info_access_t;

/**
 * Constructor of power supply info access
 * */
power_supply_info_access_t *make_power_supply_info_access(int shmid_system, int shmid_equipment, int msqid);

/**
 * Start power supply info access
 * */
void start_power_supply_info_access(power_supply_info_access_t *powsup_info_access);

#endif // POWER_SUPPLY_INFO_ACCESS_H
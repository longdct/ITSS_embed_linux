#ifndef POWER_SYSTEM_H
#define POWER_SYSTEM_H

/**
 * Structure of power system
 * */

typedef struct
{
	int current_power;
	int threshold_over;
	int supply_over;
} power_system_t;

#endif // POWER_SYSTEM_H
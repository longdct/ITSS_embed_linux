#ifndef POWER_SYSTEM_H
#define POWER_SYSTEM_H

// power system struct
typedef struct
{
	int current_power;
	int threshold_over;
	int supply_over;
	// int reset;
} powsys_t;

#endif // POWER_SYSTEM_H
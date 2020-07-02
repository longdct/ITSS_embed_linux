#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include "use_mode.h"

#define MAX_EQUIP_NAME 50

// equip struct
typedef struct
{
	int pid;
	char name[MAX_EQUIP_NAME];
	int use_power[3];
	int mode;
} equip_t;

/**
 * Create new equip
 * */
equip_t *make_equip(int pid, char name[], int ordinal_use, int limited_use);

/**
 * Set attributes for equip
 * */
equip_t *set_equip(equip_t *equip, int pid, char name[], int ordinal_use, int limited_use);

/**
 * Reset equip
 * */
equip_t *reset_equip(equip_t *equip);

/**
 * Print to stdout the information of the equip
 * */
void print_equip(equip_t *equip);

#endif // EQUIPMENT_H
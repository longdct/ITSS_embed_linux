#include "equipment.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>

equip_t *make_equip(int pid, char name[], int ordinal_use, int limited_use)
{
	equip_t *equip = (equip_t *)malloc(sizeof(equip_t));
	equip->pid = pid;
	strcpy(equip->name, name);
	equip->use_power[0] = 0;
	equip->use_power[1] = ordinal_use;
	equip->use_power[2] = limited_use;
	return equip;
}

equip_t *set_equip(equip_t *equip, int pid, char name[], int ordinal_use, int limited_use)
{
	equip->pid = pid;
	strcpy(equip->name, name);
	equip->use_power[0] = 0;
	equip->use_power[1] = ordinal_use;
	equip->use_power[2] = limited_use;
	return equip;
}

equip_t *reset_equip(equip_t *equip)
{
	equip->pid = 0;
	strcpy(equip->name, "");
	equip->use_power[0] = 0;
	equip->use_power[1] = 0;
	equip->use_power[2] = 0;
	equip->mode = 0;
	return equip;
}

void print_equip(equip_t *equip)
{
	time_printf("-----------------------------\n");
	time_printf("Equipment Information\n");
	time_printf("\t Equipment Name: %s\n", equip->name);
	time_printf("\t Ordinal Mode: %dW\n", equip->use_power[1]);
	time_printf("\t Limited Mode: %dW\n", equip->use_power[2]);
	time_printf("\t Current Mode: %s\n", mode_to_string(equip->mode));
	time_printf("-----------------------------\n\n");
}
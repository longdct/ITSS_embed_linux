#ifndef ELEC_POWER_CTRL_H
#define ELEC_POWER_CTRL_H

#include "config.h"

void elec_power_ctrl_handle(int shmid_equipment, int shmid_system, int msqid);

#endif
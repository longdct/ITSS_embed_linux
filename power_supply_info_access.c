#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/shm.h>

#include "power_supply_info_access.h"
#include "utils.h"
#include "message.h"
#include "config.h"
#include "use_mode.h"

power_supply_info_access_t *make_power_supply_info_access(int shmid_system, int shmid_equipment, int msqid)
{
	power_supply_info_access_t *powsup_info_access = (power_supply_info_access_t *)malloc(sizeof(power_supply_info_access_t));
	if ((powsup_info_access->equipment = (equip_t *)shmat(shmid_equipment, (void *)0, 0)) == (void *)-1)
	{
		time_printf("shmat() for equipment list failed at power supply info access creation\n");
		free(powsup_info_access);
		return NULL;
	}
	if ((powsup_info_access->powsys = (powsys_t *)shmat(shmid_system, (void *)0, 0)) == (void *)-1)
	{
		time_printf("shmat() for power system failed at power supply info access creation\n");
		free(powsup_info_access);
		return NULL;
	}
	powsup_info_access->msqid = msqid;
	return powsup_info_access;
}

void start_power_supply_info_access(power_supply_info_access_t *powsup_info_access)
{
	char name[MAX_EQUIP_NAME];
	int mode;
	int pid;
	int ordinal_use;
	int limited_use;
	message_t mess;
	equip_t *equip;
	while (1)
	{
		// received new message
		if (msgrcv(powsup_info_access->msqid, &mess, MAX_MESSAGE_LENGTH, 2, 0) <= 0)
		{
			time_printf("msgrcv() error for msqid at pow_sup_info_access\n");
			msgctl(powsup_info_access->msqid, IPC_RMID, NULL);
			exit(1);
		}

		// in case of creating new equip
		if (mess.mtext[0] == 'n')
		{
			for (int i = 0; i < MAX_EQUIP; i++)
			{
				if (powsup_info_access->equipment[i].pid == 0)
				{
					equip = &(powsup_info_access->equipment[i]);
					break;
				}
			}

			sscanf(mess.mtext, R_NEW_EQUIPMENT_FORMAT, &pid, name, &ordinal_use, &limited_use);
			equip = set_equip(equip, pid, name, ordinal_use, limited_use);

			print_equip(equip);
			time_printf("Current system supply: %dW\n", powsup_info_access->powsys->current_power);
		}
		else if (mess.mtext[0] == 'm') // in case of changing mode
		{
			sscanf(mess.mtext, R_MODE_CHANGING_FORMAT, &pid, &mode);

			for (int i = 0; i < MAX_EQUIP; i++)
			{
				if (powsup_info_access->equipment[i].pid == pid)
				{
					equip = &(powsup_info_access->equipment[i]);
					break;
				}
			}
			equip->mode = mode;

			print_equip(equip);
		}
		else if (mess.mtext[0] == 'd') // in case of disconnection
		{
			sscanf(mess.mtext, R_DISCONNECTING_FORMAT, pid);

			char temp[MAX_MESSAGE_LENGTH];

			sprintf(temp, "Equip %s disconnected", equip->name);
			int i;
			for (i = 0; i < MAX_EQUIP; i++)
			{
				equip = &(powsup_info_access->equipment[i]);
				if (equip->pid == pid)
				{
					time_printf("%s\n", temp);
					reset_equip(equip);
					break;
				}
			}
			if (i == MAX_EQUIP)
			{
				time_printf("Error! Equipment not found\n\n");
			}
			else
			{
				sprintf(temp, "Current system supply: %dW", powsup_info_access->powsys->current_power);
				time_printf("%s\n", temp);
			}
		}

	} // endwhile
} //end function powSupplyInfoAccess

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
		tprintf("shmat() for equipment list failed at power supply info access creation\n");
		free(powsup_info_access);
		return NULL;
	}
	if ((powsup_info_access->powsys = (powsys_t *)shmat(shmid_system, (void *)0, 0)) == (void *)-1)
	{
		tprintf("shmat() for power system failed at power supply info access creation\n");
		free(powsup_info_access);
		return NULL;
	}
	powsup_info_access->msqid = msqid;
	return powsup_info_access;
}

void start_power_supply_info_access(power_supply_info_access_t *powsup_info_access)
{
	// mtype = 2
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
		if (msgrcv(powsup_info_access->msqid, &mess, MAX_MESSAGE_LENGTH + 1, 2, 0) <= 0)
		{
			// printf("\n%s\n", message_to_string(mess));
			printf("At powsup info msqid: %i\n", powsup_info_access->msqid);
			tprintf("msgrcv() error\n");
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

			sscanf(mess.mtext, "%*c|%d|%[^|]|%d|%d|", &pid, name, &ordinal_use, &limited_use);
			equip = set_equip(equip, pid, name, ordinal_use, limited_use);

			print_equip(equip);
			tprintf("System power using: %dW\n", powsup_info_access->powsys->current_power);

			// send message to logWrite

			mess = *make_message(1, "s|[%s] connected (Ordinal use: %dW, Limited use: %dW)|", equip->name,
								 equip->use_power[1],
								 equip->use_power[2]);
			msgsnd(powsup_info_access->msqid, &mess, MAX_MESSAGE_LENGTH, 0);
			printf("Sent: %s", message_to_string(&mess));

			mess = *make_message(1, "s|equip [%s] set mode to [off] ~ using 0W|", equip->name);
			msgsnd(powsup_info_access->msqid, &mess, MAX_MESSAGE_LENGTH, 0);
			printf("Sent: %s", message_to_string(&mess));
		}
		else if (mess.mtext[0] == 'm') // in case of changing mode
		{
			sscanf(mess.mtext, "%*c|%d|%d|", &pid, &mode);

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

			// send message to logWrite
			mess = *make_message(1, "s|equip [%s] has changed its mode to [%s], comsuming %dW",
								 equip->name,
								 mode_to_string(equip->mode),
								 equip->use_power[equip->mode]);
			msgsnd(powsup_info_access->msqid, &mess, MAX_MESSAGE_LENGTH, 0);
			printf("Sent: %s", message_to_string(&mess));

			sleep(1);
			char temp[MAX_MESSAGE_LENGTH];
			sprintf(temp, "System power using: %dW", powsup_info_access->powsys->current_power);
			tprintf("%s\n", temp);
			mess = *make_message(1, "s|%s", temp);
			msgsnd(powsup_info_access->msqid, &mess, MAX_MESSAGE_LENGTH, 0);
			printf("Sent: %s", message_to_string(&mess));
		}
		else if (mess.mtext[0] == 'd') // in case of disconnection
		{
			sscanf(mess.mtext, "%*c|%d|", pid);

			// send message to logWrite

			char temp[MAX_MESSAGE_LENGTH];

			sprintf(temp, "equip [%s] disconnected", equip->name);
			int i;
			for (i = 0; i < MAX_EQUIP; i++)
			{
				equip = &(powsup_info_access->equipment[i]);
				if (equip->pid == pid)
				{
					tprintf("%s\n\n", temp);
					reset_equip(equip);
					break;
				}
			}
			if (i == MAX_EQUIP)
			{
				tprintf("Error! Equipment not found\n\n");
			}
			mess = *make_message(1, "s|%s", temp);
			msgsnd(powsup_info_access->msqid, &mess, MAX_MESSAGE_LENGTH, 0);

			sprintf(temp, "System power using: %dW", powsup_info_access->powsys->current_power);
			tprintf("%s\n", temp);
			mess = *make_message(1, "s|%s", temp);
			msgsnd(powsup_info_access->msqid, &mess, MAX_MESSAGE_LENGTH, 0);
		}

	} // endwhile
} //end function powSupplyInfoAccess

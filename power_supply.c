#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>
#include <stddef.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "config.h"
#include "power_supply.h"
#include "message.h"
#include "utils.h"

power_supply_t *make_power_supply(int conn_sock, int shmid_system, int msqid)
{
	power_supply_t *powsup = (power_supply_t *)malloc(sizeof(power_supply_t));

	// Get shared memory
	if ((powsup->powsys = (power_system_t *)shmat(shmid_system, (void *)0, 0)) == (void *)-1)
	{
		time_printf("shmat() for power system failed at power supply creation\n");
		free(powsup);
		return NULL;
	}
	powsup->conn_sock = conn_sock;
	powsup->msqid = msqid;

	return powsup;
}

void start_power_supply(power_supply_t *powsup)
{
	bool is_accept_message = true;
	char received_data[BUFF_SIZE];
	int bytes_received;
	message_t *mess;

	while (1)
	{
		bytes_received = recv(powsup->conn_sock, received_data, BUFF_SIZE - 1, 0);
		if (bytes_received <= 0) // if connection is disconnected
		{
			// send device info to elec_power_ctrl
			mess = make_message(ELEC_POWER_CTRL_MESS_CODE, W_DISCONNECTING_FORMAT, getpid());
			msgsnd(powsup->msqid, mess, MAX_MESSAGE_LENGTH, 0);

			power_supply_count--;

			kill(getpid(), SIGKILL); // kill this process
			break;
		}
		else // if receive message from client
		{

			received_data[bytes_received] = '\0';
			if (is_accept_message == true)
			{
				is_accept_message = false;

				// send device info to power_supply_info_access
				mess = make_message(POW_SUP_INF_ACC_MESS_CODE, W_NEW_EQUIPMENT_FORMAT, getpid(), received_data);
				msgsnd(powsup->msqid, mess, MAX_MESSAGE_LENGTH, 0);

				// send device info to elec_power_ctrl
				mess->mtype = ELEC_POWER_CTRL_MESS_CODE;
				msgsnd(powsup->msqid, mess, MAX_MESSAGE_LENGTH, 0);
			}
			else
			{
				// if not accept message sent

				// send mode to to power_supply_info_access
				mess = make_message(POW_SUP_INF_ACC_MESS_CODE, W_MODE_CHANGING_FORMAT, getpid(), received_data);
				msgsnd(powsup->msqid, mess, MAX_MESSAGE_LENGTH, 0);

				// send device info to elec_power_ctrl
				mess->mtype = ELEC_POWER_CTRL_MESS_CODE;
				msgsnd(powsup->msqid, mess, MAX_MESSAGE_LENGTH, 0);
			}
		}
	}
}
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
	if ((powsup->powsys = (powsys_t *)shmat(shmid_system, (void *)0, 0)) == (void *)-1)
	{
		tprintf("shmat() for power system failed at power supply creation\n");
		free(powsup);
		return NULL;
	}
	powsup->conn_sock = conn_sock;
	powsup->msqid = msqid;

	return powsup;
} // end function make_power_supply

void start_power_supply(power_supply_t *powsup)
{
	bool is_first_message = true; // check if this is first time client sent
	char received_data[BUFF_SIZE];
	int bytes_received;
	message_t *mess;

	while (1)
	{
		bytes_received = recv(powsup->conn_sock, received_data, BUFF_SIZE - 1, 0);
		if (bytes_received <= 0)
		{
			// if connection is disconnected,
			// send message to powSupplyInfoAccess
			mess = make_message(2, DISCONNECTING_FORMAT, getpid());
			msgsnd(powsup->msqid, mess, MAX_MESSAGE_LENGTH, 0);
			printf("Sent: %s", message_to_string(mess));

			powerSupply_count--;
			// kill this process
			kill(getpid(), SIGKILL);
			break;
		}
		else
		{
			// if receive message from client
			received_data[bytes_received] = '\0';
			if (is_first_message == true)
			{
				is_first_message = false;
				// send device info to powSupplyInfoAccess

				mess = make_message(2, NEW_EQUIPMENT_FORMAT, getpid(), received_data); // n for NEW
				msgsnd(powsup->msqid, mess, MAX_MESSAGE_LENGTH, 0);
				printf("Sent: %s", message_to_string(mess));
			}
			else
			{
				// if not first time client send
				// send mode to powSupplyInfoAccess

				mess = make_message(2, MODE_CHANGING_FORMAT, getpid(), received_data); // m for MODE
				msgsnd(powsup->msqid, mess, MAX_MESSAGE_LENGTH, 0);
				printf("Sent: %s", message_to_string(mess));
			}
		}
	} 
}
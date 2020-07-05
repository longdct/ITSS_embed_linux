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
#include <unistd.h>
#include "config.h"
#include "power_supply_info_access.h"
#include "utils.h"
#include "message.h"
#include "config.h"
#include "use_mode.h"
#include "connect_message.h"
#include "power_supply.h"
#include "utils.h"
#include "equipment.h"


connect_message_t *make_connect_message(int shmid_system, int listen_sock, int msqid)
{
	connect_message_t *connect_message = (connect_message_t *)malloc(sizeof(connect_message_t));
	connect_message->listen_sock = listen_sock;
	connect_message->msqid = msqid;
	connect_message->shmid_system = shmid_system;
	return connect_message;
}
void start_connect_message(connect_message_t *connect_message)
{
	int sin_size;
	int conn_sock;
	struct sockaddr_in client;
	int bytes_sent, bytes_received;
	pid_t power_supply;
	if (listen(connect_message->listen_sock, BACKLOG) == -1)
	{
		time_printf("listen() failed\n");
		exit(1);
	}
	while (1)
	{

		sin_size = sizeof(struct sockaddr_in);
		if ((conn_sock = accept(connect_message->listen_sock, (struct sockaddr *)&client, &sin_size)) == -1)
		{
			time_printf("accept() failed\n");
			continue;
		}

		if (power_supply_count == MAX_EQUIP)
		{
			char re = '9';
			if ((bytes_sent = send(conn_sock, &re, 1, 0)) <= 0)
				time_printf("send() failed\n");
			close(conn_sock);
			break;
		}

		if ((power_supply = fork()) < 0)
		{
			time_printf("power_supply fork() failed\n");
			continue;
		}

		if (power_supply == 0)
		{
			//child process
			close(connect_message->listen_sock);
			start_power_supply(make_power_supply(conn_sock, connect_message->shmid_system, connect_message->msqid));
			close(conn_sock);
		}
		else
		{
			//parent process
			close(conn_sock);
			power_supply_count++;
			time_printf("A equip connected, connect_mng forked new process power_supply --- pid: %d.\n", power_supply);
		}
	}

	close(connect_message->listen_sock);
}
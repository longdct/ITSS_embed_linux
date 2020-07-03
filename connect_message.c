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
#include "connect_message.h"
#include "power_supply.h"

connect_message_t *make_connect_message(int listen_sock,int msqid){
    	connect_message_t *connect_message = (connect_message_t *)malloc(sizeof(connect_message_t));
        connect_message->listen_sock=listen_sock;
        connect_message->msqid=msqid;
        return connect_message;
}
void start_connect_message(connect_message_t connect_message){
    /Step 3: Listen request from client
	if (listen(listen_sock, BACKLOG) == -1)
	{
		tprintf("listen() failed\n");
		exit(1);
	}

	//Step 4: Communicate with client
	while (1)
	{
		//accept request
		sin_size = sizeof(struct sockaddr_in);
		if ((conn_sock = accept(listen_sock, (struct sockaddr *)&client, &sin_size)) == -1)
		{
			tprintf("accept() failed\n");
			continue;
		}

		// if 11-th equip connect to SERVER
		if (powerSupply_count == MAX_EQUIP)
		{
			char re = '9';
			if ((bytes_sent = send(conn_sock, &re, 1, 0)) <= 0)
				tprintf("send() failed\n");
			close(conn_sock);
			break;
		}

		// create new process powerSupply
		if ((powerSupply = fork()) < 0)
		{
			tprintf("powerSupply fork() failed\n");
			continue;
		}

		if (powerSupply == 0)
		{
			//in child
			close(listen_sock);
			start_power_supply(make_power_supply(conn_sock, shmid_system, msqid));
			close(conn_sock);
		}
		else
		{
			//in parent
			close(conn_sock);
			powerSupply_count++;
			tprintf("A equip connected, connectMng forked new process powerSupply --- pid: %d.\n", powerSupply);
		}
	} //end communication

	close(listen_sock);
} //end function connectMng_handle
}
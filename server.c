//////////////////////////////////////////////////////////
//  \    /\   Simple power supply program - server side //
//   )  ( ')                                            //
//  (  /  )                                             //
//   \(__)|                                          N. //
//////////////////////////////////////////////////////////

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
#include "power_supply.h"
#include "message.h"
#include "utils.h"
#include "equipment.h"
#include "use_mode.h"
#include "connect_message.h"
#define POWER_THRESHOLD 5000
#define WARNING_THRESHOLD 4500
#define BACKLOG 10 /* Number of allowed connections */

#define MAX_LOG_EQUIP 100

////////////////////
// Variables list //
////////////////////
int server_port;
pid_t connectMng, powerSupply, elePowerCtrl, powSupplyInfoAccess, logWrite;
int powerSupply_count;
int listen_sock, conn_sock;
const char **use_mode;
int bytes_sent, bytes_received;
struct sockaddr_in server;
struct sockaddr_in client;
int sin_size;
key_t key_s = 6666, key_e = 7777, key_m = 8888; //system info, equip storage, message queue
int shmid_system, shmid_equipment, msqid;		//system info, equip storage, message queue
FILE *log_server;

powsys_t *powsys;	// state  of system
equip_t *equipment; // list of all equip

void sigHandleSIGINT()
{
	msgctl(msqid, IPC_RMID, NULL);
	shmctl(shmid_system, IPC_RMID, NULL);
	shmctl(shmid_equipment, IPC_RMID, NULL);
	fclose(log_server);
	kill(0, SIGKILL);
	exit(0);
}

void connectMng_handle()
{
	///////////////////////
	// Connect to client //
	///////////////////////
	//Step 1: Construct a TCP socket to listen connection request
	if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		tprintf("socket() failed\n");
		exit(1);
	}

	//Step 2: Bind address to socket
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(server_port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(listen_sock, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		tprintf("bind() failed\n");
		exit(1);

		
//  connect_message.c
	
	}
	start_connect_message(make_connect_message(listen_sock, msqid));
}
void elePowerCtrl_handle()
{
	//////////////////////////////
	// Connect to shared memory //
	//////////////////////////////
	if ((equipment = (equip_t *)shmat(shmid_equipment, (void *)0, 0)) == (void *)-1)
	{
		tprintf("shmat() failed\n");
		exit(1);
	}

	if ((powsys = (powsys_t *)shmat(shmid_system, (void *)0, 0)) == (void *)-1)
	{
		tprintf("shmat() failed\n");
		exit(1);
	}

	int i;
	int check_warn_threshold = 0;

	while (1)
	{
		// get total power using
		int sum_temp = 0;
		for (i = 0; i < MAX_EQUIP; i++)
			sum_temp += equipment[i].use_power[equipment[i].mode];
		powsys->current_power = sum_temp;

		// check threshold
		if (powsys->current_power >= POWER_THRESHOLD)
		{
			powsys->supply_over = 1;
			powsys->threshold_over = 1;
		}
		else if (powsys->current_power >= WARNING_THRESHOLD)
		{
			powsys->supply_over = 0;
			powsys->threshold_over = 1;
			// powsys->reset = 0;
		}
		else
		{
			check_warn_threshold = 0;
			powsys->supply_over = 0;
			powsys->threshold_over = 0;
			// powsys->reset = 0;
		}

		// WARN over threshold
		if (powsys->threshold_over && !check_warn_threshold)
		{
			check_warn_threshold = 1;

			// send message to logWrite
			message_t new_msg;
			new_msg.mtype = 1;
			char temp[MAX_MESSAGE_LENGTH];
			sprintf(temp, "WARNING!!! Over threshold, power comsuming: %dW", powsys->current_power);
			tprintf("%s\n", temp);
			sprintf(new_msg.mtext, "s|%s", temp);
			msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);
		}

		// overload
		if (powsys->supply_over)
		{
			// send message to logWrite
			message_t new_msg;
			new_msg.mtype = 1;
			char temp[MAX_MESSAGE_LENGTH];

			sprintf(temp, "DANGER!!! System overload, power comsuming: %dW", powsys->current_power);
			tprintf("%s\n", temp);
			sprintf(new_msg.mtext, "s|%s", temp);
			msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);

			tprintf("Server reset in 10 seconds\n");

			int no;
			for (no = 0; no < MAX_EQUIP; no++)
			{
				if (equipment[no].mode == 1)
				{
					new_msg.mtype = 2;
					sprintf(new_msg.mtext, "m|%d|2|", equipment[no].pid);
					msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);
				}
			}

			pid_t my_child;
			if ((my_child = fork()) == 0)
			{
				// in child
				sleep(5);

				int no;
				for (no = 0; no < MAX_EQUIP; no++)
				{
					if (equipment[no].mode != 0)
					{
						new_msg.mtype = 2;
						sprintf(new_msg.mtext, "m|%d|0|", equipment[no].pid);
						msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);
					}
				}
				kill(getpid(), SIGKILL);
			}
			else
			{
				//in parent
				while (1)
				{
					sum_temp = 0;
					for (i = 0; i < MAX_EQUIP; i++)
						sum_temp += equipment[i].use_power[equipment[i].mode];
					powsys->current_power = sum_temp;

					if (powsys->current_power < POWER_THRESHOLD)
					{
						powsys->supply_over = 0;
						tprintf("OK, power now is %d", powsys->current_power);
						kill(my_child, SIGKILL);
						break;
					}
				}
			}
		}
	} // endwhile
} //end function elePowerCtrl_handle

void logWrite_handle()
{
	// mtype == 1
	message_t got_msg;

	//////////////////////////////
	// Connect to shared memory //
	//////////////////////////////
	if ((equipment = (equip_t *)shmat(shmid_equipment, (void *)0, 0)) == (void *)-1)
	{
		tprintf("shmat() failed\n");
		exit(1);
	}

	if ((powsys = (powsys_t *)shmat(shmid_system, (void *)0, 0)) == (void *)-1)
	{
		tprintf("shmat() failed\n");
		exit(1);
	}

	///////////////////////////
	// Create sever log file //
	///////////////////////////
	char file_name[255];
	time_t t = time(NULL);
	struct tm *now = localtime(&t);
	strftime(file_name, sizeof(file_name), "log/server_%Y-%m-%d_%H:%M:%S.txt", now);
	log_server = fopen(file_name, "w");
	tprintf("Log server started, file is %s\n", file_name);

	///////////////////////////////
	// Listen to other processes //
	///////////////////////////////
	while (1)
	{
		// got mail!
		if (msgrcv(msqid, &got_msg, MAX_MESSAGE_LENGTH, 1, 0) == -1)
		{
			tprintf("msgrcv() error");
			exit(1);
		}

		// header = 's' => Write log to server
		if (got_msg.mtext[0] == 's')
		{
			char buff[MAX_MESSAGE_LENGTH];
			//extract from message
			sscanf(got_msg.mtext, "%*2c%[^|]|", buff);
			// get time now
			char log_time[20];
			strftime(log_time, sizeof(log_time), "%Y/%m/%d_%H:%M:%S", now);
			// write log
			fprintf(log_server, "%s | %s\n", log_time, buff);
		}
	}
} //end function logWrite_handle

int main(int argc, char const *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
		exit(1);
	}
	server_port = atoi(argv[1]);
	printf("SERVER start, PID is %d.\n", getpid());

	///////////////////////////////////////////
	// Create shared memory for power system //
	///////////////////////////////////////////
	if ((shmid_system = shmget(key_s, sizeof(powsys_t), 0644 | IPC_CREAT)) < 0)
	{
		tprintf("shmget() failed\n");
		exit(1);
	}
	if ((powsys = (powsys_t *)shmat(shmid_system, (void *)0, 0)) == (void *)-1)
	{
		tprintf("shmat() failed\n");
		exit(1);
	}
	powsys->current_power = 0;
	powsys->threshold_over = 0;
	powsys->supply_over = 0;
	// powsys->reset = 0;

	/////////////////////////////////////////////
	// Create shared memory for equipment storage//
	/////////////////////////////////////////////
	if ((shmid_equipment = shmget(key_e, sizeof(equip_t) * MAX_EQUIP, 0644 | IPC_CREAT)) < 0)
	{
		tprintf("shmget() failed\n");
		exit(1);
	}
	if ((equipment = (equip_t *)shmat(shmid_equipment, (void *)0, 0)) == (void *)-1)
	{
		tprintf("shmat() failed\n");
		exit(1);
	}

	// Init data for shared memory
	int i;
	for (i = 0; i < MAX_EQUIP; i++)
	{
		equipment[i].pid = 0;
		strcpy(equipment[i].name, "");
		equipment[i].use_power[0] = 0;
		equipment[i].use_power[1] = 0;
		equipment[i].use_power[2] = 0;
		equipment[i].mode = 0;
	}

	//////////////////////////////////
	// Create message queue for IPC //
	//////////////////////////////////
	if ((msqid = msgget(key_m, 0644 | IPC_CREAT)) < 0)
	{
		tprintf("msgget() failed\n");
		exit(1);
	}

	///////////////////
	// Handle Ctrl-C //
	///////////////////
	signal(SIGINT, sigHandleSIGINT);

	///////////////////////////////////
	// start child process in SERVER //
	///////////////////////////////////
	if ((connectMng = fork()) == 0)
	{
		powerSupply_count = 0;
		connectMng_handle();
	}
	else if ((elePowerCtrl = fork()) == 0)
	{
		elePowerCtrl_handle();
	}
	else if ((powSupplyInfoAccess = fork()) == 0)
	{
		start_power_supply_info_access(make_power_supply_info_access(shmid_system, shmid_equipment, msqid));
	}
	else if ((logWrite = fork()) == 0)
	{
		logWrite_handle();
	}
	else
	{
		tprintf("SERVER forked new process connectMng ------------------ pid: %d.\n", connectMng);
		tprintf("SERVER forked new process elePowerCtrl ---------------- pid: %d.\n", elePowerCtrl);
		tprintf("SERVER forked new process powSupplyInfoAccess --------- pid: %d.\n", powSupplyInfoAccess);
		tprintf("SERVER forked new process logWrite -------------------- pid: %d.\n\n", logWrite);
		waitpid(connectMng, NULL, 0);
		waitpid(elePowerCtrl, NULL, 0);
		waitpid(powSupplyInfoAccess, NULL, 0);
		waitpid(logWrite, NULL, 0);
		tprintf("SERVER exited\n\n");
	}

	return 0;
}

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

#include "config.h"
#include "power_supply_info_access.h"
#include "power_supply.h"
#include "message.h"
#include "utils.h"
#include "equipment.h"
#include "use_mode.h"
#include "connect_message.h"
#include "log_write.h"
#include "elec_power_ctrl.h"

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
		time_printf("socket() failed\n");
		exit(1);
	}

	//Step 2: Bind address to socket
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(server_port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(listen_sock, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		time_printf("bind() failed\n");
		exit(1);
	}
	//  connect_message.c
	start_connect_message(make_connect_message(shmid_system, listen_sock, msqid));
}

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
		time_printf("shmget() failed\n");
		exit(1);
	}
	if ((powsys = (powsys_t *)shmat(shmid_system, (void *)0, 0)) == (void *)-1)
	{
		time_printf("shmat() failed\n");
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
		time_printf("shmget() failed\n");
		exit(1);
	}
	if ((equipment = (equip_t *)shmat(shmid_equipment, (void *)0, 0)) == (void *)-1)
	{
		time_printf("shmat() failed\n");
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
		time_printf("msgget() failed\n");
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
		ele_power_ctrl_handle(shmid_equipment, shmid_system, msqid);
	}
	else if ((powSupplyInfoAccess = fork()) == 0)
	{
		start_power_supply_info_access(make_power_supply_info_access(shmid_system, shmid_equipment, msqid));
	}
	else if ((logWrite = fork()) == 0)
	{
		///////////////////////////
		// Create sever log file //
		///////////////////////////
		char file_name[255];
		time_t t = time(NULL);
		struct tm *now = localtime(&t);
		strftime(file_name, sizeof(file_name), "log/server_%Y-%m-%d_%H:%M:%S.txt", now);
		log_server = fopen(file_name, "w");
		printf("Log server started, file is %s\n", file_name);
		log_write_handle(log_server, shmid_equipment, shmid_system, msqid);
	}
	else
	{
		time_printf("SERVER forked new process connectMng ------------------ pid: %d.\n", connectMng);
		time_printf("SERVER forked new process elePowerCtrl ---------------- pid: %d.\n", elePowerCtrl);
		time_printf("SERVER forked new process powSupplyInfoAccess --------- pid: %d.\n", powSupplyInfoAccess);
		time_printf("SERVER forked new process logWrite -------------------- pid: %d.\n\n", logWrite);
		waitpid(connectMng, NULL, 0);
		waitpid(elePowerCtrl, NULL, 0);
		waitpid(powSupplyInfoAccess, NULL, 0);
		waitpid(logWrite, NULL, 0);
		time_printf("SERVER exited\n\n");
	}

	return 0;
}

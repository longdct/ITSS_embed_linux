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

#define MAX_LOG_EQUIP 100

int server_port;
pid_t connect_mng, power_supply, ele_power_ctrl, pow_supply_info_access, log_write;
int power_supply_count;
int listen_sock, conn_sock;
const char **use_mode;
int bytes_sent, bytes_received;
struct sockaddr_in server;
struct sockaddr_in client;
int sin_size;
key_t key_s = 6666, key_e = 7777, key_m = 8888;
int shmid_system, shmid_equipment, msqid;
FILE *log_server;

power_system_t *powsys; // system state
equip_t *equipment;		// list of equipment

void sigHandleSIGINT()
{
	msgctl(msqid, IPC_RMID, NULL);
	shmctl(shmid_system, IPC_RMID, NULL);
	shmctl(shmid_equipment, IPC_RMID, NULL);
	fclose(log_server);
	kill(0, SIGKILL);
	exit(0);
}

void connect_mng_handle()
{
	// Create TCP socket
	if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		time_printf("socket() failed\n");
		exit(1);
	}

	// Bind address to socket
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(server_port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(listen_sock, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		time_printf("bind() failed\n");
		exit(1);
	}

	//  Start listening
	start_connect_message(make_connect_message(shmid_system, listen_sock, msqid));
}

int main(int argc, char const *argv[])
{
	// Get server's listening port
	if (argc != 2)
	{
		fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
		exit(1);
	}
	server_port = atoi(argv[1]);
	printf("SERVER start, PID is %d.\n", getpid());

	// Create shared memory for power system
	if ((shmid_system = shmget(key_s, sizeof(power_system_t), 0644 | IPC_CREAT)) < 0)
	{
		time_printf("shmget() failed\n");
		exit(1);
	}
	if ((powsys = (power_system_t *)shmat(shmid_system, (void *)0, 0)) == (void *)-1)
	{
		time_printf("shmat() failed\n");
		exit(1);
	}
	powsys->current_power = 0;
	powsys->threshold_over = 0;
	powsys->supply_over = 0;

	// Create shared memory for equipment storage
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

	// Initiate data
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

	// Create message queue
	if ((msqid = msgget(key_m, 0666 | IPC_CREAT)) < 0)
	{
		time_printf("msgget() failed\n");
		exit(1);
	}

	// Handle Ctrl-C
	signal(SIGINT, sigHandleSIGINT);

	// Start child processes
	if ((connect_mng = fork()) == 0)
	{
		power_supply_count = 0;
		connect_mng_handle();
	}
	else if ((ele_power_ctrl = fork()) == 0)
	{
		ele_power_ctrl_handle(shmid_equipment, shmid_system, msqid);
	}
	else if ((pow_supply_info_access = fork()) == 0)
	{
		start_power_supply_info_access(make_power_supply_info_access(shmid_system, shmid_equipment, msqid));
	}
	else if ((log_write = fork()) == 0)
	{
		// Create sever log file
		char file_name[255];
		time_t t = time(NULL);
		struct tm *now = localtime(&t);
		strftime(file_name, sizeof(file_name), "log/_%Y-%m-%d_%H:%M:%S.txt", now);
		log_server = fopen(file_name, "w");
		printf("Log server started, file is %s\n", file_name);
		log_write_handle(log_server, shmid_equipment, shmid_system, msqid);
	}
	else
	{
		time_printf("SERVER forked new process connect_mng ------------------ pid: %d.\n", connect_mng);
		time_printf("SERVER forked new process ele_power_ctrl ---------------- pid: %d.\n", ele_power_ctrl);
		time_printf("SERVER forked new process pow_supply_info_access --------- pid: %d.\n", pow_supply_info_access);
		time_printf("SERVER forked new process log_write -------------------- pid: %d.\n\n", log_write);
		waitpid(connect_mng, NULL, 0);
		waitpid(ele_power_ctrl, NULL, 0);
		waitpid(pow_supply_info_access, NULL, 0);
		waitpid(log_write, NULL, 0);
		time_printf("SERVER exited\n\n");
	}

	return 0;
}

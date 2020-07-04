#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include "log_write.h"
#include "message.h"
#include "equipment.h"
#include "power_system.h"

void log_write_handle(FILE *log_server, int shmid_equipment, int shmid_system, int msqid)
{
	// mtype == 1
	message_t got_msg;
	equip_t *equipment;
	powsys_t *powsys;

	//////////////////////////////
	// Connect to shared memory //
	//////////////////////////////
	if ((equipment = (equip_t *)shmat(shmid_equipment, (void *)0, 0)) == (void *)-1)
	{
		printf("shmat() failed\n");
		exit(1);
	}

	if ((powsys = (powsys_t *)shmat(shmid_system, (void *)0, 0)) == (void *)-1)
	{
		printf("shmat() failed\n");
		exit(1);
	}

	///////////////////////////////
	// Listen to other processes //
	///////////////////////////////
	while (1)
	{
		// got mail!
		if (msgrcv(msqid, &got_msg, MAX_MESSAGE_LENGTH, 1, 0) == -1)
		{
			printf("msgrcv() error");
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
			time_t t = time(NULL);
			struct tm *now = localtime(&t);
			strftime(log_time, sizeof(log_time), "%Y/%m/%d_%H:%M:%S", now);
			// write log
			fprintf(log_server, "%s | %s\n", log_time, buff);
		}
	}
} //end function logWrite_handle
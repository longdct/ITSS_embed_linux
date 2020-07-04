#ifndef LOG_WRITE_H
#define LOG_WRITE_H

#include <stdio.h>

void log_write_handle(FILE *log_server, int shmid_equipment, int shmid_system, int msqid);
#endif
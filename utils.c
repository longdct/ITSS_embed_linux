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

#include "utils.h"

int time_printf(const char* format, ...){
	struct tm * cur_time;
	time_t current = time(NULL);
	cur_time = localtime (&current);
	printf("%02d:%02d: Thread %5d \t", cur_time->tm_hour, cur_time->tm_min, getpid());
	
	va_list args;
	va_start(args, format);
	return vprintf(format, args);
}

int file_time_printf(FILE *file, const char* format, ...){
	struct tm * cur_time;
	time_t current = time(NULL);
	cur_time = localtime (&current);
	fprintf(file, "%02d:%02d: Thread %5d \t", cur_time->tm_hour, cur_time->tm_min, getpid());
	
	va_list args;
	va_start(args, format);
	return vfprintf(file, format, args);
}
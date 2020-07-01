#ifndef MESSAGE_H
#define MESSAGE_H

#define MAX_MESSAGE_LENGTH 1000
#define BUFF_SIZE 8192

/**
 * message struct
 * mtype = 1 -> logWrite_handle
 * mtype = 2 -> powSupplyInfoAccess_handle
 */
typedef struct
{
	long mtype;
	char mtext[MAX_MESSAGE_LENGTH];
} message_t;

#endif // MESSAGE_H
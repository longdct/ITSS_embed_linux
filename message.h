#ifndef MESSAGE_H
#define MESSAGE_H

#define MAX_MESSAGE_LENGTH 1000
#define BUFF_SIZE 8192

#define DISCONNECTING_FORMAT "d|%i|"
#define MODE_CHANGING_FORMAT "m|%i|%s|"
#define NEW_EQUIPMENT_FORMAT "n|%i|%s|"

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

message_t *make_message(long mtype, char format[], ...);
char *message_to_string(message_t *mess);
#endif // MESSAGE_H
#ifndef MESSAGE_H
#define MESSAGE_H

#define MAX_MESSAGE_LENGTH 1000
#define BUFF_SIZE 8192

#define W_DISCONNECTING_FORMAT "d|%i|"
#define W_MODE_CHANGING_FORMAT "m|%i|%s|"
#define W_NEW_EQUIPMENT_FORMAT "n|%i|%s|"

#define R_DISCONNECTING_FORMAT "%*c|%i|"
#define R_MODE_CHANGING_FORMAT "%*c|%d|%d|"
#define R_NEW_EQUIPMENT_FORMAT "%*c|%d|%[^|]|%d|%d|"

#define LOG_WRITE_MESS_CODE 1
#define POW_SUP_INF_ACC_MESS_CODE 2
#define ELEC_POWER_CTRL_MESS_CODE 3

typedef struct
{
	long mtype;
	char mtext[MAX_MESSAGE_LENGTH];
} message_t;

message_t *make_message(long mtype, char format[], ...);
char *message_to_string(message_t *mess);
#endif // MESSAGE_H

#ifndef CONNECT_MESSAGE_H
#define CONNECT_MESSAGE_H

typedef struct
{
    int listen_sock;
    int msqid;
} connect_message_t;

connect_message_t *make_connect_message(int listen_sock, int msqid);
void start_connect_message(connect_message_t *msg);

#endif 
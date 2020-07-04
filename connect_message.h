
#ifndef CONNECT_MESSAGE_H
#define CONNECT_MESSAGE_H

typedef struct
{
    int powerSupply_count;
    int listen_sock;
    int msqid;
    int shmid_system;
} connect_message_t;

connect_message_t *make_connect_message(int shmid_system,int listen_sock, int msqid);
void start_connect_message(connect_message_t *connect_message);

#endif 
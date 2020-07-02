#include "message.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

message_t *make_message(long mtype, char format[], ...)
{
    va_list args;
    va_start(args, format);

    message_t *mess = (message_t *)malloc(sizeof(message_t));
    mess->mtype = mtype;
    vsprintf(mess->mtext, format, args);

    va_end(args);
    return mess;
}

char *message_to_string(message_t *mess)
{
    char *str = (char *)malloc(sizeof(char)*MAX_MESSAGE_LENGTH);
    sprintf(str, "Type: %i\t Contents: %s\n", mess->mtype, mess->mtext);
    return str;
}
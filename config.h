#ifndef CONFIG_H
#define CONFIG_H

#include <sys/ipc.h>

#define MAX_EQUIP 10

typedef enum
{
    false,
    true
} bool;

extern int powerSupply_count;

#endif // CONFIG_H
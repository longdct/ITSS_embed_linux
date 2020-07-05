#ifndef CONFIG_H
#define CONFIG_H

#define MAX_EQUIP 10
#define POWER_THRESHOLD 5000
#define WARNING_THRESHOLD 4500

typedef enum
{
    false,
    true
} bool;

extern int power_supply_count;

#endif // CONFIG_H
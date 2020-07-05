#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>

#include "utils.h"
#include "elec_power_ctrl.h"
#include "message.h"
#include "equipment.h"
#include "power_system.h"
#include "utils.h"
#include "use_mode.h"

equip_t *extract_equipment_from_msg(equip_t *equipment_list, int pid)
{
    for (int i = 0; i < MAX_EQUIP; i++)
    {
        if (equipment_list[i].pid == pid)
        {
            return &(equipment_list[i]);
        }
    }
    time_printf("EQUIPMENT NOT FOUND!!!!!!\n");
    exit(1);
    return NULL;
}

int ele_handle_msg(equip_t *equipment_list, int msqid, int current_power)
{
    char name[MAX_EQUIP_NAME];
    int mode;
    int pid;
    int ordinal_use;
    int limited_use;

    message_t mess;
    equip_t *equipment;

    if (msgrcv(msqid, &mess, MAX_MESSAGE_LENGTH, ELEC_POWER_CTRL_MESS_CODE, 0) == -1)
    {
        printf("msgrcv() error");
        exit(1);
    }

    // in case of creating new equip
    if (mess.mtext[0] == 'n')
    {
        sscanf(mess.mtext, R_NEW_EQUIPMENT_FORMAT, &pid, name, &ordinal_use, &limited_use);
        equipment = extract_equipment_from_msg(equipment_list, pid);
        sprintf(mess.mtext, "s|Equipment %s connected. Normal power usage: %dW. Limited power usage: %dW. Current mode: %s", equipment->name, equipment->use_power[1], equipment->use_power[2], mode_to_string(equipment->mode));
    }
    else if (mess.mtext[0] == 'm') // in case of changing mode
    {
        sscanf(mess.mtext, R_MODE_CHANGING_FORMAT, &pid, &mode);
        equipment = extract_equipment_from_msg(equipment_list, pid);
        sprintf(mess.mtext, "s|Equipment %s change mode. Current mode: %s", equipment->name, mode_to_string(equipment->mode));
    }
    else if (mess.mtext[0] == 'd') // in case of disconnection
    {
        sscanf(mess.mtext, R_DISCONNECTING_FORMAT, pid);
        equipment = extract_equipment_from_msg(equipment_list, pid);
        sprintf(mess.mtext, "s|Equipment %s disconnected", equipment->name);
    }

    mess.mtype = LOG_WRITE_MESS_CODE;
    msgsnd(msqid, &mess, MAX_MESSAGE_LENGTH, 0);

    // recalculate power
    int sum_temp = 0;
    for (int i = 0; i < MAX_EQUIP; i++)
        sum_temp += equipment_list[i].use_power[equipment_list[i].mode];
    return sum_temp;
}

void ele_power_ctrl_handle(int shmid_equipment, int shmid_system, int msqid)
{
    equip_t *equipment;
    power_system_t *powsys;

    // Connect to shared memory
    if ((equipment = (equip_t *)shmat(shmid_equipment, (void *)0, 0)) == (void *)-1)
    {
        time_printf("shmat() failed\n");
        exit(1);
    }

    if ((powsys = (power_system_t *)shmat(shmid_system, (void *)0, 0)) == (void *)-1)
    {
        time_printf("shmat() failed\n");
        exit(1);
    }

    int i;
    int check_warn_threshold = 0;

    while (1)
    {
        // get total power using
        int sum_temp = ele_handle_msg(equipment, msqid, powsys->current_power);
        powsys->current_power = sum_temp;

        // check threshold
        if (powsys->current_power >= POWER_THRESHOLD)
        {
            powsys->supply_over = 1;
            powsys->threshold_over = 1;
        }
        else if (powsys->current_power >= WARNING_THRESHOLD)
        {
            powsys->supply_over = 0;
            powsys->threshold_over = 1;
        }
        else
        {
            check_warn_threshold = 0;
            powsys->supply_over = 0;
            powsys->threshold_over = 0;
        }

        // WARN over threshold
        if (powsys->threshold_over && !check_warn_threshold)
        {
            check_warn_threshold = 1;

            // send message to log_write
            message_t new_msg;
            new_msg.mtype = LOG_WRITE_MESS_CODE;
            char temp[MAX_MESSAGE_LENGTH];
            sprintf(temp, "WARNING!!! Over threshold, power comsuming: %dW", powsys->current_power);
            time_printf("%s\n", temp);
            sprintf(new_msg.mtext, "s|%s", temp);
            msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);
        }

        // overload
        if (powsys->supply_over)
        {
            // send message to log_write
            message_t new_msg;
            new_msg.mtype = LOG_WRITE_MESS_CODE;
            char temp[MAX_MESSAGE_LENGTH];

            sprintf(temp, "DANGER!!! System overload, power comsuming: %dW", powsys->current_power);
            time_printf("%s\n", temp);
            sprintf(new_msg.mtext, "s|%s", temp);
            msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);

            time_printf("Server reset in 10 seconds\n");

            int no;
            for (no = 0; no < MAX_EQUIP; no++)
            {
                if (equipment[no].mode == 1)
                {
                    sprintf(new_msg.mtext, "m|%d|2|", equipment[no].pid);

                    new_msg.mtype = LOG_WRITE_MESS_CODE;
                    msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);

                    new_msg.mtype = POW_SUP_INF_ACC_MESS_CODE;
                    msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);
                }
            }

            pid_t my_child;
            if ((my_child = fork()) == 0)
            {
                // in child
                sleep(5);

                int no;
                for (no = 0; no < MAX_EQUIP; no++)
                {
                    if (equipment[no].mode != 0)
                    {
                        sprintf(new_msg.mtext, "m|%d|0|", equipment[no].pid);

                        new_msg.mtype = LOG_WRITE_MESS_CODE;
                        msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);

                        new_msg.mtype = POW_SUP_INF_ACC_MESS_CODE;
                        msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);
                    }
                }
                kill(getpid(), SIGKILL);
            }
            else
            {
                //in parent
                while (1)
                {
                    sum_temp = 0;
                    for (i = 0; i < MAX_EQUIP; i++)
                        sum_temp += equipment[i].use_power[equipment[i].mode];
                    powsys->current_power = sum_temp;

                    if (powsys->current_power < POWER_THRESHOLD)
                    {
                        powsys->supply_over = 0;
                        time_printf("Power now is %d\n", powsys->current_power);
                        kill(my_child, SIGKILL);
                        break;
                    }
                }
            }
        }
    }
}
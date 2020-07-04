#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>

#include "elec_power_ctrl.h"
#include "message.h"
#include "equipment.h"
#include "power_system.h"

void elec_power_ctrl_handle(int shmid_equipment, int shmid_system, int msqid)
{
    equip_t *equipment;
    powsys_t *powsys;

    //////////////////////////////
    // Connect to shared memory //
    //////////////////////////////
    if ((equipment = (equip_t *)shmat(shmid_equipment, (void *)0, 0)) == (void *)-1)
    {
        printf("shmat() failed\n");
        exit(1);
    }

    if ((powsys = (powsys_t *)shmat(shmid_system, (void *)0, 0)) == (void *)-1)
    {
        printf("shmat() failed\n");
        exit(1);
    }

    int i;
    int check_warn_threshold = 0;

    while (1)
    {
        // get total power using
        int sum_temp = 0;
        for (i = 0; i < MAX_EQUIP; i++)
            sum_temp += equipment[i].use_power[equipment[i].mode];
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
            // powsys->reset = 0;
        }
        else
        {
            check_warn_threshold = 0;
            powsys->supply_over = 0;
            powsys->threshold_over = 0;
            // powsys->reset = 0;
        }

        // WARN over threshold
        if (powsys->threshold_over && !check_warn_threshold)
        {
            check_warn_threshold = 1;

            // send message to logWrite
            message_t new_msg;
            new_msg.mtype = 1;
            char temp[MAX_MESSAGE_LENGTH];
            sprintf(temp, "WARNING!!! Over threshold, power comsuming: %dW", powsys->current_power);
            printf("%s\n", temp);
            sprintf(new_msg.mtext, "s|%s", temp);
            msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);
        }

        // overload
        if (powsys->supply_over)
        {
            // send message to logWrite
            message_t new_msg;
            new_msg.mtype = 1;
            char temp[MAX_MESSAGE_LENGTH];

            sprintf(temp, "DANGER!!! System overload, power comsuming: %dW", powsys->current_power);
            printf("%s\n", temp);
            sprintf(new_msg.mtext, "s|%s", temp);
            msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);

            printf("Server reset in 10 seconds\n");

            int no;
            for (no = 0; no < MAX_EQUIP; no++)
            {
                if (equipment[no].mode == 1)
                {
                    new_msg.mtype = 2;
                    sprintf(new_msg.mtext, "m|%d|2|", equipment[no].pid);
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
                        new_msg.mtype = 2;
                        sprintf(new_msg.mtext, "m|%d|0|", equipment[no].pid);
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
                        printf("OK, power now is %d", powsys->current_power);
                        kill(my_child, SIGKILL);
                        break;
                    }
                }
            }
        }
    } // endwhile
} //end function elePowerCtrl_handle
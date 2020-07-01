

void powSupplyInfoAccess() {
	// mtype = 2
	msg_t got_msg;

	//////////////////////////////
	// Connect to shared memory //
	//////////////////////////////
	if ((devices = (device_t*) shmat(shmid_d, (void*) 0, 0)) == (void*)-1) {
		tprintf("shmat() failed\n");
		exit(1);
	}

	if ((powsys = (powsys_t*) shmat(shmid_s, (void*) 0, 0)) == (void*)-1) {
		tprintf("shmat() failed\n");
		exit(1);
	}

	////////////////
	// check mail //
	////////////////
	while(1) {
		// got mail!
		if (msgrcv(msqid, &got_msg, MAX_MESSAGE_LENGTH, 2, 0) <= 0) {
			tprintf("msgrcv() error");
			exit(1);
		}

		// header = 'n' => Create new device
		if (got_msg.mtext[0] == 'n') {
			int no;
			for (no = 0; no < MAX_DEVICE; no++) {
				if (devices[no].pid == 0)
					break;
			}
			sscanf(got_msg.mtext, "%*c|%d|%[^|]|%d|%d|",
				&devices[no].pid,
				devices[no].name,
				&devices[no].use_power[1],
				&devices[no].use_power[2]);
			devices[no].mode = 0;
			tprintf("--- Connected device info ---\n");
			tprintf("       name: %s\n", devices[no].name);
			tprintf("     normal: %dW\n", devices[no].use_power[1]);
			tprintf("      limit: %dW\n", devices[no].use_power[2]);
			tprintf("   use mode: %s\n", use_mode[devices[no].mode]);
			tprintf("-----------------------------\n\n");
			tprintf("System power using: %dW\n", powsys->current_power);

			// send message to logWrite
			msg_t new_msg;
			new_msg.mtype = 1;
			sprintf(new_msg.mtext, "s|[%s] connected (Normal use: %dW, Linited use: %dW)|", 
				devices[no].name, 
				devices[no].use_power[1], 
				devices[no].use_power[2]);
			msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);

			sprintf(new_msg.mtext, "s|Device [%s] set mode to [off] ~ using 0W|", devices[no].name);
			msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);
		}

		// header = 'm' => Change the mode!
		if (got_msg.mtext[0] == 'm') {
			int no, temp_pid, temp_mode;

			sscanf(got_msg.mtext, "%*c|%d|%d|", &temp_pid, &temp_mode);

			for (no = 0; no < MAX_DEVICE; no++) {
				if (devices[no].pid == temp_pid)
					break;
			}
			devices[no].mode = temp_mode;

			// send message to logWrite
			msg_t new_msg;
			new_msg.mtype = 1;
			char temp[MAX_MESSAGE_LENGTH];

			sprintf(temp, "Device [%s] change mode to [%s], comsume %dW", 
				devices[no].name, 
				use_mode[devices[no].mode],
				devices[no].use_power[devices[no].mode]);
			tprintf("%s\n", temp);
			sprintf(new_msg.mtext, "s|%s", temp);
			msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);

			sleep(1);
			sprintf(temp, "System power using: %dW", powsys->current_power);
			tprintf("%s\n", temp);
			sprintf(new_msg.mtext, "s|%s", temp);
			msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);

		}

		// header = 'd' => Disconnect
		if (got_msg.mtext[0] == 'd') {
			int no, temp_pid;
			sscanf(got_msg.mtext, "%*c|%d|", &temp_pid);

			// send message to logWrite
			msg_t new_msg;
			new_msg.mtype = 1;
			char temp[MAX_MESSAGE_LENGTH];
			
			sprintf(temp, "Device [%s] disconnected", devices[no].name);

			for (no = 0; no < MAX_DEVICE; no++) {
				if (devices[no].pid == temp_pid) {
					tprintf("%s\n\n", temp);
					devices[no].pid = 0;
					strcpy(devices[no].name, "");
					devices[no].use_power[0] = 0;
					devices[no].use_power[1] = 0;
					devices[no].use_power[2] = 0;
					devices[no].mode = 0;
					break;
				} else {
					tprintf("Error! Device not found\n\n");
				}
			}
			sprintf(new_msg.mtext, "s|%s", temp);
			msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);

			sprintf(temp, "System power using: %dW", powsys->current_power);
			tprintf("%s\n", temp);
			sprintf(new_msg.mtext, "s|%s", temp);
			msgsnd(msqid, &new_msg, MAX_MESSAGE_LENGTH, 0);
		}

	} // endwhile
} //end function powSupplyInfoAccess

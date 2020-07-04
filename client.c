//////////////////////////////////////////////////////////
//  \    /\   Simple power supply program - client side //
//   )  ( ')                                            //
//  (  /  )                                             //
//   \(__)|                                          N. //
//////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFF_SIZE 8192

int main(int argc, char const *argv[])
{
	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <Server IP> <Echo Port>\n", argv[0]);
		exit(1);
	}

	// Get info for equip
	char name[50];
	int mode_2;
	int mode_3;
	int use_mode;
	printf("Equipment name: ");
	scanf("%s", name);
	printf("Normal power mode: ");
	scanf("%*c%d", &mode_2);
	getchar();
	printf("Limited power mode: ");
	scanf("%d", &mode_3);
	getchar();

	// Step 0: Init variable
	int client_sock;
	char buff[BUFF_SIZE];
	struct sockaddr_in server_addr;
	int msg_len, bytes_sent, bytes_received;

	// Step 1: Construct socket
	client_sock = socket(AF_INET, SOCK_STREAM, 0);

	// Step 2: Specify server address
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2]));
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);

	// Step 3: Request to connect server
	if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
	{
		perror("accept() failed\n");
		exit(1);
	}

	// Step 4: Communicate with server

	// First, send equip info to server
	memset(buff, '\0', strlen(buff) + 1);
	sprintf(buff, "%s|%d|%d", name, mode_2, mode_3);
	msg_len = strlen(buff);
	if (msg_len == 0)
	{
		printf("No info on equip\n");
		close(client_sock);
		exit(1);
	}
	bytes_sent = send(client_sock, buff, msg_len, 0);
	if (bytes_sent <= 0)
	{
		printf("Connection closed\n");
		close(client_sock);
		exit(1);
	}

	// Then, wait for server response
	if (fork() == 0)
	{
		// Child: listen from server
		while (1)
		{
			bytes_received = recv(client_sock, buff, BUFF_SIZE - 1, 0);
			if (bytes_received <= 0)
			{
				// if DISCONNECT
				printf("\nServer shuted down.\n");
				break;
			}
			else
			{
				buff[bytes_received] = '\0';
			}

			int buff_i = atoi(buff);
			// if (buff_i = 9) => max equip reached => quit

			if (buff_i == 9)
			{
				printf("Max number of equipment reached. Cannot connect to server\n");
			}
		}
	}
	else
	{
		// Parent: open menu for user
		do
		{
			sleep(1);
			printf(
				"--------\nPlease choose 0, 1, or 2 to change mode of equipment:\n"
				"0) Turning off\n"
				"1) Normal mode\n"
				"2) Power saving mode\n"
				"Other options to disconnect.\nYour choice: ");

			char menu = getchar();
			getchar();

			switch (menu)
			{
			case '0':
				printf("TURNING OFF\n\n");
				break;
			case '1':
				printf("NORMAL MODE\n\n");
				break;
			case '2':
				printf("POWER SAVING MODE\n\n");
				break;
			default:
				menu = '3';
				printf("DISCONNECTED\n");
			}
			if (menu == '3')
				break;
			send(client_sock, &menu, 1, 0);
		} while (1);
	}

	// Step 5: Close socket
	close(client_sock);
	kill(0, SIGKILL);
	return 0;
}
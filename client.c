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

	char name[50];
	int mode_2;
	int mode_3;
	int use_mode;
	if (argc == 3)
	{
		printf("Equipment Name: ");
		scanf("%s", name);
		printf("Normal Power Usage: ");
		scanf("%*c%d", &mode_2);
		printf("Limited Power Usage: ");
		scanf("%*c%d%*c", &mode_3);
	}
	else if (argc == 6)
	{
		strcpy(name, argv[3]);
		mode_2 = atoi(argv[4]);
		mode_3 = atoi(argv[5]);
	}
	else
	{
		fprintf(stderr, "Usage: %s <Server IP> <Echo Port> or <Server IP> <Echo Port> <Equipment name> <Normal Power Usage> <Limited Power Usage> \n", argv[0]);
		exit(1);
	}

	int client_sock;
	char buff[BUFF_SIZE];
	struct sockaddr_in server_addr;
	int msg_len, bytes_sent, bytes_received;
	client_sock = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2]));
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);

	if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
	{
		perror("Server not available\n");
		exit(1);
	}

	memset(buff, '\0', strlen(buff) + 1);
	sprintf(buff, "%s|%d|%d", name, mode_2, mode_3);
	msg_len = strlen(buff);
	if (msg_len == 0)
	{
		printf("Equipment info not available\n");
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

	if (fork() == 0)
	{
		while (1)
		{
			bytes_received = recv(client_sock, buff, BUFF_SIZE - 1, 0);
			if (bytes_received <= 0)
			{
				printf("\nServer shuted down.\n");
				break;
			}
			else
			{
				buff[bytes_received] = '\0';
			}

			int buff_count = atoi(buff);
			if (buff_count == 9)
			{
				printf("Max number of equipment reached. Cannot connect to server\n");
			}
		}
	}
	else
	{
		do
		{
			sleep(1);
			printf(
				"----- MENU -----\n"
				"0. Turn off\n"
				"1. Normal mode\n"
				"2. Limited mode\n"
				"(Choose 0,1 or 2, others to disconnect): ");

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
				printf("LIMITED MODE\n\n");
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
	close(client_sock);
	kill(0, SIGKILL);
	return 0;
}
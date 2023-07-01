#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "common.h"
#include "helpers.h"

int main(int argc, char *argv[])
{
	int num_clients = 2;

	struct pollfd poll_fds[20];

	char buffer[1551];

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd = -1;
	if (argc != 4)
	{
		printf("\n Usage: %s<ip><port>\n", argv[0]);
		return 1;
	}
	// Parsam port-ul ca un numar
	uint16_t port;
	int rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	// Obtinem un socket TCP pentru receptionarea conexiunilor
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd<0, "socket");

	 // Completăm in serv_addr adresa serverului, familia de adrese si portul
	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);

	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");

	// Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
  	// rulam de 2 ori rapid
	int enable = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	rc = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	DIE(rc<0, "connect");

	//trimit id ul clientului
	send_all(sockfd, argv[1], strlen(argv[1]));

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in
	// multimea poll_fds
	poll_fds[0].fd = sockfd;
	poll_fds[0].events = POLLIN;

	poll_fds[1].fd = STDIN_FILENO;
	poll_fds[1].events = POLLIN;

	while (1)
	{
		rc = poll(poll_fds, num_clients, -1);
		DIE(rc<0, "select");

		for (int i = 0; i < num_clients; i++)
		{
			if (poll_fds[i].revents &POLLIN)
			{
				// am primit un mesaj de pe socketul stdin
				//daca mesajul este exit opresc clientul si inchid socketul
				//altfel, este o comanda de abonare si il trimit la server
				//serverul se ocupa cu parsarea mesajului
				if (poll_fds[i].fd == STDIN_FILENO)
				{
					memset(buffer, 0, DIMBUF);
					fgets(buffer, DIMBUF, stdin);

					if (strstr(buffer, "exit") != NULL)
					{
						close(sockfd);
						return 0;
					}

					rc = send_all(sockfd, buffer, strlen(buffer));
					DIE(rc<0, "sent");
				}
				else
				{
					// am primit un mesaj de la sever
					//citesc mesajul
					//daca este de oprire, inchid clientul si opresc socketul
					//atfel afisez mesajul (mesaj de la UDP parsat)
					struct chat_packet sent_packet;

					rc = recv_all(sockfd, &sent_packet, sizeof(sent_packet));
					DIE(rc<0, "recv");
					sent_packet.message[sent_packet.len] = '\0';

					if (strstr(sent_packet.message, "exit") != NULL)
					{
						close(sockfd);
						return 0;
					}
					else
					{
						printf("%s\n", sent_packet.message);
					}
				}
			}
		}
	}
}
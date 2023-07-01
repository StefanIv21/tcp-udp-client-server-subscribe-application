#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "common.h"
#include "helpers.h"

//functie in care copiez toate datele dintr o structura in alta
void get_all_data(struct client *c1, struct client *c2, int n, int m)
{
	memcpy(c1[n].ID, c2[m].ID, strlen(c2[m].ID));
	c1[n].nr_topics = c2[m].nr_topics;
	c1[n].client_addr = c2[m].client_addr;
	c1[n].verificare = c2[m].verificare;
	for (int k = 0; k < c1[n].nr_topics; k++)
	{
		memcpy(c1[n].topic_struct[k].topic, c2[m].topic_struct[k].topic, sizeof(c2[m].topic_struct[k].topic));
		memcpy(c1[n].topic_struct[k].unsenttopic, c2[m].topic_struct[k].unsenttopic, strlen(c2[m].topic_struct[k].unsenttopic));
		c1[n].topic_struct[k].SF = c2[m].topic_struct[k].SF;
	}
}

//pun datele initiale in structura
void set_zero(struct client *c1, int n)
{
	memset(c1[n].ID, 0, strlen(c1[n].ID));

	c1[n].verificare = 0;
	for (int k = 0; k < c1[n].nr_topics; k++)
	{
		memset(c1[n].topic_struct[k].topic, 0, strlen(c1[n].topic_struct[k].topic));
		memset(c1[n].topic_struct[k].unsenttopic, 0, strlen(c1[n].topic_struct[k].unsenttopic));
		c1[n].topic_struct[k].SF = -1;
	}
	c1[n].nr_topics = 0;

}

int main(int argc, char *argv[])
{
	//dimensiunile initiale pentru structura de client online, respectiv offline
	int dim_clients_online = 50;
	int dim_clients_ofline = 50;
	int dim_topics = 20;
	int rc;
	int tcpsocket, udpsocket;
	int ofline_clients = 0;
	char buffer[DIMBUF];
	int all_clients_online = 5;
	//aloc structurile
	struct client *clients_online = calloc(dim_clients_online,sizeof(struct abonari));
	struct client *clients_ofline = calloc(dim_clients_ofline,sizeof(struct abonari));
	if (clients_online == NULL)
			return -1;
	if (clients_ofline == NULL)
			return -1;

	for (int i = 0; i < dim_clients_online; i++)
	{
		clients_online[i].topic_struct = malloc(dim_topics* sizeof(struct abonari));
		if (clients_online[i].topic_struct == NULL)
			return -1;
		for(int j = 0; j< dim_topics;j++)
		{
			clients_online[i].topic_struct[j].unsenttopic = (char*)malloc(1551 * sizeof(char));	
		}
	}
  	for (int i = 0; i < dim_clients_ofline; i++)
	{
		clients_ofline[i].topic_struct = malloc(dim_topics* sizeof(struct abonari));
		if (clients_ofline[i].topic_struct == NULL)
			return -1;
		for(int j = 0; j< dim_topics;j++)
		{
			clients_ofline[i].topic_struct[j].unsenttopic =(char*) malloc(1551 * sizeof(char));	
		}
	}

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	if (argc != 2)
	{
		printf("Usage: %s<port>\n", argv[0]);
		return 1;
	}

	// Parsam port-ul ca un numar
	uint16_t port;
	rc = sscanf(argv[1], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	// Obtinem un socket TCP,respectiv UDP pentru receptionarea conexiunilor
	tcpsocket = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcpsocket<0, "socket");
	udpsocket = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udpsocket<0, "socket");

	 // Completăm in serv_addr adresa serverului, familia de adrese si portul
	struct sockaddr_in serv_addr;

	// Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
  	// rulam de 2 ori rapid
	int enable = 1;
	if (setsockopt(tcpsocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	 // Asociem adresa serverului cu socketul creat folosind bind
	rc = bind(tcpsocket, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(rc<0, "bind");
	rc = bind(udpsocket, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(rc<0, "bind");

	// Setam socket-ul tcpsocket pentru ascultare
	rc = listen(tcpsocket, MAX_CONNECTIONS);
	DIE(rc<0, "listen");

	struct pollfd poll_fds[20];
	int num_clients = 3;

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in
	// multimea poll_fds
	poll_fds[0].fd = tcpsocket;
	poll_fds[0].events = POLLIN;

	poll_fds[1].fd = udpsocket;
	poll_fds[1].events = POLLIN;

	poll_fds[2].fd = STDIN_FILENO;
	poll_fds[2].events = POLLIN;

	while (1)
	{
		rc = poll(poll_fds, num_clients, -1);
		DIE(rc<0, "poll");

		for (int i = 0; i < num_clients; i++)
		{
			if (poll_fds[i].revents &POLLIN)
			{
				//am primit un mesaj pe socketul udp
				if (poll_fds[i].fd == udpsocket)
				{
					struct sockaddr_in clent_addr;
					socklen_t len = sizeof(clent_addr);
					memset(&clent_addr, 0, sizeof(clent_addr));

					// s-au primit date
          			// asa ca serverul trebuie sa le receptioneze
					char buf[1551];
					int rc = recvfrom(udpsocket, buf, 1551, 0, (struct sockaddr *) &clent_addr, &len);
					DIE(rc<0, "recvfrom");

					//parsez tot continul bufferului pentru a obtine campurile cautate
					memset(buffer, 0, DIMBUF);
					char topic[50];
					uint8_t tip_date;

					char continut[1500];
					memcpy(topic, buf, 50);
					topic[strlen(topic)] = '\0';
					memcpy(&tip_date, buf + 50, 1);
					memcpy(continut, buf + 51, 1500);
					continut[strlen(continut)] = '\0';


					//in functie de tipul de date primit,aplic instructiunile din enuntul temei
					//pun tot mesajul in buffer pentru a l trmite la clienti
					if (tip_date == 0)
					{
						uint32_t number;
						memcpy(&number, continut + 1, 4);
						number = ntohl(number);
						if (continut[0] == 1)
						{
							number = -number;
						}
						sprintf(buffer, "%s:%d - %s - INT - %d", inet_ntoa(clent_addr.sin_addr), ntohs(clent_addr.sin_port), topic, number);
					}
					else if (tip_date == 1)
					{
						uint16_t mod;
						memcpy(&mod, continut, 2);
						sprintf(buffer, "%s:%d - %s - SHORT_REAL - %.2f", inet_ntoa(clent_addr.sin_addr), ntohs(clent_addr.sin_port), topic, (float) ntohs(mod) / 100);
					}
					else if (tip_date == 2)
					{
						uint32_t mod;
						float number;
						uint8_t putere;
						memcpy(&mod, continut + 1, 4);
						putere = continut[5];

						number = (float) ntohl(mod);
						while (putere)
						{
							number /= 10;
							putere--;
						}

						if (continut[0] == 1)
							number = -number;
						sprintf(buffer, "%s:%d - %s - FLOAT - %f", inet_ntoa(clent_addr.sin_addr), ntohs(clent_addr.sin_port), topic, number);
					}
					else if (tip_date == 3)
					{
						sprintf(buffer, "%s:%d - %s - STRING - %s", inet_ntoa(clent_addr.sin_addr), ntohs(clent_addr.sin_port), topic, continut);
					}

					buffer[strlen(buffer)] = '\0';

					//pentru clientii care sunt offline,verific ce topic are SF 1
					//pentru topicurile cu SF 1 ,pun mesajele in vectorul "unsenttopic" pentru a le retine

					for (int j = 0; j < ofline_clients; j++)
					{
						for (int k = 0; k < clients_ofline[j].nr_topics; k++)
						{
							if (strcmp(clients_ofline[j].topic_struct[k].topic, topic) == 0)
							{
								if (clients_ofline[j].topic_struct[k].SF == 1)
								{
									char *name = clients_ofline[j].topic_struct[k].unsenttopic;
									strcat(name, buffer);
									name[strlen(name)] = '\n';
								}
							}
						}
					}

					//pentru clientii care sunt online,verific care client este abonat la topicul trimis de udp
					//pentru cei abonati trimit mesajul pe socket
					for (int j = 5; j < all_clients_online; j++)
					{
						for (int k = 0; k < clients_online[j].nr_topics; k++)
						{
							if (strcmp(clients_online[j].topic_struct[k].topic, topic) == 0)
							{
								rc = send_all(j, buffer, strlen(buffer));
								DIE(rc<0, "recv");
							}
						}
					}
				}
				else if (poll_fds[i].fd == tcpsocket)
				{
					 // a venit o cerere de conexiune pe socketul inactiv (tcp),
          			// pe care serverul o accepta
					struct sockaddr_in cli_addr;
					socklen_t len = sizeof(struct sockaddr_in);

					int newsockfd = accept(tcpsocket, (struct sockaddr *) &cli_addr, &len);
					DIE(newsockfd<0, "accept");


					//pun adresa clientului in structura
					//modific campul verifica (astept sa primesc id clientului)
					clients_online[newsockfd].verificare = 1;
					clients_online[newsockfd].client_addr = cli_addr;


					// se adauga noul socket intors de accept() la multimea descriptorilor
          			// de citire
					poll_fds[num_clients].fd = newsockfd;
					poll_fds[num_clients].events = POLLIN;
					num_clients++;
					//tin evidenta pentru clienti care sunt online
					all_clients_online++;
					//realoc structura daca este cazul
					// if(newsockfd + 1  == dim_clients_online )
					// {
					// 	dim_clients_online = dim_clients_online + 5;
					// 	clients_online =(struct client *)realloc(clients_online,dim_clients_online * sizeof(struct client));
					// 	if (clients_online == NULL)
					// 		return -1;
					// 	for (int i = dim_clients_online - 5; i < dim_clients_online; i++)
					// 	{
					// 		clients_online[i].topic_struct = malloc(dim_topics * sizeof(struct abonari));
					// 		if (clients_online[i].topic_struct == NULL)
					// 			return -1;
					// 	}
						
					// }
				}
				else if (poll_fds[i].fd == STDIN_FILENO)
				{
					//parsez linia din stdin si verific daca mesajul este exit
					//in caz afirmativ opresc clinetii si servarul
					char comanda[10];
					scanf("%s", comanda);
					if (strstr(comanda, "exit") != NULL)
					{
						for (int j = 5; j < all_clients_online; j++)
						{
							if (strcmp(clients_online[j].ID, "") != 0)
							{
								rc = send_all(j, "exit", strlen("exit"));
								DIE(rc<0, "recv");
		
							}
						}
						//dezaloc memoria
						for (int i = 0; i < dim_clients_online; i++)
						{
							free(clients_online[i].topic_struct);
						}
						for (int i = 0; i < dim_clients_ofline; i++)
						{
							free(clients_ofline[i].topic_struct);
						}
						free(clients_online);
						free(clients_ofline);
						close(tcpsocket);
						return 0;
					}
				}
				else
				{
					// s-au primit date pe unul din socketii de client,
         			 // asa ca serverul trebuie sa le receptioneze
					struct chat_packet sent_packet;

					int ret = recv_all(poll_fds[i].fd, &sent_packet, sizeof(sent_packet));
					DIE(ret<0, "recv");
					//extrag mesajul primit
					sent_packet.message[sent_packet.len] = '\0';
					memcpy(buffer, sent_packet.message, sent_packet.len);
					//index pentru structuri
					int index = poll_fds[i].fd;

					if (ret == 0)
					{
						 // conexiunea s-a inchis
						 // copiez datele  clientului  online in structura pentru clientii offline
						printf("Client %s disconnected.\n", clients_online[index].ID);
						get_all_data(clients_ofline, clients_online, ofline_clients, index);
						ofline_clients++;
						//realoc daca este cazul
						// if (ofline_clients  == dim_clients_ofline)
						// {
						// 	dim_clients_ofline = dim_clients_ofline + 5;
						// 	clients_ofline = realloc(clients_ofline,dim_clients_ofline * sizeof (struct client));
						// 	for (int i =dim_clients_ofline-5 ; i < dim_clients_ofline; i++)
						// 	{
						// 		clients_ofline[i].topic_struct = malloc(dim_topics* sizeof(struct abonari));
						// 		if (clients_ofline[i].topic_struct == NULL)
						// 			return -1;
						// 	}
						// 	if(clients_ofline == NULL)
						// 		return -1;
						// }
						//reinitializez datele pentru a putea fi folosit de catre urmatorul client
						set_zero(clients_online, index);

						//inchid socketul
						close(index);

						// se scoate din multimea de citire socketul inchis
						for (int j = i; j < num_clients - 1; j++)
						{
							poll_fds[j] = poll_fds[j + 1];
						}

						num_clients--;
					}
					//daca datele primite sunt id ul clientului 
					else if (clients_online[index].verificare == 1)
					{
						clients_online[index].verificare = 0;
						int ok = 0;
						int ok2 = 0;
						//daca id ul primit se afla deja in clientii care sunt online
						//inchid conexiunea cu socketul actual
						for (int j = 5; j < all_clients_online; j++)
						{
							if ((strcmp(sent_packet.message, clients_online[j].ID) == 0))
							{
								ok = 1;

								printf("Client %s already connected.\n", clients_online[j].ID);
								rc = send_all(index, "exit", 4);
								DIE(rc<0, "recv");
								close(index);
								//se scoate din multimea de citire socketul inchis
								for (int k = i; k < num_clients - 1; k++)
								{
									poll_fds[k] = poll_fds[k + 1];
								}

								num_clients--;
								all_clients_online--;
								break;
							}
						}
						//caut sa vad daca id ul curent se afla pintre cele vechi
						if (ok == 0)
						{
							for (int k = 0; k < ofline_clients; k++)
							{
								if ((strcmp(sent_packet.message, clients_ofline[k].ID) == 0))
								{
									//pun in structura pentru clientul online, datele vechi
									ok2 = 1;
									get_all_data(clients_online, clients_ofline, index, k);

									//se scot datele vechi din vector
									for (int f = k; f < ofline_clients - 1; f++)
									{
										clients_ofline[f] = clients_ofline[f + 1];
									}

									ofline_clients--;

									break;
								}
							}
							//daca clientul nu este unul nou
							//parcurg fiecare topic
							//daca sf este 1,trimit toate mesajele
							if (ok2 == 1)
							{
								printf("New client %s connected from %s:%d\n",
									clients_online[index].ID, inet_ntoa(clients_online[index].client_addr.sin_addr),
									ntohs(clients_online[index].client_addr.sin_port));

								for (int k = 0; k < clients_online[index].nr_topics; k++)
								{
									if (clients_online[index].topic_struct[k].SF == 1)
									{
										char * name = clients_online[index].topic_struct[k].unsenttopic;
										rc = send_all(index, name, strlen(name));
										DIE(rc<0, "recv");
										memset(name, 0, strlen(name));
									}
								}
							}
							//este un client nou
							//ii retin id ul in vector
							else
							{
								strcpy(clients_online[index].ID, sent_packet.message);

								printf("New client %s connected from %s:%d\n",
									clients_online[index].ID, inet_ntoa(clients_online[index].client_addr.sin_addr),
									ntohs(clients_online[index].client_addr.sin_port));
							}
						}
					}
					//am primit o comanda de la client
					else
					{
						//parsez datele primite si verific de care tip este comanda
						struct abonari abonat;
						char comanda[15];
						sscanf(buffer, "%s", comanda);

						if (strcmp(comanda, "subscribe") == 0)
						{
							//daca topicul dorit este deja si are alt sf, atunci il schimb
							int ok = 0;
							sscanf(buffer, "%s%s%d", comanda, abonat.topic, &abonat.SF);
							int nr_topics = clients_online[index].nr_topics;

							for (int j = 0; j < nr_topics; j++)
							{
								if (strcmp(clients_online[index].topic_struct[j].topic, abonat.topic) == 0)
								{
									clients_online[index].topic_struct[j].SF = abonat.SF;
									ok = 1;
									break;
								}
							}
							//daca topicul este unul nou il adaug in structura
							if (ok == 0)
							{
								clients_online[index].topic_struct[clients_online[index].nr_topics] = abonat;
								clients_online[index].nr_topics++;
								//trimit pe socket mesajul de confirmare
								rc = send_all(index, "Subscribed to topic.", strlen("Subscribed to topic."));
								DIE(rc<0, "recv");
								//realoc structura daca este cazul
								// if(clients_online[index].nr_topics == dim_topics)
								// {
								// 	dim_topics = dim_topics + 5;
								// 	for (int i = 0; i < dim_clients_online; i++)
								// 	{
								// 		clients_online[i].topic_struct = realloc(clients_online[i].topic_struct,dim_topics* sizeof(struct abonari));
								// 		if(clients_online[i].topic_struct == NULL)
								// 			return -1;
								// 	}
								// 	for (int i = 0; i < dim_clients_ofline; i++)
								// 	{
								// 		clients_ofline[i].topic_struct = realloc(clients_ofline[i].topic_struct,dim_topics* sizeof(struct abonari));
								// 		if(clients_online[i].topic_struct == NULL)
								// 			return -1;
								// 	}
								// }
							}
						}
						else if (strcmp(comanda, "unsubscribe") == 0)
						{
							sscanf(buffer, "%s%s", comanda, abonat.topic);
							//verific sa vad daca topic se afla in structura
							int nr_topics = clients_online[index].nr_topics;
							for (int j = 0; j < nr_topics; j++)
								if (strcmp(clients_online[index].topic_struct[j].topic, abonat.topic) == 0)
								{
									//se scoate topicul din vector de tip (struct abonati)
									for (int k = j; k < nr_topics - 1; k++)
									{
										clients_online[index].topic_struct[k] = clients_online[index].topic_struct[k + 1];
									}

									clients_online[index].nr_topics--;
								}
							//trimit pe socket mesajul de confirmare
							rc = send_all(index, "Unsubscribed from topic.", strlen("Unsubscribed from topic."));
							DIE(rc<0, "recv");
						}
					}
				}
			}
		}
	}
}
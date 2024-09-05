#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd, n, ret;
	struct sockaddr_in server_addr;
	char buffer[BUFLEN];

	if (argc < 4)
	{
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &server_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	DIE(ret < 0, "connect");

	// Trimit dimensiunea ID-ului la server
	int id_len = strlen(argv[1]);
	n = send(sockfd, &id_len, sizeof(int), 0);
	DIE(n < 0, "send");

	// Trimit ID-ul la server
	n = send(sockfd, argv[1], id_len, 0);
	DIE(n < 0, "send");

	// Multimile de descriptori
	fd_set reads_client, tmp_fds;
	FD_ZERO(&tmp_fds);
	FD_ZERO(&reads_client);
	FD_SET(sockfd, &reads_client);
	FD_SET(STDIN_FILENO, &reads_client);

	int fdmax = sockfd;
	int i;
	int flags = 1;

	// Dezactivez algoritmul lui Neagle
	int neagle = setsockopt(sockfd, SOL_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));
	DIE(neagle < 0, "setscokopt");
	while (1)
	{
		tmp_fds = reads_client;
		char tip_de_date[20];
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		for (i = 0; i <= fdmax; i++)
		{
			if (FD_ISSET(i, &tmp_fds))
			{
				if (i == sockfd)
				{
					// primesc un mesaj de la server

					// Primesc prima data dimensiunea mesajului
					int size;
					n = recv(i, &size, sizeof(int), 0);
					DIE(n < 0, "recv");

					// Primesc apoi tot mesajul
					mesajTCP mesaj;
					int octeti_primiti = 0;
					while (octeti_primiti < size && n != 0)
					{
						n = recv(i, (char*)&mesaj + octeti_primiti, size - octeti_primiti, 0);
						DIE(n < 0, "recv");
						octeti_primiti += n;
					}
					if (n == 0)
					{
						// conexiunea s-a inchis
						close(sockfd);
						// se scoate din multimea de citire socketul inchis
						FD_CLR(sockfd, &reads_client);
						return 0;
					}
					else
					{
						//Am primit mesaj de la server despre un topic
						if (mesaj.tip_date == 0)
						{
							strcpy(tip_de_date, "INT");
						}
						else if (mesaj.tip_date == 1)
						{
							strcpy(tip_de_date, "SHORT_REAL");
						}
						else if (mesaj.tip_date == 2)
						{
							strcpy(tip_de_date, "FLOAT");
						}
						else
						{
							strcpy(tip_de_date, "STRING");
						}
						// Printez in formatul cerut
						printf("%s:%d - %s - %s - %s\n", inet_ntoa(mesaj.sourceUDP.sin_addr), mesaj.sourceUDP.sin_port, mesaj.topic, tip_de_date, mesaj.continut);
					}
				}
				else
				{
					// se citeste de la tastatura
					char *token;
					char command[20];
					char topic[50];
					char SF[2];
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);

					if (strncmp(buffer, "exit", 4) == 0)
					{
						//Daca primesc exit inchid socket-ul si il sterg din multimea de descriptori
						close(sockfd);
						FD_CLR(sockfd, &reads_client);
						return 0;
					}
					//parsez input-ul
					char aux[100];
					strcpy(aux, buffer);
					token = strtok(buffer, " ");
					if (token != NULL)
					{
						strcpy(command, token);
						token = strtok(NULL, " ");
						if (token != NULL)
						{
							strcpy(topic, token);
							if (strcmp("subscribe", command) == 0)
							{
								token = strtok(NULL, "\0");
								if (token != NULL)
								{
									strcpy(SF, token);
								}
							}
						}
					}
					if (strcmp(command, "subscribe") == 0)
					{ // Daca primesc subscribe trimit la server si printez

						// Trimit dimensiunea
						int size = strlen(aux) + 1;
						n = send(sockfd, &size, sizeof(int), 0);
						DIE(n < 0, "send");

						//Trimit comanda
						n = send(sockfd, aux ,size, 0);
						DIE(n < 0, "send");
						printf("Subscribed to topic.\n");
					}
					else if (strcmp(command, "unsubscribe") == 0)
					{
						// Daca primesc unsubscribe trimit la server si printez
						// Trimit dimensiunea
						int size = strlen(aux) + 1;
						n = send(sockfd, &size, sizeof(int), 0);
						DIE(n < 0, "send");
						
						// Trimit comanda
						n = send(sockfd, aux, size, 0);
						DIE(n < 0, "send");
						printf("Unsubscribed from topic.\n");
					} else {
						perror("Invalid input");
					}
					memset(buffer, 0, BUFLEN);
				}
			}
		}
	}

	return 0;
}

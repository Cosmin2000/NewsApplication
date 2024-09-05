#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <math.h>
#include <arpa/inet.h>
#include "helpers.h"

using namespace std;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	int sockfd, new_sock, port;
	char buffer[BUFLEN];
	struct sockaddr_in server_addr, client_addr, client_UDP, server_addr_UDP;
	int n, i, ret;
	socklen_t client_len;

	// multimea de file descriptori de citire
	fd_set read_socks; 
	fd_set tmp_fds;	 
	// file descriptor-ul maxim
	int fdmax;		 

	if (argc < 2)
	{
		usage(argv[0]);
	}

	// Golesc multimea de descriptori de citire <read_socks> si multimea temporara <tmp_fds>
	FD_ZERO(&read_socks);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	//Deschid socket-ul UDP
	int sock_UDP;
	DIE((sock_UDP = socket(AF_INET, SOCK_DGRAM, 0)) == -1, "deschidere socket");

	// Dezactivez algoritmul lui Neagle
	int flags = 1;
	int neagle = setsockopt(sockfd, SOL_TCP, TCP_NODELAY, &flags, sizeof(flags));
	DIE(neagle < 0, "setscokopt");

	port = atoi(argv[1]);
	DIE(port == 0, "atoi");

	// Completez informatiile pentru server pentru clientul TCP
	memset((char *)&server_addr, 0, sizeof(server_addr));
	// Adrese IPv4
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	// Toate adresele IP disponibile
	server_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	memset((char *)&server_addr_UDP, 0, sizeof(server_addr_UDP));
	// Adrese IPv4
	server_addr_UDP.sin_family = AF_INET;
	server_addr_UDP.sin_port = htons(port);
	// Toate adresele IP disponibile
	server_addr_UDP.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sock_UDP, (struct sockaddr *)&server_addr_UDP, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = listen(sockfd, SOMAXCONN);
	DIE(ret < 0, "listen");

	// se adauga socket-ul tcp pasiv, stdin-ul si socket-ul udp in multimea read_socks
	FD_SET(sockfd, &read_socks);
	FD_SET(sock_UDP, &read_socks);
	FD_SET(STDIN_FILENO, &read_socks);
	if (sockfd > sock_UDP)
		fdmax = sockfd;
	else
		fdmax = sock_UDP;
	// Map care are key topic-ul si valoare are lista de abonati
	map<string, vector<Subscriber *>> topics_map;
	// Vector de clienti conectati
	vector<Client *> conn_clients;
	// Vector de clienti deconectati
	vector<Client *> dis_clients;
	// Coada pentru SF
	// Are cheia ID-ul abonatului si valoare are coada de mesaje asociata
	map<string, queue<mesajTCP>> sf_queues;

	while (1)
	{
		tmp_fds = read_socks;
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		for (i = 0; i <= fdmax; i++)
		{
			if (FD_ISSET(i, &tmp_fds))
			{
				if (i == sockfd)
				{
					// Primesc cereri pe socket-ul pasiv
					client_len = sizeof(client_addr);
					new_sock = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
					DIE(new_sock < 0, "accept");

					//Dezactivez algoritmul lui Neagle pe socket-ul nou venit
					int flags1 = 1;
					int neagle = setsockopt(new_sock, IPPROTO_TCP, TCP_NODELAY, &flags1, sizeof(flags1));
					DIE(neagle < 0, "setscokopt");

					
					//Primesc dimensiunea ID-ului
					int id_len;
					n = recv(new_sock, &id_len, sizeof(int), 0);
					DIE(n < 0, "recv");

					// Primesc ID-ul clientului
					memset(buffer, 0, BUFLEN);
					n = recv(new_sock, buffer, id_len, 0);
					DIE(n < 0, "recv");

					// Daca exista deja ID-ul creat
					if (contain(conn_clients, buffer) != -1)
					{
						printf("Client %s already connected.\n", buffer);
						close(new_sock);
					}else {
						
						int index_elem = contain(dis_clients, buffer);
						// Daca s-a reconectat
						if (index_elem != -1)
						{
							// Scot abonatul din lista de clienti deconectati, ii fac status-ul 1(conectat)
							// Ii actualizez socket-ul, il bag in lista de abonati conectati si daca exista
							// o coada asociata lui, ii trimit mesajele din coada.
							Client *elem = dis_clients[index_elem];
							elem->status = 1;
							elem->fd = new_sock;
							conn_clients.push_back(elem);
							dis_clients.erase(dis_clients.begin() + index_elem);
							string id(elem->ID);
							if (sf_queues.find(id) != sf_queues.end())
							{
								while (!sf_queues[id].empty())
								{ // Trimit intai dimensiunea mesajului apoi mesajul
									mesajTCP msj = sf_queues[id].front();
									int size = (int)strlen(msj.continut) + (int)sizeof(msj.topic) + (int)sizeof(msj.tip_date) + (int)sizeof(msj.sourceUDP);
									n = send(new_sock, &size, sizeof(int), 0);
									DIE(n < 0, "send");
									// Trimit apoi mesajul
									int octeti_trimisi = 0;
									while (octeti_trimisi < size)
									{
										n = send(new_sock, (char*)&msj + octeti_trimisi, size - octeti_trimisi, 0);
										DIE(n < 0, "send");
										octeti_trimisi += n;
									}
									sf_queues[id].pop();
								}
							}
							
						}
						else
						{
							// Client nou, deci il adaug in lista de abonati conectati
							conn_clients.push_back(new_sub(new_sock, 0, buffer));
						}
						FD_SET(new_sock, &read_socks);
						if (new_sock > fdmax)
						{
							fdmax = new_sock;
						}
						printf("New client %s connected from %s:%d.\n", buffer,
							   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
					}
				}
				else if (i == sock_UDP)
				{
					// Am primit mesaj de la un client UDP
					// Creez mesajul pentru a-l trimit la abonatii TCP
					mesajTCP msg;
					char *bufferUDP = (char *)malloc(1555 * sizeof(char));
					socklen_t socklen = sizeof(client_UDP);
					ret = recvfrom(sock_UDP, bufferUDP, 1555, 0, (struct sockaddr *)&client_UDP, &socklen);
					DIE(ret < 0, "recvfrom");

					// Verific daca exista deja topic-ul
					string search(bufferUDP);
					if (topics_map.find(search) == topics_map.end())
					{
						// Daca nu exista, il creez
						vector<Subscriber *> vec;
						topics_map.insert({search, vec});
					}
					// Iau tip-ul payload-ului
					uint8_t tip = (uint8_t)(*(bufferUDP + 50));
					msg.sourceUDP = client_UDP;
					msg.tip_date = tip;
					strcpy(msg.topic, bufferUDP);
					//Parsez payload-ul in functie de tip
					if (tip == 0)
					{ // Int
						uint8_t sign = (uint8_t)(*(bufferUDP + 51));
						uint32_t *content = ((uint32_t *)&bufferUDP[52]);
						if (sign == 0)
							sprintf(msg.continut, "%u", ntohl(*content));
						else
							sprintf(msg.continut, "-%u", ntohl(*content));
					}
					else if (tip == 1)
					{ //Short Real
						uint16_t *content = ((uint16_t *)&bufferUDP[51]);
						uint16_t payload = ntohs((*content));
						sprintf(msg.continut, "%.2f", (double)payload / 100);
					}
					else if (tip == 2)
					{ //Float
						uint8_t sign = (uint8_t)(*(bufferUDP + 51));
						uint32_t *content = ((uint32_t *)&bufferUDP[52]);
						uint8_t pwr_of_ten = (uint8_t)(*(bufferUDP + 56));
						uint32_t payload = htonl(*content);
						if (sign == 0)
							sprintf(msg.continut, "%.04f", payload * pow(10, -pwr_of_ten));
						else
							sprintf(msg.continut, "-%.04f", payload * pow(10, -pwr_of_ten));
					}
					else
					{ // String
						char *content = (char *)(bufferUDP + 51);
						sprintf(msg.continut, "%s", content);
					}
					//pentru fiecare abonat de pe topic-ul de la care am primit, trimit mesajul
					vector<Subscriber *> v2;
					v2.assign(topics_map[search].begin(), topics_map[search].end());
					for (int i = 0; i < (int)v2.size(); i++)
					{
						if (v2[i]->sub->status == 1)
						{
							// Daca e conectat
							int size = (int)strlen(msg.continut) + (int)sizeof(msg.topic) + (int)sizeof(msg.tip_date) + (int)sizeof(msg.sourceUDP);
							//Trimit dimensiunea
							n = send(v2[i]->sub->fd, &size, sizeof(int), 0);
							DIE(n < 0, "send");
							// Trimit mesajul
							int octeti_trimisi = 0;
							// Ma asigur ca se trimit toti bytes
							while (octeti_trimisi < size)
							{
								n = send(v2[i]->sub->fd, (char*)&msg + octeti_trimisi, size - octeti_trimisi, 0);
								DIE(n < 0, "send");
								octeti_trimisi += n;
							}
						}
						else
						{
							// Daca abonatul e deconectat
							if (v2[i]->sf == 1)
							{
								// Daca are SF = 1 ii bag in coada mesajul (daca nu are coada o creez)
								string id(v2[i]->sub->ID);
								if (sf_queues.find(id) == sf_queues.end())
								{
									queue<mesajTCP> q;
									q.push(msg);
									sf_queues.insert({id, q});
								}
								else
								{
									sf_queues[id].push(msg);
								}
							}
						}
					}
				}
				else if (i == STDIN_FILENO)
				{
					// Primesc de la tastatura
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);
					int k = 0;
					if (strncmp(buffer, "exit", 4) == 0)
					{
						for (k = 0; k < fdmax + 1; k++)
						{
							if (FD_ISSET(k, &read_socks))
							{ // Inchid toti clienti
								close(k);
								FD_CLR(k, &read_socks);
							}
						}
						// Inchid socket-ul
						close(sockfd);
						return 0;
					}
				}
				else
				{
					// Am primit size-ul
					int size;
					n = recv(i, &size, sizeof(int), 0);
					DIE(n < 0, "recv");
					
					//Primesc comanda doar daca clientul nu se inchide
					if ( n != 0) {
						memset(buffer, 0, BUFLEN);
						n = recv(i, buffer, size, 0);
						DIE(n < 0, "recv");
					}
					

					if (n == 0)
					{
						// Daca s-a inchis inchid socket-ul il scot
						// din lista de clienti conectati si il bag
						// in lista de clienti deconectati
						int index_elem = contain_by_sock(conn_clients, i);
						Client *sub = conn_clients[index_elem];
						printf("Client %s disconnected.\n", sub->ID);
						conn_clients.erase(conn_clients.begin() + index_elem);
						sub->status = 0;
						dis_clients.push_back(sub);
						close(i);
						// il scot din lista de descriptori
						FD_CLR(i, &read_socks);
					}
					else
					{	
						// Daca am primit vreo comanda parsez input-ul
						// Iau primul cuvant(subscribe/unsubscribe)
						char *line = strtok(buffer,"\n");
						if (line != NULL) {
							char *command = strtok(line, " ");
							if (command != NULL)
							{
								// Iau topicul
								char *tpc = strtok(NULL, " \0\n");
								if (tpc != NULL)
								{	
									// Verific daca  topic-ul exista
									// Fac un string din char* pentru a putea cauta in MAP
									string topic_str(tpc);
									if (topics_map.find(topic_str) != topics_map.end())
									{
										// Iau clientul de la care am primit comanda
										int index_elem = contain_by_sock(conn_clients, i);
										Client *new_subscr = conn_clients[index_elem];
										int index_sub = index_of_elem(topics_map[topic_str],i);
										if (strcmp(command, "subscribe") == 0)
										{	// Daca am primit subscribe, adaug clientul in lista
											// de abonati . Daca e deja abonat, afisez la stderr
											char *SF = strtok(NULL, " \n\0");
											if (SF != NULL && index_sub == -1){
												int sf_atoi = atoi(SF);
												DIE(sf_atoi < 0, "atoi");
												topics_map[topic_str].push_back(new_subscriber(new_subscr,sf_atoi));
											} else {
												perror("Connected or invalid input");
											}	
										}
										else
										{
											// Am primit unsubscribe prin urmare, il sterg din lista de abonati a topicului aferent
											// Daca nu e abonat afisez la stderr
											if (index_sub != -1)
												topics_map[topic_str].erase(topics_map[topic_str].begin() + index_sub);
											else 
												perror("Not subscribed");
										}
									}
									else
									{
										// Daca nu exista topic-ul, il creez
										vector<Subscriber *> vec;
										topics_map.insert({topic_str, vec});
									}
								}
							}
						}
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;
}

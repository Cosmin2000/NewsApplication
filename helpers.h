#ifndef _HELPERS_H
#define _HELPERS_H 1

#define BUFLEN		1600	// dimensiunea maxima a calupului de date

#include <stdio.h>
#include <stdlib.h>
#include <netinet/tcp.h>
#include <string.h>
#include <algorithm>
#include <vector>
#include <map>
#include <queue>
#include <string>
#include <netinet/in.h>

using namespace std;

typedef struct Mesaj {
	
	struct sockaddr_in sourceUDP;  // structura socket-ului UDP de la care a venit mesajul
	char topic[50];				   // topic-ul
	uint8_t tip_date;			   // Tip-ul payload-ului
	char continut[BUFLEN];		   // Payload-ul
} __attribute__((packed)) mesajTCP;

typedef struct client {
	int fd;					// socket-ul aferent
	char ID[10];			// ID-ul
	int status;				// 1 - conectat  , 0 - deconectat		
} Client;

typedef struct subscriber { //Structura folosita pentru lista de clienti a unui topic pentru putea retine si SF
 	int sf;					// parametru SF
 	Client *sub;				// Subscriber-ul
} Subscriber;

// Creeaza un subscriber
Subscriber *new_subscriber(Client *sb, int SF);
// Creeaza un nou client
Client *new_sub (int filedesc, int SF, char *ID);
// Cauta elementul cu socket dat. Daca il gaseste returneaza index-ul altfel -1
int index_of_elem(vector<Subscriber*> vec, int sock);
// verifica daca contine clientul. Daca il gaseste returneaza index-ul altfel -1
int contain(vector<Client *> v, char *ID);
// Verifica daca contine abonatul dupa socket. Returneaza index-ul sau -1						  
int contain_by_sock(vector<Client *> v, int sock); 			 
// Verifica daca exista topic-ul. Daca il gaseste returneaza index-ul altfel -1
int exists_topic(map<char *, vector<Client *>> tmap, char *tpc);  

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)



#endif

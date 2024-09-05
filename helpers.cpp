#include "helpers.h"

using namespace std;

Subscriber *new_subscriber(Client *sb, int SF) {
	Subscriber *subscr = (Subscriber*)malloc(sizeof(struct subscriber));
	subscr->sf = SF;
	subscr->sub = sb;
	return subscr;
}
Client *new_sub (int filedesc, int SF, char *ID) {
	Client *subcriber = (Client*)malloc(sizeof(Client));
	subcriber->fd = filedesc;
	strcpy(subcriber->ID,ID);
	subcriber->status = 1;
	return subcriber;
}

int index_of_elem(vector<Subscriber*> vec, int sock) {
	for (int l = 0; l < (int)vec.size();l++){
		if ( vec[l]->sub->fd == sock) {	
			return l;
		}
	}
	return -1;
}

int contain(vector<Client *> v, char *ID)
{
	for (int i = 0; i < (int)v.size(); i++)
	{
		if (strcmp(v[i]->ID, ID) == 0)
		{
			return i;
		}
	}
	return -1;
}

int contain_by_sock(vector<Client *> v, int sock)
{
	for (int i = 0; i < (int)v.size(); i++)
	{
		if (sock == v[i]->fd)
		{
			return i;
		}
	}
	return -1;
}
// Verifica daca exista topic-ul
int exists_topic(map<char *, vector<Client *>> tmap, char *tpc)
{
	for (auto it = tmap.begin(); it != tmap.end(); it++)
	{
		if (strcmp(it->first, tpc) == 0)
		{
			return 1;
		}
	}
	return 0;
}
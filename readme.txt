Grigore Cosmin Mitica 322CC

											
						NEWS APPLICATION

Protocol peste TCP: Am creat o structura, mesajTCP, care are drept campuri: structura sockaddr a clientului UDP de la care a venit mesajul,
topic-ul mesajului, tipul de date, si payload-ul mesajului.
	Pentru a fi eficient, dupa ce construiesc mesajul TCP, ii calculez dimensiunea in bytes si o trimit abonatului(fac un send de sizeof(int)). 
Dupa ce trimit dimensiunea fac o bucla in care trimit doar cati bytes trebuie, adica dimensiunea calculata (fac un while in care fac mai multe 
send-uri pentru a ma asigura ca se trimit toti bytes.)
	Prin urmare, in clientul TCP(cand primesc date de la server) prima data voi face un recv de 4 bytes in care voi astepta dimensiunea, 
iar apoi intr-o bucla in care asteapta exact dimensiunea cunoscuta voi primi mesajul(fac cate recv-uri e nevoie). Dupa ce am primit mesaj-ul 
construiesc in functie de tipul de date un string pentru format-ul de afisare si printez cum este precizat in cerinta. Pentru afisarea 
IP-ului ma folosesc de functia inet_aton.
	Atunci cand trimit comanda de subscribe/unsubscribe de la client la server, trimit prima data dimensiunea string-ului, iar apoi trimit string-ul,
iar in server fac un recv de 4 bytes(sizeof(int)) dupa care inca un recv de dimensiunea primita in care astept string-ul.
	Acelasi procedeu urmaresc si atunci cand trimit ID-ul clientului la server.


Implementare:

	Clientul TCP : 
		In cadrul implementarii clientului TCP, prima data am creat socket-ul cu ajutorul functiei socket, apoi am completat informatiile 
server-ului (Port, IP) pe care le-am primit ca parametru, apoi m-am conectat la server cu ajutorul functiei connect pe socket-ul creat.
	Ca sistem de autentificare am trimis prima data ID-ul primit ca parametru pe care il astept in server.
	Am creat apoi multimile de descriptori de fisier in care am adaugat socket-ul creat si descriptorul stdin-ului. Am dezactivat apoi algoritmul lui Neagle pe
socket-ul TCP creat si apoi intr-o bucla infinita am asteptat date.
	Cu ajutorul functie select am vazut pe care socket imi vine date (multiplexare).
	Daca am primit date pe socket-ul creat inseamna ca am primit mesaj/stire de la server, si respect regulile precizate in Protocolul enuntat mai sus. 
Atunci cand numarul de numarul de octeti primiti este 0, inchid socket-ul ,il scot din lista de file descriptori si inchid clientul.
	Daca nu sunt in niciunul dintre cazurile de mai sus, inseamnca ca primesc comanda de la stdin. 
	Prima data iau input-ul cu ajutorul functiei fgets, iar apoi parsez comanda cu ajutorul functiei strtok. Prima data extrag comanda si topicul,
iar daca comanda este "subscribe" mai aplic o data strtok pentru a extrage SF. Vad apoi daca comanda e de tip subscribe sau unsubscribe. 
Daca e una dintre acestea o trimit la server (trimit prima data dimensiunea apoi string-ul),altfel afisez la stderr: "Invalid Input".

	Server:
		In cadrul implementarii server-ului creez prima data socket-ul TCP pe care il voi face pasiv, apoi socket-ul UDP pentru clientii UDP. 
Dezactivez apoi algoritmul lui Neagle pentru socekt-ul pasiv, completez informatiile pe care astept date (PORT-ul pe care il primesc ca parametru),
iar ca IP las orice IP disponibil(Cu INADDR_ANY).Fac doua structuri de adresa, una pentru socketul TCP si una pentru socket-ul UDP. Fac apoi 
legarea socket-ului UDP si a celui pasiv la server folosind functia bind. Cu ajutorul functiei listen fac socket-ul TCP pasiv.
	Creez apoi multimea de file descriptori de citire(fac inca una temporara fiindca functia select mi-o modifica) in care adaug socket-ul TCP pasiv,
socket-ul UDP si socket-ul stdin-ului.
	Pentru a tine evidenta abonatiilor la topic am facut un MAP din STL care are drept cheie un string reprezentand topic-ul, iar valoare un vector de
pointeri la structura Subscriber. De asemenea, pentru a tine evidenta clientilor deconectati si conectati am facut 2 vectori (din STL) in care retin clientii
conectati, respectiv deconectati, fiindu-mi mai usor pentru implementarea partii de SF. 
	Pentru partea de SF, am construi tot un MAP din STL, care are drept cheie un string reprezentand ID-ul clientului care trebuie sa primeasca mesajele,
ar ca valoare are o coada de mesaje.
	Coada o folosesc tot din STL structuri de mesajTCP.
	Apoi, intr-o bucla infinita, cu ajutorul functiei select vad pe ce socekt imi vin date (Multiplexarea).
	Daca imi vin date pe socket-ul pasiv, inseamna ca am primit o cerere de conectare. Prima data dezactivez algoritmul lui Neagle pentru socket-ul clientului
acceptat, accept realizat cu ajutorul functiei accept, iar apoi astept ID-ul clientul care s-a conectat. Verific daca mai exista un client cu acelasi ID, 
iar daca exista, printez si inchid client-ul acceptat. Altfel, verific daca exista un client deconectat care are acelasi ID, iar daca exista scot clientul din
lista de clienti deconectati , ii actualizez statusul(il fac 1 - "conectat") , socketul, il adaug in lista de clienti conectati, il adaug in lista de file descriptori,
iar daca are o coada asociata ID-ului ii trimit toate mesajele din coada respectand protocolul enuntat mai sus. Daca nu s-a reconectat inseamna ca este client nou,
il adaug in lista de file descriptori, il adaug in lista de clienti conectati si printez.
	Daca imi vin date de la socket-ul UDP inseamnca ca am primit o stire. Cu ajutorul functie recvfrom iau stirea. Daca nu exista deja o intrare in lista de topic-uri
cu topic-ul nou venit, creez un topic fara clienti. In continuare extrag elementele din mesajul UDP conform tabelului prezentat in enunt. Construiesc 
in acelasi timp mesajul TCP, punand tip-ul extras,adresa socket-ului UDP sursa, topic-ul extras din primii 50 de octeti, tip-ul payload-ului (urmatorul octet),
iar continutul mesajului TCP il construiesc ca un string in care pun payload-ul mesajului UDP construit ca in tabel. In continuare, dupa ce construiesc mesajul, 
pentru fiecare abonat al topicului(daca are abonati), daca acesta este conectat trimit mesajul respectand regulile protocolului descris mai sus, iar daca este deconectat
si are SF-ul pe 1, ii adaug mesajul in coada asociata ID-ului. Daca nu are o coada asociata, ii creez una.
	Daca imi vin date de la stdin, verific daca primesc exit, iar daca primesc, inchid fiecare client, apoi socket-ul TCP pasiv si opresc server-ul. 
In acelasi timp, ii scot si din lista de file descriptori de citire.
	Daca nu sunt in niciunul din cazurile de mai sus, inseamna ca primesc date de la un client TCP. Prin urmare, fac un o sa preaiau comanda conform protocolului enuntat. 
Daca numarul de bytes returnati de recv este 0, clientul s-a inchis, prin urmare printez ca in enunt, scot clientul din lista de clienti conectati, il pun in lista de clienti
deconectati actulizandu-i status-ul la "deconectat" si in final ii inchid socket-ul si il sterg din lista de file descriptori de citire.
	Daca clientul nu s-a inchis inseamna ca am primit o comanda, pe care o parsez cu ajutorul functiei Strtok. La inceput scot \n - ul din string
(fiindca eu iau comanda din client cu fgets), iar apoi iau comanda si topic-ul. Verific apoi daca topic-ul primit exista. Daca exista, iau clientul conectat
care are socket-ul pe care am primit comanda, iar daca comanda e "subscribe" iau parametrul SF, iar daca acesta exista, iar clientul nu e deja abonat
creez un abonat nou pe care il adaug in lista de abonati a topicului, altfel printez o eroare la stderr "Connected or invalid input"
	Daca comanda este unsubscribe, scot clientul din lista de abonati doar daca acesta era in lista de abonati a topicului, altfel afisez la stderr eroare "Not subscribed".
	Daca topic-ul nu exista deja, creez unul care are o lista goala de abonati. 
	In server pot primi de la clientul TCP doar subscribe sau unsubscribe.

Alte Observatii:
	Am observat ca dupa o rulare sau 2, checker-ul se blocheaza datorita portului care trebuie schimbat din test.py.
	Am folosit Map, Queue si Vector din STL.
	Pentru implementarea temei m-am ajutat de laboratorul de Multiplexare, laboratorul 8.
	Pentru afisare la stdout am folosit functia printf, iar pentru citire de la stdin am folosit functia fgets.
	Pentru client am facut o structura numita "Client", iar pentru a diferentia abonatul de client am facut inca o structura "Subscriber" care contine un Client
si parametru SF cu care este abonat.
	In fisierul helpers.cpp se afla functiile ajutatoare, iar in helpers.h se afla declararile structurilor.


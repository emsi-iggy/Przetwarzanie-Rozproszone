#include <stdio.h>
#include <time.h>
#include <mpi.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#define ROOT 0
#define max(a,b) ((a) > (b) ? (a) : (b))

void init();
int end = 0;
int size, rank;
int zegarLamporta = 0;
int staryZegarLamporta = 0;
int nowyZegarLamporta = 0;
int ileZajecyPozostalo = 50;
int tagWiadomosci = 1; //1-licencja, 2-zajace, 3-transport

int Licencje = 2;
int Mysliwi = 4;

MPI_Status status;

pthread_mutex_t mutexZegar = PTHREAD_MUTEX_INITIALIZER;

int wiadomosc[10] = {0};
int rozmiarWiadomosci = sizeof(wiadomosc);

/* ************** 
wiadomosc[0] to idProcesu
wiadomosc[1] to zegarLamporta
wiadomosc[2]= {1,2,3} to pytam o licencje,zające,transporty
wiadomosc[3]= {1} to czy zgoda (-1 to nie zgoda)
wiadomosc[4] to ile zajęcy chce zabić
*          ********/
int ileMamZgodLicencja = 0;
int ileMamZgodZajace = 0;
int ileMamZgodTranport = 0;
int procesChceLicencje = 0;

void aktualizujZegar();

void * funkcjaWatku() {
	while(!end) {
		MPI_Recv(&wiadomosc, rozmiarWiadomosci, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		printf("WATEK >> status:%d,(ja:%d) Otrzymalem od procesu: %d, wiadomosc o zegarze[%d], coChce[%d], czyDalemZgode=%d\n",status.MPI_SOURCE,rank,wiadomosc[0],wiadomosc[1],wiadomosc[2],wiadomosc[3]);
		zegarLamporta = max(zegarLamporta, wiadomosc[1]);
		
		if(wiadomosc[3]==1) {
			ileMamZgodLicencja += 1;
		}
		else if(wiadomosc[3]==2) {
			ileMamZgodZajace += 1;
		}
		else if(wiadomosc[3]==3) {
			ileMamZgodTranport += 1;
		}
		else {
			int doKogo = -1;
			if(wiadomosc[2]==1) { //byla prosba o licencje
			printf("WATEK ---> Proces %d pyta mnie %d o licencje, CzyJaChcialemLicencje: %d\n",wiadomosc[0],rank,procesChceLicencje);
				if(!procesChceLicencje) {
					aktualizujZegar();

					doKogo = wiadomosc[0];
					wiadomosc[0] = rank;
					wiadomosc[1] = zegarLamporta;
					wiadomosc[3] = 1;
					MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, doKogo, status.MPI_SOURCE, MPI_COMM_WORLD);

				}
				else {
					if(staryZegarLamporta > wiadomosc[1]) { //proces pyta wczesniej niz ja 
						aktualizujZegar();

						doKogo = wiadomosc[0];
						wiadomosc[0] = rank;
						wiadomosc[1] = zegarLamporta;
						wiadomosc[3] = 1;
						MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, doKogo, status.MPI_SOURCE, MPI_COMM_WORLD);
					}
					else if(zegarLamporta == wiadomosc[1]) {
						if(rank > wiadomosc[0]) {
							aktualizujZegar();

							doKogo = wiadomosc[0];
							wiadomosc[0] = rank;
							wiadomosc[1] = zegarLamporta;
							wiadomosc[3] = 1;
							MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, doKogo, status.MPI_SOURCE, MPI_COMM_WORLD);
						}
					}
					else {
						//wiadomosc[3] = 0;
					//	iadomosc,rozmiarWiadomosci, MPI_INT, wiadomosc[0], status.MPI_SOURCE, MPI_COMM_WORLD);
					}
				
				}
				
			}
		}
		
	}
	return NULL;
}

int main(int argc, char **argv) {

	int provided;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	srand(time(0)+rank); //kazdy ma inny seed
    init();

	//stworz watki
	pthread_t watek1;
	int errno = pthread_create(&watek1, NULL, funkcjaWatku, (void*)&end); //zwraca 0 gdy sukces
	
    /*
        chce licencje:
    */
	procesChceLicencje = (rand() % 101 + 1) > 50;
    if (procesChceLicencje) { //50% szans ze chce licencje, jezeli chce to:
		printf("Proces: %d chce dostac licencje!\n",rank);
		
		aktualizujZegar();

		staryZegarLamporta = zegarLamporta;

		wiadomosc[0] = rank;
		wiadomosc[1] = zegarLamporta;
        wiadomosc[2] = 1;

		
		for(int i=0; i<size; i++) {
			if(i==rank) continue;
			MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, i, status.MPI_SOURCE, MPI_COMM_WORLD);
			printf("Wysylam: (ja: %d) do %d, zegar=%d\n", wiadomosc[0],i,wiadomosc[1]);
		}

		printf("JA:%d czekam na odpowiednia ilosc zgod licencji\n",rank);
		while(ileMamZgodLicencja < Mysliwi - Licencje + 1) {
			//oczekuj na odpowiednia ilosc zgod
		}
		printf("JA:%d Dostalem odpowiednia ilosc zgod: %d i moge wejsc do parku !!\n", rank, ileMamZgodLicencja);


		/*for(int i=0; i<size; i++) {
			if(i==rank) continue;
			MPI_Recv(&wiadomosc, rozmiarWiadomosci, MPI_INT, MPI_ANY_SOURCE, tagWiadomosci, MPI_COMM_WORLD, &status);
			printf("Odbieram: rank=%d, zegar=%d, czySieZgodzil=%d\n", wiadomosc[0],wiadomosc[1],wiadomosc[3]);
		}*/
    }
	else {
		for(int i=0; i<1000000000;i++) {} //sleep
	}


	printf("wyswietlam w mainie\n");
	if(errno) {
		printf("Nie udalo sie utworzyc watku!!\n");
	}

	//polacz z powrotem watki
	errno = pthread_join(watek1, NULL); //zwraca 0 gdy sukces
	if(errno) {
		printf("Nie udalo sie polaczyc watkow!!\n");
	}

	printf("U mnie (%d) jest godzina: %d\n",rank,zegarLamporta);


	MPI_Finalize();
}

void init() {
	if(size < 1290) //(MAX_INT)^(1/3)
		zegarLamporta = rand() % (size*size*size);
    else if(size < 46340) //sqrt(MAX_INT)
	    zegarLamporta = rand() % (size*size);
    else 
        zegarLamporta = rand() % (size);
}

void aktualizujZegar() {
	pthread_mutex_lock(&mutexZegar);
    zegarLamporta += 1;
    pthread_mutex_unlock(&mutexZegar);	
}

void * funkcjaWatku2() {
	while(!end) {
		MPI_Recv(&wiadomosc, rozmiarWiadomosci, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		printf("status:%d, Otrzymalem od procesu: %d, wiadomosc o zegarze[%d], coChce[%d]\n",status.MPI_SOURCE,wiadomosc[0],wiadomosc[1],wiadomosc[2]);
		//if(nowyZegarLamporta < zegarLamporta) = max()
		nowyZegarLamporta = max(zegarLamporta, wiadomosc[1]);
		if(wiadomosc[2]==1) {
			if(zegarLamporta > wiadomosc[1]) { //proces pyta wczesniej niz ja 
				wiadomosc[3] = 1;
				MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, wiadomosc[0], status.MPI_SOURCE, MPI_COMM_WORLD);
			}
			else if(zegarLamporta == wiadomosc[1]) {
				if(rank > wiadomosc[0]) {
					wiadomosc[3] = 1;
					MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, wiadomosc[0], status.MPI_SOURCE, MPI_COMM_WORLD);
				}
				else {
					wiadomosc[3] = 0;
					MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, wiadomosc[0], status.MPI_SOURCE, MPI_COMM_WORLD);
				}
			}
			else {
				wiadomosc[3] = 0;
				//iadomosc,rozmiarWiadomosci, MPI_INT, wiadomosc[0], status.MPI_SOURCE, MPI_COMM_WORLD);
			}
			
		}
	}
	return NULL;
}
void *funkcjaWatku1() {
	//printf("W watku komunikacyjnym:");
	if(wiadomosc[2] != 0) { //ktos cos chce
        pthread_mutex_lock(&mutexZegar);
        zegarLamporta += 1;
        pthread_mutex_unlock(&mutexZegar);
        wiadomosc[0] = rank;
        wiadomosc[1] = zegarLamporta;

		switch(wiadomosc[2]) {
		case 1: //wysylam wszystkim pytanie o licencje
			printf("W watku komunikacyjnym:");
			printf("Ubiegam sie (ja - %d) o wejscie do parku (sekcji krytycznej)\n", rank);
			for(int i=0; i<size; i++) {
				if(i==rank) continue;
				MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, i, status.MPI_SOURCE, MPI_COMM_WORLD);
				printf("Wkomunikacyjny: Wysylam: rank=%d, zegar=%d, cos=%d\n", wiadomosc[0],wiadomosc[1],wiadomosc[2]);
			}
            wiadomosc[2] = 0;
			break;
		case 2: //wysylam wszystkim pytanie o zabicie n zajacow
			printf("W watku komunikacyjnym:");
			printf("Ubiegam sie (ja - %d) o wejscie do parku (sekcji krytycznej)\n", rank);
			int ileZajecyChceZabic = rand() % ileZajecyPozostalo + 1; 
			wiadomosc[4] = ileZajecyChceZabic;
			for(int i=0; i<size; i++) {
				if(i==rank) continue;
				MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, i, status.MPI_SOURCE, MPI_COMM_WORLD);
				printf("komunikacyjny: Wysylam: rank=%d, zegar=%d, cos=%d\n", wiadomosc[0],wiadomosc[1],wiadomosc[2]);
			}
			break;
		case 3: //wysylam wszystkim pytanie o transportt
			for(int i=0; i<size; i++) {
				if(i==rank) continue;
				MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, i, status.MPI_SOURCE, MPI_COMM_WORLD);
				printf("komunikacyjny: Wysylam: rank=%d, zegar=%d, cos=%d\n", wiadomosc[0],wiadomosc[1],wiadomosc[2]);
			}
			break;
		}
	}


    return NULL;
}

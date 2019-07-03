#include <stdio.h>
#include <time.h>
#include <mpi.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define ROOT 0
#define max(a,b) ((a) > (b) ? (a) : (b))

#define Licencje 1
#define Mysliwi 2 //liczba mysliwych = ilosc procesow


int end = 0;
int size, rank;
int zegar = 0;
int staryZegar = 0;
int ileZajecyPozostalo = 50;
int tagWiadomosci = 1; //1-licencja, 2-zajace, 3-transport

MPI_Status status;

pthread_mutex_t mutexZegar = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexZgodyL = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


int wiadomosc[10] = {0};
int wiadomosc2[10] = {0};
int rozmiarWiadomosci = sizeof(wiadomosc);

/* ************** 
wiadomosc[0] to idProcesu
wiadomosc[1] to zegarLamporta
wiadomosc[2]= {1,2,3} to pytam o licencje,zające,transporty
wiadomosc[3]= {1} to czy zgoda (-1 to nie zgoda)
wiadomosc[4] to ile zajęcy chce zabić
wiadomosc[5] = ?
wiadomosc[6] = ?
wiadomosc[7] = ?
wiadomosc[8] = ?
*          ********/

int ileMamZgodLicencja = 0;
int ileMamZgodZajace = 0;
int ileMamZgodTranport = 0;
int procesChceLicencje = 0;

void init();
void aktualizujZegar();
void aktualizujStaryZegar();
void aktualizujZegarReceive(int zegarWiadomosci);
int doKogo;

void * funkcjaWatku() {
	while(!end) {
		wiadomosc2[3]=0;
		MPI_Recv(&wiadomosc2, rozmiarWiadomosci, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		printf("WATEK >> Proces: %d otrzymal od: %d wiadomosc o zegarze: %d, coChce: %d, czyUdzielilemZgody: %d\n",rank,wiadomosc2[0],wiadomosc2[1],wiadomosc2[2],wiadomosc2[3]);
				
		if(wiadomosc2[3]==1) {
			ileMamZgodLicencja += 1;
		}
		else if(wiadomosc2[3]==2) {
			ileMamZgodZajace += 1;
		}
		else if(wiadomosc2[3]==3) {
			ileMamZgodTranport += 1;
		}
        else {
            doKogo = wiadomosc2[0];
            wiadomosc2[3] = 1;
            MPI_Send(wiadomosc2,rozmiarWiadomosci, MPI_INT, doKogo, status.MPI_SOURCE, MPI_COMM_WORLD);
        }
		
	}
	return NULL;
}

void wejscieDoParkuCzyliKolejkaPoLicencje()
{
	wiadomosc[0] = rank;
	wiadomosc[1] = zegar;
	wiadomosc[2] = 1;
    wiadomosc[3] = 0;
	
	for(int i=0; i<size; i++) {
		if(i==rank) continue;
		pthread_mutex_lock(&mutex);
		MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, i, status.MPI_SOURCE, MPI_COMM_WORLD);
		pthread_mutex_unlock(&mutex);
		printf("Wysylam: (ja: %d) do %d, zegar=%d\n", wiadomosc[0],i,wiadomosc[1]);
	}
		
}
int main(int argc, char **argv) {

	int provided;
	MPI_Init_thread(&argc, &argv, 3, &provided);
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	if(rank==0) printf("ROZPOCZYNAM PROGRAM\n");

	srand(time(0)+rank); //kazdy ma inny seed
    //init();

	//stworz watki
	pthread_t watek1;
	int errno = pthread_create(&watek1, NULL, funkcjaWatku, (void*)&end); //zwraca 0 gdy sukces
	
    /*
        chce licencje:
    */
    zegar = rank * 2;
    
    if(rank==0) procesChceLicencje = 1;
    if (procesChceLicencje) { //50% szans ze chce licencje, jezeli chce to:
		printf("Proces: %d chce dostac licencje!\n",rank);

		wejscieDoParkuCzyliKolejkaPoLicencje();

		printf("JA:%d czekam na odpowiednia ilosc zgod licencji\n",rank);
		while(ileMamZgodLicencja < Mysliwi - Licencje + 1) {
			//oczekuj na odpowiednia ilosc zgod
		}
		//aktualizujStaryZegar();
		printf("JA:%d Dostalem odpowiednia ilosc zgod: %d i moge wejsc do parku !!\n", rank, ileMamZgodLicencja);
		while(!end) {

		}

		/*for(int i=0; i<size; i++) {
			if(i==rank) continue;
			MPI_Recv(&wiadomosc, rozmiarWiadomosci, MPI_INT, MPI_ANY_SOURCE, tagWiadomosci, MPI_COMM_WORLD, &status);
			printf("Odbieram: rank=%d, zegar=%d, czySieZgodzil=%d\n", wiadomosc[0],wiadomosc[1],wiadomosc[3]);
		}*/
    }
	else {
		printf("Ja: %d Nie chcialem licencji wiec ide sleep\n", rank);
		while(!end) {

		}
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

	printf("U mnie (%d) jest godzina: %d\n",rank,zegar);


	MPI_Finalize();
}

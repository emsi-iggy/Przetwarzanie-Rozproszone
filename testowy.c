#include <stdio.h>
#include <time.h>
#include <mpi.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#define ROOT 0
#define max(a,b) ((a) > (b) ? (a) : (b))
void check_thread_support(int provided);

int L = 50;
int M = 50;
int R = 50;
int T = 50;
//int ileZajecyPozostalo = R;
int zegarLamporta = 0;

pthread_mutex_t mutexZegar = PTHREAD_MUTEX_INITIALIZER;

struct thread_data_t
{
	int pole;
};

int wiadomosc[10] = {0};
/* ************** 
wiadomosc[0] to idProcesu
wiadomosc[1] to zegarLamporta
wiadomosc[2]= {1,2,3} to pytam o licencje,zające,transporty
wiadomosc[3]= {}to idProcesu
wiadomosc[4] to ile zajęcy chce zabić

*********/
int rozmiarWiadomosc = sizeof(wiadomosc);

int updateZegara() {
	pthread_mutex_lock(&mutexZegar);
	zegarLamporta += 1;
	pthread_mutex_unlock(&mutexZegar);
}

void wyslijPytanieOZgodePoLicencje() {

	pthread_mutex_lock(&mutexZegar);
	zegarLamporta += 1;
	pthread_mutex_unlock(&mutexZegar);
	wiadomosc[0] = rank;
	wiadomosc[1] = zegarLamporta;

	if(wiadomosc[2] != 0) {
		switch(wiadomosc[2]):
		case 1: //wysylam wszystkim pytanie o licencje
			printf("Ubiegam sie (ja - %d) o wejscie do parku (sekcji krytycznej)", rank);
			for(int i=0; i<size; i++) {
				if(i==rank) continue;
				MPI_Send(wiadomosc,rozmiarWiadomosc, MPI_INT, i, status.MPI_SOURCE, MPI_COMM_WORLD);
				printf("Wysylam: rank=%d, zegar=%d, cos=%d\n", wiadomosc[0],wiadomosc[1],wiadomosc[2]);
			}
			break;
		case 2: //wysylam wszystkim pytanie o zabicie n zajacow
			printf("Ubiegam sie (ja - %d) o wejscie do parku (sekcji krytycznej)", rank);
			int ileZajecyChceZabic = rand() % ileZajecyPozostalo + 1; 
			wiadomosc[4] = ileZajecyChceZabic;
			for(int i=0; i<size; i++) {
				if(i==rank) continue;
				MPI_Send(wiadomosc,rozmiarWiadomosc, MPI_INT, i, status.MPI_SOURCE, MPI_COMM_WORLD);
				printf("Wysylam: rank=%d, zegar=%d, cos=%d\n", wiadomosc[0],wiadomosc[1],wiadomosc[2]);
			}
			break;
		case 3: //wysylam wszystkim pytanie o transportt
			for(int i=0; i<size; i++) {
				if(i==rank) continue;
				MPI_Send(wiadomosc,rozmiarWiadomosc, MPI_INT, i, status.MPI_SOURCE, MPI_COMM_WORLD);
				printf("Wysylam: rank=%d, zegar=%d, cos=%d\n", wiadomosc[0],wiadomosc[1],wiadomosc[2]);
			}
			break;
	}
}

void odbierzWiadomoscPoLicencje(bool chce) {
	MPI_Recv(&wiadomosc, rozmiarWiadomosc, MPI_INT, i, MPI_ANY_SOURCE ,MPI_COMM_WORLD, &status);
	zegarLamporta = max(zegarLamportam, wiadomosc[1]);
	if(!chce) {
		wiadomosc[2] = 1;
		MPI_Send(wiadomosc,rozmiarWiadomosc, MPI_INT, i, status.MPI_SOURCE, MPI_COMM_WORLD);
	}
}

void wyslijZgodePoLicencje(bool chce) {
	if(!chce) {
		
	}
}

void *funkcjaWatku1(void * tab) {
	int x[10] = tab;
	while(!end) {
		printf("Wyswietlam w innym watku (end=%d)!\n",end);
	}
	
	return NULL;
}

int main(int argc, char **argv) {

	int provided;
	MPI_Status status;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	
	int size, rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    	MPI_Comm_size(MPI_COMM_WORLD, &size);

	L = size;
	if(rank==ROOT) check_thread_support(provided);
	
	srand(time(0)+rank); //kazdy ma inny seed
	int zegarLamporta = rand() % (size*size*size);
	
	//stworz watki
	pthread_t watek1;
	int errno = pthread_create(&watek1, NULL, funkcjaWatku1, (void*)&end); //zwraca 0 gdy sukces
	
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

	wiadomosc[0] = rank;
	wiadomosc[1] = zegarLamporta;
	wiadomosc[2] = 1; //pytam o zgode po licencje

	bool chceLicencje = (rand() % 101 + 1) > 50;
	if(chceLicencje) {
		wyslijPytanieOZgodePoLicencje(wiadomosc);
	}
	odbierzWiadomoscPoLicencje(chceLicencje);
	
	for(int i=0; i<size; i++) {
		

		if(i==rank) continue;
		MPI_Send(wiadomosc,rozmiarWiadomosc, MPI_INT, i, status.MPI_SOURCE, MPI_COMM_WORLD);
		printf("Wysylam: rank=%d, zegar=%d, cos=%d\n", wiadomosc[0],wiadomosc[1],wiadomosc[2]);
	}

	for(int i=0; i<size; i++) {
		if(i==rank) continue;
		MPI_Recv(&wiadomosc, rozmiarWiadomosc, MPI_INT, i, MPI_ANY_SOURCE ,MPI_COMM_WORLD, &status);
		printf("Odebralem: rank=%d, zegar=%d, cos=%d\n", wiadomosc[0],wiadomosc[1],wiadomosc[2]);
	}
	/*
	if(rank == ROOT) {
		MPI_Send(wiadomosc,rozmiarWiadomosc, MPI_INT, 1, status.MPI_SOURCE, MPI_COMM_WORLD);
		printf("Wysylam: rank=%d, zegar=%d, cos=%d\n", wiadomosc[0],wiadomosc[1],wiadomosc[2]);

	}
	else{
		MPI_Recv(&wiadomosc, rozmiarWiadomosc, MPI_INT, 0, MPI_ANY_SOURCE ,MPI_COMM_WORLD, &status);
		printf("Odebralem: rank=%d, zegar=%d, cos=%d\n", wiadomosc[0],wiadomosc[1],wiadomosc[2]);
	}*/

	
		

	MPI_Finalize();
}

void check_thread_support(int provided)
{
	printf("Thread support: %d  ", provided);
	switch(provided) {
		case MPI_THREAD_SINGLE:
			printf("Brak wsparcia dla watkow \n");
		fprintf(stderr, "Brak wystarczajacego wsparcia dla watkow - wychodze!\n");
		MPI_Finalize();
		exit(-1);
		break;
		case MPI_THREAD_FUNNELED:
			printf("tylko te watki, ktore wykonaly mpi_init_thread moga wykonac wolania do biblioteki mpi\n");
			break;
		case MPI_THREAD_SERIALIZED:
			printf("tylko jeden watek naraz moze wykonac wywolanie do biblioteki MPI\n");
			break;
		case MPI_THREAD_MULTIPLE:
			printf("Pelne wsparcie dla watkow :) \n");
			break;
		default:
			printf("Nikt nic nie wie :/");
	}
}

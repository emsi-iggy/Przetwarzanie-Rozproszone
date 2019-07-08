#include <stdio.h>
#include <time.h>
#include <mpi.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define ROOT 0
#define max(a,b) ((a) > (b) ? (a) : (b))

#define Licencje 2
#define Mysliwi 4 //liczba mysliwych = ilosc procesow
#define Zajace 10

int end = 0;
int size, rank;
int zegar = 0;
int staryZegar = 0;
int ileZajecyPozostalo = Zajace;
int tagWiadomosci = 1; //1-licencja, 2-zajace, 3-transport

MPI_Status status;

pthread_mutex_t mutexZegar = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexZgodyL = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexZgodyZ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexZgodyT = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexIleZajecyZostalo = PTHREAD_MUTEX_INITIALIZER;

int ktoJestWParku[Licencje] = {0};
int wiadomosc[10] = {0};
int wiadomosc2[10] = {0};
int rozmiarWiadomosci = sizeof(wiadomosc);

/* **************
 wiadomosc[0] to idProcesu
 wiadomosc[1] to zegarLamporta
 wiadomosc[2]= {1,2,3} to pytam o licencje,zające,transporty
 wiadomosc[3]= {1} to czy zgoda licencja, 2 to zgoda zajace, itd(to nie zgoda)
 wiadomosc[4] to ile zajęcy chce zabić
 wiadomosc[5] = aktualna liczba zajecy
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

void * funkcjaWatku() {
    int wiadomosc2[10] = {0};
    while(!end) {
        wiadomosc2[3]=0;
        MPI_Recv(&wiadomosc2, rozmiarWiadomosci, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        printf("WATEK >> Proces: %d otrzymal od: %d wiadomosc o zegarze: %d, coChce: %d, czyUdzielilemZgody: %d\n",rank,wiadomosc2[0],wiadomosc2[1],wiadomosc2[2],wiadomosc2[3]);
        //printf("WATEK >> status:%d,(ja:%d, mamZgod:%d) Otrzymalem od procesu: %d, wiadomosc o zegarze[%d], coChce[%d], czyDalemZgode=%d\n",status.MPI_SOURCE,rank,ileMamZgodLicencja,wiadomosc2[0],wiadomosc2[1],wiadomosc2[2],wiadomosc2[3]);
        
        //aktualizujZegarReceive(wiadomosc2[1]);
        
        if(wiadomosc2[3]==1) {
            if(ileMamZgodLicencja != -1)
            {
                pthread_mutex_lock(&mutexZgodyL);
                ileMamZgodLicencja += 1;
                pthread_mutex_unlock(&mutexZgodyL);
                //printf("DOSTAL ZGODE: %d ma teraz %d zgod\n",rank,ileMamZgodLicencja);
            }
            else {
                //printf("Proces juz ma odpowiednia ilosc licencji i ignoruje te wiadomosc\n");
            }
        }
        else if(wiadomosc2[3]==2) {
            pthread_mutex_lock(&mutexZgodyZ);
            ileMamZgodZajace += 1;
            pthread_mutex_lock(&mutexZgodyZ);
            printf("DOSTAL ZGODE: %d ma teraz %d zgod\n",rank,ileMamZgodZajace);
        }
        else if(wiadomosc2[3]==3) {
            pthread_mutex_lock(&mutexZgodyT);
            ileMamZgodTranport += 1;
            pthread_mutex_lock(&mutexZgodyT);
        }
        else {
            int doKogo = -1;
            if(wiadomosc2[2]==1) { //byla prosba o licencje
                //printf("WATEK ---> Proces %d pyta mnie %d o licencje, CzyJaChcialemLicencje: %d\n",wiadomosc2[0],rank,procesChceLicencje);
                if(!procesChceLicencje) {
                    aktualizujZegar();
                    
                    doKogo = wiadomosc2[0];
                    wiadomosc2[0] = rank;
                    wiadomosc2[1] = zegar;
                    wiadomosc2[2] = 0;
                    wiadomosc2[3] = 1;
                    MPI_Send(wiadomosc2,rozmiarWiadomosci, MPI_INT, doKogo, status.MPI_SOURCE, MPI_COMM_WORLD);
                    //printf("WATEK ===> Proces: %d wysyla do: %d wiadomosc z zegarem: %d i czyJestZgoda: %d\n",rank,doKogo,zegar,wiadomosc2[3]);
                    
                }
                else {
                    if(staryZegar > wiadomosc2[1]) { //proces pyta wczesniej niz ja
                        aktualizujZegar();
                        
                        doKogo = wiadomosc2[0];
                        wiadomosc2[0] = rank;
                        wiadomosc2[1] = zegar;
                        wiadomosc2[2] = 0;
                        wiadomosc2[3] = 1;
                        MPI_Send(wiadomosc2,rozmiarWiadomosci, MPI_INT, doKogo, status.MPI_SOURCE, MPI_COMM_WORLD);
                    }
                    else if(zegar == wiadomosc2[1]) {
                        if(rank > wiadomosc2[0]) {
                            aktualizujZegar();
                            
                            doKogo = wiadomosc2[0];
                            wiadomosc2[0] = rank;
                            wiadomosc2[1] = zegar;
                            wiadomosc2[2] = 0;
                            wiadomosc2[3] = 1;
                            MPI_Send(wiadomosc2,rozmiarWiadomosci, MPI_INT, doKogo, status.MPI_SOURCE, MPI_COMM_WORLD);
                        }
                    }
                    else {
                        //wiadomosc2[3] = 0;
                        //    iadomosc,rozmiarWiadomosci, MPI_INT, wiadomosc2[0], status.MPI_SOURCE, MPI_COMM_WORLD);
                    }
                    
                }
                
            }
            //tu
            else if(wiadomosc2[2]==2) { //byla prosba o zajaca
                //printf("WATEK ---> Proces %d pyta mnie %d o licencje, CzyJaChcialemLicencje: %d\n",wiadomosc2[0],rank,procesChceLicencje);
                
                if(ktoJestWParku[rank]==1) {
                    if(wiadomosc2[5]!=0) //pozostaly jakies zajace
                    {
                        pthread_mutex_lock(&mutexIleZajecyZostalo);
                        ileZajecyPozostalo = wiadomosc2[5];
                        pthread_mutex_unlock(&mutexIleZajecyZostalo);
                        printf("Liczba pozostalych robozajecy wynosi: %d\n",ileZajecyPozostalo);
                    }
                    if(staryZegar > wiadomosc2[1]) { //proces pyta wczesniej niz ja
                        aktualizujZegar();
                        
                        doKogo = wiadomosc2[0];
                        wiadomosc2[0] = rank;
                        wiadomosc2[1] = zegar;
                        wiadomosc2[2] = 0;
                        wiadomosc2[3] = 2;
                        MPI_Send(wiadomosc2,rozmiarWiadomosci, MPI_INT, doKogo, status.MPI_SOURCE, MPI_COMM_WORLD);
                    }
                    else if(zegar == wiadomosc2[1]) {
                        if(rank > wiadomosc2[0]) {
                            aktualizujZegar();
                            
                            doKogo = wiadomosc2[0];
                            wiadomosc2[0] = rank;
                            wiadomosc2[1] = zegar;
                            wiadomosc2[2] = 0;
                            wiadomosc2[3] = 2;
                            MPI_Send(wiadomosc2,rozmiarWiadomosci, MPI_INT, doKogo, status.MPI_SOURCE, MPI_COMM_WORLD);
                        }
                    }
                    else {
                        //wiadomosc2[3] = 0;
                        //    iadomosc,rozmiarWiadomosci, MPI_INT, wiadomosc2[0], status.MPI_SOURCE, MPI_COMM_WORLD);
                    }
                }
            }
        }
        
    }
    return NULL;
}

void wejscieDoParkuCzyliKolejkaPoLicencje()
{
    aktualizujZegar();
    
    staryZegar = zegar;
    
    wiadomosc[0] = rank;
    wiadomosc[1] = zegar;
    wiadomosc[2] = 1;
    
    for(int i=0; i<size; i++) {
        if(i==rank) continue;
        pthread_mutex_lock(&mutex);
        MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, i, status.MPI_SOURCE, MPI_COMM_WORLD);
        pthread_mutex_unlock(&mutex);
        printf("Wysylam: (ja: %d) do %d, zegar=%d\n", wiadomosc[0],i,wiadomosc[1]);
    }
    
}

void zabijanieZajecyCzyliKolejkaPoZajaca()
{
    aktualizujZegar();
    
    staryZegar = zegar;
    
    wiadomosc[0] = rank;
    wiadomosc[1] = zegar;
    wiadomosc[2] = 2;
    
    for(int i=0; i<size; i++) {
        if(i==rank) continue;
        pthread_mutex_lock(&mutex);
        MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, i, status.MPI_SOURCE, MPI_COMM_WORLD);
        pthread_mutex_unlock(&mutex);
        printf("Wysylam: (ja: %d) do %d, zegar=%d\n", wiadomosc[0],i,wiadomosc[1]);
    }
    
}

void wyslijInfoKtoJestWParku() {
    //ktoJestWParku
}

void wyslijZgodeWszystkim(int oCoZgoda) {
    aktualizujZegar();
    
    staryZegar = zegar;
    
    wiadomosc[0] = rank;
    wiadomosc[1] = zegar;
    wiadomosc[2] = 0;
    wiadomosc[3] = oCoZgoda;
    wiadomosc[5] = ileZajecyPozostalo;
    
    for(int i=0; i<size; i++) {
        if(i==rank) continue;
        pthread_mutex_lock(&mutex);
        MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, i, status.MPI_SOURCE, MPI_COMM_WORLD);
        pthread_mutex_unlock(&mutex);
        //printf("Wysylam: (ja: %d) do %d, zegar=%d\n", wiadomosc[0],i,wiadomosc[1]);
    }
}

int main(int argc, char **argv) {
    
    int provided;
    MPI_Init_thread(&argc, &argv, 3, &provided);
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if(rank==0) printf("ROZPOCZYNAM PROGRAM\n");
    
    srand(time(0)+rank); //kazdy ma inny seed
    init();
    
    //stworz watki
    pthread_t watek1;
    int errno = pthread_create(&watek1, NULL, funkcjaWatku, (void*)&procesChceLicencje); //zwraca 0 gdy sukces
    
    /*
     chce licencje:
     */
    pthread_mutex_lock(&mutexZgodyL);
    procesChceLicencje = (rand() % 101 + 1) > 50;
    pthread_mutex_unlock(&mutexZgodyL);
    
    if (procesChceLicencje) { //50% szans ze chce licencje, jezeli chce to:
        printf("Proces: %d chce dostac licencje!\n",rank);
        
        wejscieDoParkuCzyliKolejkaPoLicencje();
        
        printf("JA:%d czekam na odpowiednia ilosc zgod licencji\n",rank);
        while(ileMamZgodLicencja < (Mysliwi - Licencje + 1) - 1) {
            //oczekuj na odpowiednia ilosc zgod
        }
        aktualizujStaryZegar();
        printf("JA:%d Dostalem odpowiednia ilosc zgod: %d i moge wejsc do parku !!\n", rank, ileMamZgodLicencja);
        
        pthread_mutex_lock(&mutexZgodyL);
        procesChceLicencje = -1;
        pthread_mutex_unlock(&mutexZgodyL);
        
        printf("-----------------------Proces:%d jest w parku---------------------\n",rank);
        
        ktoJestWParku[rank] = 1;
        
        int liczbaZajecyDoZabicia = rand() % Zajace + 1;
        sleep(1); //symulacja polowania
        zabijanieZajecyCzyliKolejkaPoZajaca();
        
        printf("JA:%d chce ZABIC %d zajecy. Czekam na odpowiednia ilosc zgod zajace\n",rank,liczbaZajecyDoZabicia);
        while(ileMamZgodZajace < Licencje - 1) {
            //oczekuj na odpowiednia ilosc zgod
        }
        aktualizujStaryZegar();
        
        pthread_mutex_lock(&mutexIleZajecyZostalo);
        ileZajecyPozostalo--;
        pthread_mutex_unlock(&mutexIleZajecyZostalo);
        
        printf("JA:%d Dostalem odpowiednia ilosc zgod: %d i moge zabic 1 zajaca. IleZajecyPozostalo:%d !!!!!!!!\n", rank, ileMamZgodZajace,ileZajecyPozostalo);
        
        
        wyslijZgodeWszystkim(2);
        

        while(ileZajecyPozostalo > 0) {
        
        }
        
        /*for(int i=0; i<size; i++) {
         if(i==rank) continue;
         MPI_Recv(&wiadomosc, rozmiarWiadomosci, MPI_INT, MPI_ANY_SOURCE, tagWiadomosci, MPI_COMM_WORLD, &status);
         printf("Odbieram: rank=%d, zegar=%d, czySieZgodzil=%d\n", wiadomosc[0],wiadomosc[1],wiadomosc[3]);
         }*/
    }
    else {
        ktoJestWParku[rank] = -1;
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

void init() {
    if(size < 1290) //(MAX_INT)^(1/3)
        zegar = rand() % (size*size*size);
    else if(size < 46340) //sqrt(MAX_INT)
        zegar = rand() % (size*size);
    else
        zegar = rand() % (size);
}

void aktualizujZegar() {
    pthread_mutex_lock(&mutexZegar);
    zegar += 1;
    pthread_mutex_unlock(&mutexZegar);
}

void aktualizujZegarReceive(int zegarWiadomosci) {
    pthread_mutex_lock(&mutexZegar);
    zegar = max(zegar, zegarWiadomosci);
    pthread_mutex_unlock(&mutexZegar);
}

void aktualizujStaryZegar() {
    pthread_mutex_lock(&mutexZgodyL);
    staryZegar = zegar;
    pthread_mutex_unlock(&mutexZgodyL);
}

#include <stdio.h>
#include <time.h>
#include <mpi.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define ROOT 0
#define max(a,b) ((a) > (b) ? (a) : (b))
#define abs(x) ((x) < (0) ? (-x) : (x))

#define TAGzajac 100
#define TAGzwykly 101
#define Mysliwi 4    //liczba mysliwych = ilosc procesow
#define Zajace 1000
int Licencje = 2;
#define Transport 1

int ileZajecyPozostalo = Zajace;
int end = 0;
int size, rank;
int zegar = 0;
int zegarRequest = 0; //zegar z czasem wyslania komunikatu
int zapisalemZegarReqL = 0;
int zapisalemZegarReq = 0;
int zapisalemZegarReqT = 0;
int zgodaOdWszystkich = 0;
int staryZegar = 0;
int tagWiadomosci = 1; //1-licencja, 2-zajace, 3-transport
int opuszczamPark = 0;
int ileOsobJestWParku = 0;
int liczbaZajecyDoZabicia = 0;
int wyslijack = 0;
MPI_Status status;

pthread_mutex_t mutexZegar = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexZgodyL = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexZgodyZ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexZgodyT = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexKtoJestWParku = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ileZajPoz = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexACK = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexOpuscPark = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexChceZgodeL = PTHREAD_MUTEX_INITIALIZER;


int ktoJestWParku[Mysliwi+1] = {0};
int wiadomosc[10] = {0};
int rozmiarWiadomosci = sizeof(wiadomosc);

/* **************
 wiadomosc[0] to idProcesu
 wiadomosc[1] to zegarLamporta
 wiadomosc[2]= {1,2,3} to pytam o licencje,zające,transporty
 wiadomosc[3]= {1} to czy zgoda licencja, 2 to zgoda zajace, itd(to nie zgoda)
 wiadomosc[4] to ile zajęcy chce zabić
 wiadomosc[5] = aktualna liczba zajecy
 wiadomosc[9]
 *          ********/

int ileMamZgodLicencja = 0;
int ileMamZgodZajace = 0;
int ileMamZgodTranport = 0;
int procesChceLicencje = 0;

void init();
void zwiekszZegarPrzyWyslaniu();

void * funkcjaWatku() {
    int wiadomosc2[10] = {0};
    while(!end) {
        wiadomosc2[3]=0;
        MPI_Recv(&wiadomosc2, rozmiarWiadomosci, MPI_INT, MPI_ANY_SOURCE, TAGzwykly, MPI_COMM_WORLD, &status);
        pthread_mutex_lock(&mutexChceZgodeL);
        pthread_mutex_lock(&mutexZegar);
       // printf("zegar[%d], proces[%d] : Dostalem wiadomosc od procesu %d zmieniam zegar z %d na %d\n",max(zegar, wiadomosc2[1]) + 1, rank, wiadomosc2[0], zegar, max(zegar, wiadomosc2[1]) + 1);
        zegar = max(zegar, wiadomosc2[1]) + 1;
        pthread_mutex_unlock(&mutexZegar);
        if(wiadomosc2[5]>0) {
            pthread_mutex_lock(&ileZajPoz);
            ileZajecyPozostalo = wiadomosc2[5];
            pthread_mutex_unlock(&ileZajPoz);
        }
        if(wiadomosc2[9]==1)
        {
            if(opuszczamPark == 0) {
                printf("%d otrzymal wiadomosc, ze trzeba opuscic park, wiec ubiega sie o transport \n", rank);
                pthread_mutex_lock(&mutexOpuscPark);
                opuszczamPark = 1;
                pthread_mutex_unlock(&mutexOpuscPark);
            }
            
        }
        else 
		{
			
			if(wiadomosc2[3]==1) {
				if(ileMamZgodLicencja != -1)
				{
                    if(wiadomosc2[1] < zegarRequest) {} //jezeli dostalem stara wiadomosc to ignoruj
                    else {
                    pthread_mutex_lock(&mutexZgodyL);
					ileMamZgodLicencja += 1;
				//	printf("Zegar[%d] Proces[%d] dostal +1 zgode na licencje od %d, ma ich juz: %d\n",zegar,rank,wiadomosc2[0],ileMamZgodLicencja);
					pthread_mutex_unlock(&mutexZgodyL);
                    }
				}
			}
			else if(wiadomosc2[3]==2) {
               /* if(wiadomosc2[5] != -1) {
                    pthread_mutex_lock(&ileZajPoz);
                    ileZajecyPozostalo = wiadomosc2[5];
                    pthread_mutex_unlock(&ileZajPoz);
                    
                }*/
				//sleep(1);
                if(wiadomosc2[1] < zegarRequest) {} //jezeli dostalem stara wiadomosc to ignoruj
                else {
				pthread_mutex_lock(&mutexZgodyZ);
				ileMamZgodZajace += 1;
			//	printf("Zegar[%d] @@@@@@@@@@@@@@@@@@Proces[%d] dostal +1 zgode o czasie %d na zabicie zajecy, ma ich juz: %d\n",zegar,rank,wiadomosc2[1],ileMamZgodZajace);
				pthread_mutex_unlock(&mutexZgodyZ);
                }
			}
			else if(wiadomosc2[3]==3) {
                if(wiadomosc2[1] < zegarRequest) {} //jezeli dostalem stara wiadomosc to ignoruj
                else {
				pthread_mutex_lock(&mutexZgodyT);
				ileMamZgodTranport += 1;
				//printf("Proces[%d] dostal +1 zgode na transport, ma ich juz: %d\n",rank,ileMamZgodTranport);
				pthread_mutex_unlock(&mutexZgodyT);
                }
			}
			else 
			{
				int doKogo = -1;
				if(wiadomosc2[2]==1) { //byla prosba o licencje
                    //while(!zapisalemZegarReqL) {}
					//if(zapisalemZegarReqL) {
                    //printf("OTRZYMALEM od %d, zegarze %d wiadomosc, ja %d, moj zegar %d, ktoJestWparku[%d]=%d\n",
                     //    wiadomosc2[0],wiadomosc2[1],rank,zegarRequest,rank,ktoJestWParku[rank]);
                    if(ktoJestWParku[rank]<=0)
                    {
                 //       printf("OTRZYMALEM od %d zegarze %d, ja %d mam zegarReq=%d, zegar=%d\n",wiadomosc2[0],wiadomosc2[1],rank,zegarRequest,zegar);
                            
                        if( (!zapisalemZegarReqL) || (!procesChceLicencje) ||
                        (liczbaZajecyDoZabicia == 0 && wiadomosc2[4]>0) ||
                        (
                         ( (liczbaZajecyDoZabicia>0 && wiadomosc2[4]>0) || (liczbaZajecyDoZabicia==0 && wiadomosc2[4]==0))
                             && ((zegarRequest > wiadomosc2[1]) || (zegarRequest == wiadomosc2[1] && rank > wiadomosc2[0])))
                        ){
                           //(zegarRequest > wiadomosc2[1])         //jezeli ja nie chce licencji lub proces pytajacy mnie ma mniejszy zegar to wysylam zgode
                          // || (zegarRequest == wiadomosc2[1] && rank > wiadomosc2[0])) {
                           // printf("%d UDZIELA WIEC ZGODY!\n",rank);
							zwiekszZegarPrzyWyslaniu();
							doKogo = wiadomosc2[0];
							wiadomosc2[0] = rank;
							wiadomosc2[1] = zegar;
							wiadomosc2[2] = 0;
							wiadomosc2[3] = 1;
							
                      //      printf("WYSYLAM (ja %d) zegarReq: %d, zegar: %d zgode do %d\n ",rank,zegarRequest,zegar,doKogo);
							MPI_Send(wiadomosc2,rozmiarWiadomosci, MPI_INT, doKogo, TAGzwykly, MPI_COMM_WORLD);
						//}
                        }
                            
					}
                    
				/*	else {
						
						zwiekszZegarPrzyWyslaniu();
						doKogo = wiadomosc2[0];
						wiadomosc2[0] = rank;
						wiadomosc2[1] = zegar;
						wiadomosc2[2] = 0;
						wiadomosc2[3] = 1;
						
						MPI_Send(wiadomosc2,rozmiarWiadomosci, MPI_INT, doKogo, TAGzwykly, MPI_COMM_WORLD);
						
					} */
					
				}
				else if(wiadomosc2[2]==2) { //byla prosba o zajaca					
					//if(ktoJestWParku[rank]<=0)
       //             while(!zapisalemZegarReq) {}
					//if(zapisalemZegarReq) {
                    //printf("OTRZYMALEM od %d, zegarze %d wiadomosc, ja %d, moj zegar %d\n",
                      //     wiadomosc2[0],wiadomosc2[1],rank,zegarRequest);
						//if((zegarRequest > wiadomosc2[1])         //jezeli proces pytajacy mnie ma mniejszy zegar to wysylam zgode
						//   || (zegarRequest == wiadomosc2[1] && rank > wiadomosc2[0])) {   //jezeli proces ma ten sam zegar to jezeli ma mniejszy indeks to wysylam zgode
						   if( (!zapisalemZegarReq) ||
                           (liczbaZajecyDoZabicia == 0 && wiadomosc2[4]>0) ||
                           (
                            ( (liczbaZajecyDoZabicia>0 && wiadomosc2[4]>0) || (liczbaZajecyDoZabicia==0 && wiadomosc2[4]==0))
                                && ((zegarRequest > wiadomosc2[1]) || (zegarRequest == wiadomosc2[1] && rank > wiadomosc2[0])))
                           ){
                            
                            
						    zwiekszZegarPrzyWyslaniu();
							doKogo = wiadomosc2[0];
							wiadomosc2[0] = rank;
							wiadomosc2[1] = zegar;
							wiadomosc2[2] = 0;
							wiadomosc2[3] = 2;

							MPI_Send(wiadomosc2,rozmiarWiadomosci, MPI_INT, doKogo, TAGzwykly, MPI_COMM_WORLD);
							
						//}
					}
					/*else {
						
						 zwiekszZegarPrzyWyslaniu();
						 doKogo = wiadomosc2[0];
						 wiadomosc2[0] = rank;
						 wiadomosc2[1] = zegar;
						 wiadomosc2[2] = 0;
						 wiadomosc2[3] = 2;
						 
						 MPI_Send(wiadomosc2,rozmiarWiadomosci, MPI_INT, doKogo, TAGzwykly, MPI_COMM_WORLD);
						
					}*/
					
				}
				else if(wiadomosc2[2]==3) { //byla prosba o transport
	//			   while(!zapisalemZegarReqT) {}
                    //if(zapisalemZegarReqT) {
                    //if(ktoJestWParku[rank]<=0)
					//if((zegarRequest > wiadomosc2[1])         //jezeli proces pytajacy mnie ma mniejszy zegar to wysylam zgode
					 // || (zegarRequest == wiadomosc2[1] && rank > wiadomosc2[0])) {   //jezeli proces ma ten sam zegar to jezeli ma mniejszy indeks to wysylam zgode
                       if( (!zapisalemZegarReqT) ||
                       (liczbaZajecyDoZabicia == 0 && wiadomosc2[4]>0) ||
                       (
                        ( (liczbaZajecyDoZabicia>0 && wiadomosc2[4]>0) || (liczbaZajecyDoZabicia==0 && wiadomosc2[4]==0) )
                            && ((zegarRequest > wiadomosc2[1]) || (zegarRequest == wiadomosc2[1] && rank > wiadomosc2[0]))
                       )
                       ){
                        
					   zwiekszZegarPrzyWyslaniu();
					   doKogo = wiadomosc2[0];
					   wiadomosc2[0] = rank;
					   wiadomosc2[1] = zegar;
					   wiadomosc2[2] = 0;
					   wiadomosc2[3] = 3;
					    
					   MPI_Send(wiadomosc2,rozmiarWiadomosci, MPI_INT, doKogo, TAGzwykly, MPI_COMM_WORLD);
					}
				  // }
				  /* else
				   {
					   zwiekszZegarPrzyWyslaniu();
					   
					   doKogo = wiadomosc2[0];
					   wiadomosc2[0] = rank;
					   wiadomosc2[1] = zegar;
					   wiadomosc2[2] = 0;
					   wiadomosc2[3] = 3;
					   
					   MPI_Send(wiadomosc2,rozmiarWiadomosci, MPI_INT, doKogo, TAGzwykly, MPI_COMM_WORLD);
					}*/
			   }
			}
		}
        //pthread_mutex_unlock(&mutexZegar);
        pthread_mutex_unlock(&mutexChceZgodeL);
    }
    return NULL;
}

void wejscieDoParkuCzyliKolejkaPoLicencje()
{    
	zwiekszZegarPrzyWyslaniu();
	
    wiadomosc[0] = rank;
    wiadomosc[1] = zegar;
    wiadomosc[2] = 1;
    wiadomosc[3] = 0;
    wiadomosc[4] = liczbaZajecyDoZabicia;
    
    zegarRequest = zegar;
    zapisalemZegarReqL = 1;
    
    
    pthread_mutex_lock(&mutex);
    for(int i=0; i<size; i++) {
        if(i==rank) continue;
        
        MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, i, TAGzwykly, MPI_COMM_WORLD);
        
     //   printf("<zegar[%d] Kolejka Licencje> Wysylam: (ja: %d) do %d, zegar=%d\n", zegar, wiadomosc[0],i,wiadomosc[1]);
    }
    pthread_mutex_unlock(&mutex);
}

void zabijanieZajecyCzyliKolejkaPoZajaca()
{    
	zwiekszZegarPrzyWyslaniu();
	
    wiadomosc[0] = rank;
    wiadomosc[1] = zegar;
    wiadomosc[2] = 2;
    wiadomosc[3] = 0;
    wiadomosc[4] = liczbaZajecyDoZabicia;
    wiadomosc[5] = -1;
    
    zegarRequest = zegar;
    zapisalemZegarReq = 1;
    
    pthread_mutex_lock(&mutex);
    for(int i=0; i<size; i++) {
        if(i==rank) continue;
        if(ktoJestWParku[i] > 0) {
            
            MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, i, TAGzwykly, MPI_COMM_WORLD);
            
         //   printf("zegar[%d] <Kolejka Zajace> Wysylam: (ja: %d) do %d, zegar=%d\n", zegar,wiadomosc[0],i,wiadomosc[1]);
        }
    }pthread_mutex_unlock(&mutex);
    
}

void opuszczanieParkuCzyliKolejkaPoTransport()
{    
	zwiekszZegarPrzyWyslaniu();
	
    wiadomosc[0] = rank;
    wiadomosc[1] = zegar;
    wiadomosc[2] = 3;
    wiadomosc[3] = 0;
    wiadomosc[4] = liczbaZajecyDoZabicia;
    wiadomosc[5] = -1;
    
    zegarRequest = zegar;
    zapisalemZegarReqT = 1;
    
    for(int i=0; i<size; i++) {
        if(i==rank) continue;
        if(ktoJestWParku[i] > 0) {
            pthread_mutex_lock(&mutex);
            MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, i, TAGzwykly, MPI_COMM_WORLD);
            pthread_mutex_unlock(&mutex);
          //  printf("<Kolejka Transport> Wysylam: (ja: %d) do %d, zegar=%d\n", wiadomosc[0],i,wiadomosc[1]);
        }
    }
    
}


void *odbierzInfoKtoJestWParku() {
    int ktoJestWParku2[Mysliwi+1] = {0};
    while(1) {
        MPI_Recv(&ktoJestWParku2, sizeof(ktoJestWParku2), MPI_INT, MPI_ANY_SOURCE, TAGzajac, MPI_COMM_WORLD, &status);
        
        //printf("@@@@@ DOstalem (%d) wiadomosc z zegarem: %d\n",rank,ktoJestWParku2[status.MPI_SOURCE]);
        pthread_mutex_lock(&mutexZegar);
        zegar = max(zegar, ktoJestWParku2[status.MPI_SOURCE]) + 1;
        pthread_mutex_unlock(&mutexZegar);
        
        pthread_mutex_lock(&mutexKtoJestWParku);
        int i=status.MPI_SOURCE;
        //for(int i=0; i<Mysliwi; i++) {
            
            if(ktoJestWParku[i]==-1 || ktoJestWParku2[i]==-1)
            {
                //pthread_mutex_lock(&mutexKtoJestWParku);
                ktoJestWParku[i] = -1;
                //pthread_mutex_unlock(&mutexKtoJestWParku);
                
            }
            else if(ktoJestWParku2[i]==-5) ktoJestWParku[i]=-5;
            else {
               // pthread_mutex_lock(&mutexKtoJestWParku);
                ktoJestWParku[i] = max(ktoJestWParku[i],ktoJestWParku2[i]);
                //pthread_mutex_unlock(&mutexKtoJestWParku);
            }
                
        //}
        pthread_mutex_unlock(&mutexKtoJestWParku);
        
        if(ktoJestWParku2[Mysliwi]!=-2)
        {
           // printf("Zegar[%d] ODEBRALEM WIADOMOSC %d OD PROCESU %d, wyslijack=%d\n",zegar, rank,status.MPI_SOURCE,wyslijack);
         if(ktoJestWParku2[Mysliwi]==-1) {
               // printf("Zegar[%d] Otrzymalem zadanie %d wyslac ack do %d\n",zegar, rank,status.MPI_SOURCE);
                pthread_mutex_lock(&mutexKtoJestWParku);
                ktoJestWParku[Mysliwi] = 1;
                pthread_mutex_unlock(&mutexKtoJestWParku);
                
                zwiekszZegarPrzyWyslaniu();
                
                pthread_mutex_lock(&mutex);
                MPI_Send(ktoJestWParku,sizeof(ktoJestWParku), MPI_INT, status.MPI_SOURCE, TAGzajac, MPI_COMM_WORLD);
                pthread_mutex_unlock(&mutex);
                
            }
            
            if(ktoJestWParku2[Mysliwi])
            {
                //printf("Zegar[%d] %d Otrzymalem zgode od %d\n",zegar, rank,status.MPI_SOURCE);
                pthread_mutex_lock(&mutexACK);
                zgodaOdWszystkich++;
                pthread_mutex_unlock(&mutexACK);
                pthread_mutex_lock(&mutexKtoJestWParku);
                ktoJestWParku2[Mysliwi] = 0;
                pthread_mutex_unlock(&mutexKtoJestWParku);
             //   printf("PROCES %d DOSTAL ZGODE W TEJ PETLI WHILE\n", rank);
            }
            if(zgodaOdWszystkich == Mysliwi-1) {
                //printf("%d DOSTAL WSZYSTKIE ZGODYYY\n",rank);
                ktoJestWParku2[Mysliwi] = 0;
                
            }
        }


        
    }
    return NULL;
}

void wyslijInfoKtoJestWParku() {
    wyslijack = 1;
    pthread_mutex_lock(&mutexACK);
    zgodaOdWszystkich = 0;
    pthread_mutex_unlock(&mutexACK);
    
	
	zwiekszZegarPrzyWyslaniu();
	
    pthread_mutex_lock(&mutexKtoJestWParku);
    ktoJestWParku[Mysliwi] = -1;
    pthread_mutex_unlock(&mutexKtoJestWParku);
    //zegarRequest = zegar;
    for(int i=0; i<size; i++) {
        if(i==rank) continue;
        pthread_mutex_lock(&mutex);
        MPI_Send(ktoJestWParku,sizeof(ktoJestWParku), MPI_INT, i, TAGzajac, MPI_COMM_WORLD);
        pthread_mutex_unlock(&mutex);
    }
}

void wyslijInfoKtoJestWParku2() {
    //wyslijack = 1;
    //pthread_mutex_lock(&mutexACK);
    //zgodaOdWszystkich = 0;
    //pthread_mutex_unlock(&mutexACK);
    
    
    zwiekszZegarPrzyWyslaniu();
    
    pthread_mutex_lock(&mutexKtoJestWParku);
    ktoJestWParku[Mysliwi] = -2;
    pthread_mutex_unlock(&mutexKtoJestWParku);
    //zegarRequest = zegar;
    for(int i=0; i<size; i++) {
        if(i==rank) continue;
        pthread_mutex_lock(&mutex);
        MPI_Send(ktoJestWParku,sizeof(ktoJestWParku), MPI_INT, i, TAGzajac, MPI_COMM_WORLD);
        pthread_mutex_unlock(&mutex);
    }
}

int sumujTabliceKtoNieChcialDoParku() {
    int s = 0;
    for(int i=0; i<Mysliwi; i++) {
        if(ktoJestWParku[i] == -1) s += 1;
    }
    return s;
}

int sumujTabliceKtoJestWParku() {
    int s = 0;
    for(int i=0; i<Mysliwi; i++) {
        if(ktoJestWParku[i] > 0) s += 1;
    }
    return s;
}

void wyslijZgodeWszystkim(int oCoZgoda) {
        
	zwiekszZegarPrzyWyslaniu();
	
    wiadomosc[0] = rank;
    wiadomosc[1] = zegar;
    wiadomosc[2] = 0;
    wiadomosc[3] = oCoZgoda;
    wiadomosc[4] = liczbaZajecyDoZabicia;
    wiadomosc[5] = 0;
    
    for(int i=0; i<size; i++) {
        if(i==rank) continue;
        if(ktoJestWParku[i] > 0) {
            pthread_mutex_lock(&mutex);
            MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, i, TAGzwykly, MPI_COMM_WORLD);
            pthread_mutex_unlock(&mutex);
     //       printf("<Wyslij wszystkim Kolejka Zajace> Wysylam: (ja: %d) do %d, zegar=%d\n", wiadomosc[0],i,wiadomosc[1]);
        }
    }
}

void wyslijInfoOPozostZaj() {
        
    zwiekszZegarPrzyWyslaniu();
    
    wiadomosc[0] = rank;
    wiadomosc[1] = zegar;
    wiadomosc[2] = 0;
    wiadomosc[3] = 0;
    wiadomosc[4] = liczbaZajecyDoZabicia;
    wiadomosc[5] = ileZajecyPozostalo;
    
    for(int i=0; i<size; i++) {
        if(i==rank) continue;
        //if(ktoJestWParku[i] > 0) {
            pthread_mutex_lock(&mutex);
            MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, i, TAGzwykly, MPI_COMM_WORLD);
            pthread_mutex_unlock(&mutex);
     //       printf("<Wyslij wszystkim Kolejka Zajace> Wysylam: (ja: %d) do %d, zegar=%d\n", wiadomosc[0],i,wiadomosc[1]);
       // }
    }
}

void wyslijZgodeWszystkimLicencja() {
        
	zwiekszZegarPrzyWyslaniu();
	
    wiadomosc[0] = rank;
    wiadomosc[1] = zegar;
    wiadomosc[2] = 0;
    wiadomosc[3] = 1;
    wiadomosc[4] = liczbaZajecyDoZabicia;
    wiadomosc[5] = ileZajecyPozostalo;
    
    for(int i=0; i<size; i++) {
        if(i==rank) continue;
        
            pthread_mutex_lock(&mutex);
            MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, i, TAGzwykly, MPI_COMM_WORLD);
            pthread_mutex_unlock(&mutex);
     //       printf("<Wyslij wszystkim Kolejka Zajace> Wysylam: (ja: %d) do %d, zegar=%d\n", wiadomosc[0],i,wiadomosc[1]);
        
    }
}

void wyslijZgodeWszystkimZajaceWszyscy() {
        
    zwiekszZegarPrzyWyslaniu();
    
    wiadomosc[0] = rank;
    wiadomosc[1] = zegar;
    wiadomosc[2] = 0;
    wiadomosc[3] = 2;
    wiadomosc[4] = liczbaZajecyDoZabicia;
    wiadomosc[5] = 0;
    
    for(int i=0; i<size; i++) {
        if(i==rank) continue;
        //if(ktoJestWParku[i] > 0) {
            pthread_mutex_lock(&mutex);
            MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, i, TAGzwykly, MPI_COMM_WORLD);
            pthread_mutex_unlock(&mutex);
     //       printf("<Wyslij wszystkim Kolejka Zajace> Wysylam: (ja: %d) do %d, zegar=%d\n", wiadomosc[0],i,wiadomosc[1]);
        //}
    }
}

void wszyscyOpuszczajaPark() {
     
	 zwiekszZegarPrzyWyslaniu();
	 
     wiadomosc[0] = rank;
     wiadomosc[1] = zegar;
     wiadomosc[2] = 0;
     wiadomosc[3] = 0;
     wiadomosc[4] = liczbaZajecyDoZabicia;
     wiadomosc[9] = 1;
     
     for(int i=0; i<size; i++) {
         if(i==rank) continue;
         if(ktoJestWParku[i] > 0) {
             pthread_mutex_lock(&mutex);
             MPI_Send(wiadomosc,rozmiarWiadomosci, MPI_INT, i, TAGzwykly, MPI_COMM_WORLD);
             pthread_mutex_unlock(&mutex);
      //       printf("<Wyslij wszystkim Kolejka Zajace> Wysylam: (ja: %d) do %d, zegar=%d\n", wiadomosc[0],i,wiadomosc[1]);
         }
     }
    
}


void start() {
poczatek:
    if(rank==ROOT) printf("Rozpoczynam proces wejscia %d\n",rank);
    wiadomosc[9]=0;
    
//
    
    pthread_mutex_lock(&mutexZgodyL);
     ileMamZgodLicencja = 0;
    pthread_mutex_unlock(&mutexZgodyL);
    pthread_mutex_lock(&mutexZgodyZ);
    ileMamZgodZajace = 0;
    pthread_mutex_unlock(&mutexZgodyZ);
    pthread_mutex_lock(&mutexZgodyT);
    ileMamZgodTranport = 0;
    pthread_mutex_unlock(&mutexZgodyT);
//
    zapisalemZegarReqL = zapisalemZegarReq = zapisalemZegarReqT = 0;
    procesChceLicencje = 0;
    pthread_mutex_lock(&mutexOpuscPark);
    opuszczamPark = 0;
    pthread_mutex_unlock(&mutexOpuscPark);
    
   // init();
    if(Licencje > Mysliwi) Licencje = Mysliwi;
    int flaga = 0;


    procesChceLicencje = 1;//(rand() % 100 + 1) > 1; //(100-1)% szans ze chce licencje, jezeli chce to:
    
    if (procesChceLicencje) {
        pthread_mutex_lock(&mutexChceZgodeL);
        printf("Zegar[%d] Proces: %d chce dostac licencje do parku!\n",zegar, rank);
        /*wyslijInfoKtoJestWParku();
        while(zgodaOdWszystkich < Mysliwi-1 -1) { } //czekaj na ack od wszystkich
                   pthread_mutex_lock(&mutexZgodyL);
                   ileMamZgodLicencja = 0;
                   pthread_mutex_unlock(&mutexZgodyL);
                   pthread_mutex_lock(&mutexZgodyZ);
                   ileMamZgodZajace = 0;
                   pthread_mutex_unlock(&mutexZgodyZ);
                   pthread_mutex_lock(&mutexZgodyT);
                   ileMamZgodTranport = 0;
                   pthread_mutex_unlock(&mutexZgodyT);
        printf("WYSZEDLEM Z WHILE %d\n",rank);*/
        
        wejscieDoParkuCzyliKolejkaPoLicencje();
        pthread_mutex_unlock(&mutexChceZgodeL);
        while(ileMamZgodLicencja < (Mysliwi - Licencje + 1) - 1) {
            //oczekuj na odpowiednia ilosc zgod
            if(opuszczamPark) break;
        }
        
        if(!opuszczamPark) {
            
            pthread_mutex_lock(&mutexZgodyL);
            ileMamZgodLicencja = -1;
            pthread_mutex_unlock(&mutexZgodyL);
            
            printf("Zegar[%d] -----------------------Proces:%d jest w parku---------------------\n",zegar,rank);
            pthread_mutex_lock(&mutexKtoJestWParku);
            ktoJestWParku[rank] = zegar;
            pthread_mutex_unlock(&mutexKtoJestWParku);
            //sleep(2); printf("\n-------------------------------------------------------\n");sleep(2);
            if(liczbaZajecyDoZabicia == 0) liczbaZajecyDoZabicia = rand() % (Zajace/15) + 1;
            
            wyslijInfoKtoJestWParku();
            
            while(zgodaOdWszystkich < Mysliwi-1 -1) { } //czekaj na ack od wszystkich
            pthread_mutex_lock(&mutexZgodyL);
            ileMamZgodLicencja = 0;
            pthread_mutex_unlock(&mutexZgodyL);
            pthread_mutex_lock(&mutexZgodyZ);
            ileMamZgodZajace = 0;
            pthread_mutex_unlock(&mutexZgodyZ);
            pthread_mutex_lock(&mutexZgodyT);
            ileMamZgodTranport = 0;
            pthread_mutex_unlock(&mutexZgodyT);
            printf("Zegar[%d] Ja(%d) liczba zgod L=%d, Z=%d, T=%d \n",zegar,rank,ileMamZgodLicencja,ileMamZgodZajace,ileMamZgodTranport);
            //printf("######### Zegar[%d], %d Otrzymalem zgode od wszystkich\n",zegar, rank);
            
            while((sumujTabliceKtoJestWParku() < Licencje)) {
                if(sumujTabliceKtoJestWParku()+sumujTabliceKtoNieChcialDoParku() == Mysliwi) break; //przypadek gdy 1 jest w parku, 3 nie chcialo, a licencji > 1
                sleep(1);
            }
            
            zapisalemZegarReqL = 0;
            ileOsobJestWParku = sumujTabliceKtoJestWParku();
            //printf("Zegar[%d] Proces: %d TWIERDZI ze Wszyscy sa w PARKU (suma=%d)\n",zegar, rank, ileOsobJestWParku);
         //   for(int i=0;i<Mysliwi;i++) {
         //       if(ktoJestWParku[i]>0) printf("zegar[%d]----------------Proces %d mowi ze %d jest w parku\n",
          //                                    zegar,rank,i); }
            //printf("Proces: %d TWIERDZI ze Wszyscy sa w PARKU (suma=%d)\n",rank, ileOsobJestWParku);
           // printf("---%d) Wchodze do nieskonczonego while\n",rank);
           // while(1){}
            //zegarRequest = zegar;
            pthread_mutex_lock(&mutexZgodyZ);
            ileMamZgodZajace = 0;
            pthread_mutex_unlock(&mutexZgodyZ);
            pthread_mutex_lock(&mutexChceZgodeL);
            zabijanieZajecyCzyliKolejkaPoZajaca();
            pthread_mutex_unlock(&mutexChceZgodeL);
            
        }
        while(ileMamZgodZajace < sumujTabliceKtoJestWParku()-1) {
            //oczekuj na odpowiednia ilosc zgod
            sleep(1);
            if(opuszczamPark) break;
        }
       // printf("     Zegar[%d] %d) Jestem poza whilem do zgodZajecy wiec ide polowac\n",zegar,rank);
        zapisalemZegarReq = 0;
        //printf("%d mam zgode na zabicie zajecy!\n",rank);
        
        if(ileZajecyPozostalo > 0) {
            if(!opuszczamPark) {
            if(ileZajecyPozostalo >= liczbaZajecyDoZabicia) {
                printf("Zegar[%d] Przed zabiciem: %d, proces %d) Zabil %d zajecy. Pozostalo ich              %d\n",zegar, ileZajecyPozostalo, rank, liczbaZajecyDoZabicia, ileZajecyPozostalo-liczbaZajecyDoZabicia);
                pthread_mutex_lock(&ileZajPoz);
                ileZajecyPozostalo -= liczbaZajecyDoZabicia;
                pthread_mutex_unlock(&ileZajPoz);
                
                liczbaZajecyDoZabicia = 0;
                
                printf("%d) Zabilem tyle ile chcialem, wzywam transport\n",rank);
                
                wyslijInfoOPozostZaj();
                wyslijZgodeWszystkimZajaceWszyscy();
                
                //while(1){printf("TRANSPORT - %d mam odpowiedna ilosc zgod: %d\n",rank, ileMamZgodTranport);sleep(5);}
            }
            else
            {
                printf("%d) Zabil %d zajecy. Pozostalo ich %d, chcial zabic %d\n",rank, ileZajecyPozostalo, 0, liczbaZajecyDoZabicia);
                liczbaZajecyDoZabicia -= ileZajecyPozostalo;
                pthread_mutex_lock(&ileZajPoz);
                ileZajecyPozostalo = 0;
                pthread_mutex_unlock(&ileZajPoz);
                
                //printf("Wszystkie zajace w parku zostaly zabite. Pora wezwac technikow i opuscic park\n");
                wyslijInfoOPozostZaj();
            }
            //wyslijZgodeWszystkim(2);
        //wyslijInfoOPozostZaj();
        //wyslijZgodeWszystkimZajaceWszyscy();
            //wyslijZgodeWszystkim(2);
            
            pthread_mutex_lock(&mutexChceZgodeL);
            opuszczanieParkuCzyliKolejkaPoTransport();
            pthread_mutex_unlock(&mutexChceZgodeL);
            
            
            while(ileMamZgodTranport < ileOsobJestWParku - Transport) { } //czekaj na odpowiednia ilosc zgod
            flaga = rank;
			zapisalemZegarReqT=0;
            printf("Zegar[%d] %d Opuszcza park, bo zabil odpowiednia ilosc zajecy i ma transport\n", zegar, rank);
            
            pthread_mutex_lock(&mutexOpuscPark);
            opuszczamPark = 1;
            pthread_mutex_unlock(&mutexOpuscPark);
            
            ktoJestWParku[rank] = -5; //0
            wyslijInfoKtoJestWParku2();
            while(zgodaOdWszystkich < Mysliwi-1 -1) { } //czekaj na ack od wszystkich
           // printf("xxxddddd Zegar[%d], %d Otrzymalem zgode od wszystkich\n",zegar, rank);
            wyslijZgodeWszystkim(3);
            if(ileZajecyPozostalo != 0) wyslijZgodeWszystkimLicencja();
//
            
            pthread_mutex_lock(&mutexZgodyL);
             ileMamZgodLicencja =  0;
            pthread_mutex_unlock(&mutexZgodyL);
            pthread_mutex_lock(&mutexZgodyZ);
            ileMamZgodZajace = 0;
            pthread_mutex_unlock(&mutexZgodyZ);
            pthread_mutex_lock(&mutexZgodyT);
            ileMamZgodTranport = 0;
            pthread_mutex_unlock(&mutexZgodyT);
//
            
            if(ileZajecyPozostalo > 0) start();
            
        }
        }
        if(ileZajecyPozostalo == 0)
        {
            printf("Zegar[%d], %d) Wszystkie zajace zostaly zabite. Pora wezwac technikow i opuscic park\n",zegar,rank);
            pthread_mutex_lock(&mutexChceZgodeL);
            wszyscyOpuszczajaPark();
            pthread_mutex_unlock(&mutexChceZgodeL);
            
            
            ktoJestWParku[rank] = -5; //0
            
            if(flaga!=rank) 
			{
                ktoJestWParku[rank] = -5;
				while(ileMamZgodTranport < ileOsobJestWParku - Transport) { } //czekaj na odpowiednia ilosc zgod
				printf("Zegar[%d], %d otrzymal odpowiednia ilosc zgod na transport!\n",zegar,rank);
				wyslijZgodeWszystkim(3);
//
                pthread_mutex_lock(&mutexZgodyL);
                 ileMamZgodLicencja =  0;
                pthread_mutex_unlock(&mutexZgodyL);
                pthread_mutex_lock(&mutexZgodyZ);
                ileMamZgodZajace = 0;
                pthread_mutex_unlock(&mutexZgodyZ);
                pthread_mutex_lock(&mutexZgodyT);
                ileMamZgodTranport = 0;
                pthread_mutex_unlock(&mutexZgodyT);
//
                zapisalemZegarReqT=0;
			}
            if(liczbaZajecyDoZabicia == 0) ktoJestWParku[rank] = -5;
        }
                          
//wyslijInfoKtoJestWParku2();
//wyslijInfoKtoJestWParku();
//while(sumujTabliceKtoJestWParku() != 0) {}
        
        printf("Zegar[%d], %d mowi, ze technicy weszli odbudowac robozajace\n",zegar, rank);
        sleep(4);
        printf("Zegar[%d], Robozajace odbudowane\n",zegar);
        ileZajecyPozostalo = Zajace;
    }
    else {
        pthread_mutex_lock(&mutexKtoJestWParku);
        ktoJestWParku[rank] = -1;
        pthread_mutex_unlock(&mutexKtoJestWParku);
        wyslijInfoKtoJestWParku2();
        pthread_mutex_lock(&mutexOpuscPark);
        opuszczamPark = 1;
        pthread_mutex_unlock(&mutexOpuscPark);
        
        printf("Ja: %d Nie chcialem licencji wiec skonczylem program\n", rank);
    }
    goto poczatek;
}
int main(int argc, char **argv) {
       
    int provided;
    MPI_Init_thread(&argc, &argv, 3, &provided);
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    //if(rank==0) printf("ROZPOCZYNAM PROGRAM\n");
    
    srand(time(0)+rank); //kazdy ma inny seed
    
    //stworz watki
    pthread_t watek1, watek2, watek3;
    int errno = pthread_create(&watek1, NULL, funkcjaWatku, (void*)&procesChceLicencje); //zwraca 0 gdy sukces
    int errno2 = pthread_create(&watek2, NULL, odbierzInfoKtoJestWParku, NULL); //zwraca 0 gdy sukces

    
    if(rank==ROOT) printf("\n\nROZPOCZYNAM WEJSCIE DO PARKU\n\n");
    //init();
    while(1) {init(); start(); }
    //printf("KONIEC\n");
    //sleep(5);
    //if(rank==ROOT) printf("\n\nROZPOCZYNAM WEJSCIE DO PARKU\n\n");
    //start();
        
    if(errno || errno2) {
        printf("Nie udalo sie utworzyc watku!!\n");
    }
    
    //polacz z powrotem watki
    errno = pthread_join(watek1, NULL); //zwraca 0 gdy sukces
    errno2 = pthread_join(watek2, NULL);
    if(errno || errno2) {
        printf("Nie udalo sie polaczyc watkow!!\n");
    }
        
    
    MPI_Finalize();
}

void init() {
    /*if(size < 1290) //(MAX_INT)^(1/3)
        zegar = rand() % (size*size) + 1; */
    zegar = 1;
    usleep(100 * (rand() % 100 + 1));
    printf("%d ma godzine: %d \n", rank, zegar);
}

void zwiekszZegarPrzyWyslaniu() {
    pthread_mutex_lock(&mutexZegar);
    zegar++;
    pthread_mutex_unlock(&mutexZegar);
}

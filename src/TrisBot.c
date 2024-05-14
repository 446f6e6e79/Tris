/************************************
*VR485945, VR485743
*Davide Donà, Andrea Blushi
*Data di realizzazione: 07-05-24
*************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <string.h>
#include <time.h>
#include "errExit.h"
#include "semaphore.h"
#include "utils.h"

// Definizione variabili globali
int shmid;
sharedData *sD;
int playerIndex;
int semID;

void getBotPlay();
void comunicaDisconnessione();
void terminazioneSicura();
void sigUser1Handler(int sig);

void sigUser1Handler(int sig){
    switch(sD->stato){
        case 0://La partita termina in pareggio
            //Attendo 3 secondi, poi sblocco il server che inizierà un nuovo game
            //Faccio far l'attesa solo al player intando avvio la prima parte del semaforo
            s_signal(semID, SEM_SERVER);

            break;
        case 1://Nella partita avviene una vittoria
        case 2:
            if(sD->stato == playerIndex){
                //Nel caso il bot vincesse vuole sempre giocare
                s_signal(semID, getOtherPlayerIndex(playerIndex));
                s_signal(semID, SEM_SERVER);//Avviso il server di inizializzare la tavola e che voglio rigiocare
            }   
                //Attendo la decisione del player vincitore
                s_wait(semID, playerIndex);

                /*
                    Player SCOLLEGATO:
                        - comunicoDisconnessione al server
                            - decremento activePlayer = 0
                            - segnalo l'abbandono al server
                        - termino correttamente il processo
                */
                
                if(sD->activePlayer == 1){
                    s_wait(semID, SEM_MUTEX);
                    comunicaDisconnessione();
                    s_signal(semID, SEM_MUTEX);
                    terminazioneSicura();
                }
                //Passo il turno al server che gestirà il match
                s_signal(semID, SEM_SERVER);
            
            break;
        
        case 3: //Processo server disconnesso
           
            terminazioneSicura();
            break;
        
        case 4: //L'altro giocatore si è disconnesso il bot di scollega di conseguenza
            s_wait(semID, SEM_MUTEX);
           
            
                comunicaDisconnessione();
                s_signal(semID, SEM_MUTEX);
                s_signal(semID, SEM_INIZIALIZZAZIONE);
                terminazioneSicura();
            
            break;
    }
}

int main() {
    //Inizializzo il random
    srand(time(NULL));

    if (signal(SIGUSR1, sigUser1Handler) == SIG_ERR) {
        errExit("Errore nel USR1 Handler");
    }
    // Inizializzazione dei semafori
    semID = semget(SEM_KEY, NUM_SEM, IPC_CREAT | 0666);
    if (semID == -1) {
        errExit("Errore nella get del semaforo\n");
    }

    // Recupero lo shareMemoryID usando la systemCall shmget
    shmid = shmget(MEMORY_KEY, sizeof(sharedData), 0666);
    if (shmid < 0) {
        errExit("Errore nella generazione della memoria condivisa\n");
    }

    sD = (sharedData *)shmat(shmid, NULL, 0);
    if (sD == (void *)-1) {
        errExit("Errore nell'attach della memoria condivisa\n");
    }
    //Il comportamento del BOT riprende gli atteggiamenti di un Player
    // Settaggio delle variabili condivise (SEZIONE CRITICA)
    
    s_wait(semID, SEM_MUTEX);
    playerIndex = 2;
    strcpy(sD->playerName[playerIndex - 1], "Computer");
    sD->pids[playerIndex] = getpid();
    semOp(semID, SEM_INIZIALIZZAZIONE, +3);
    s_signal(semID, SEM_MUTEX);

    // Attendo che entrambi i giocatori siano attivi
    s_wait(semID, SEM_INIZIALIZZAZIONE);

    //Incremento activePlayer solo una volta avviato il processo player
    sD->activePlayer++;

    // Inizio del gioco
    do {
        // Attende il proprio turno
        s_wait(semID, playerIndex);
        getBotPlay();
        // Avvisa il processo Server che una mossa è stata eseguita
        s_signal(semID, SEM_SERVER);
    } while (1);

}

void terminazioneSicura(){
    //Chiusura memoria condivisa con attach
    shmdt((void *) sD);
    exit(1);
}
/*
    Decremento il valore di activePlayer, invio il segnale al server.
    Tale funzione, in quanto accede alla memoria condivisa, dovrà essere racchiusa in un semaforo mutex
*/
void comunicaDisconnessione(){
    sD->activePlayer--;
    //Avviso il server che entrambi abbiamo abbandonato
    if(kill(sD->pids[PID_SERVER], SIGUSR1) == -1) {
        terminazioneSicura();
    }
}

//Emula un comportamento di un bot generando una posizione casuale in cui inserire il gettone
void getBotPlay() {
    int index;
    do {
        index = rand() % BOARD_SIZE;
    } while (sD->board[index] != ' ');

    sD->board[index] = sD->player[playerIndex - 1];
}

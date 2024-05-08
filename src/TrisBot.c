/************************************
*VR485945, VR485743
*Davide Donà, Andrea Blushi
*Data di realizzazione: 07-05-24
*************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
void terminazioneSicura();

int main() {
    //Inizializzo il random
    srand(time(NULL));

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
    s_wait(semID, 0);
    playerIndex = 2;
    strcpy(sD->playerName[playerIndex - 1], "Computer");
    sD->pids[playerIndex] = getpid();
    semOp(semID, SEM_SERVER, +3);
    s_signal(semID, 0);

    // Attendo che entrambi i giocatori siano attivi
    s_wait(semID, SEM_SERVER);
    sD->activePlayer++;

    // Inizio del gioco
    do {
        // Attende il proprio turno
        s_wait(semID, playerIndex);
        getBotPlay();
        // Avvisa il processo Server che una mossa è stata eseguita
        s_signal(semID, SEM_SERVER);
    } while (1);

    terminazioneSicura();
}

void terminazioneSicura(){
    //Chiusura memoria condivisa con attach
    shmdt((void *) sD);
    exit(1);
}

//Emula un comportamento di un bot generando una posizione casuale in cui inserire il gettone
void getBotPlay() {
    int index;
    do {
        index = rand() % BOARD_SIZE;
    } while (sD->board[index] != ' ');

    sD->board[index] = sD->player[playerIndex - 1];
}

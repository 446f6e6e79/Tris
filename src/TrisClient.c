#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "errExit.h"
#include <semaphore.h>

#define BOARD_SIZE 9

typedef struct{
    char player1;
    char player2;
    char * player1Name;
    char * player2Name;
    char board[BOARD_SIZE];
}sharedData;

int shmid;
sharedData *sD;

void terminazioneSicura();

int main(int argC, char * argV[]) {

    if(argC != 2){
        printf("Usage: %s <nomeUtente>", argV[0]);
        return 1;
    }
    if(argV[2][0] == '*'){
        //Gioca Contro il PC
    }
    
    //Recupero lo shareMemoryID usando la systemCall shmget
    shmid = shmget(1234, sizeof(sharedData), 0666);
    if (shmid < 0) {
        errExit("Errore nella generazione della memoria condivisa");
    }

    sD = (sharedData *)shmat(shmid, NULL, 0);
    if (sD == (void *)-1) {
        errExit("Errore nell'attach alla memoria condivisa");
    }

    printf("Player 1: %c\n", sD->player1);
    printf("Player 2: %c\n", sD->player2);

    int x,y;
 
    printf("Inserisci coordinate posizione (x y): ");
    scanf("%d %d", &x, &y);
    //Ottengo l'indice 
    int index = (3 * (y - 1)) + x - 1;
    printf("Hai inserito: %d %d\n", x, y);
    terminazioneSicura();
    return 0;
}

void terminazioneSicura(){
    //Chiusura e pulizia della memoria condivisa con annessi attach
    shmdt((void *) sD);
    shmctl(shmid, IPC_RMID, NULL);
}

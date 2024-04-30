#include<stdio.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h>
#include <stdlib.h>


void terminazioneSicura();

void firstSigIntHandler(int sig){
    printf("È stato premuto CTRL-C.\nUna seconda pressione comporta la terminazione!");
    //Cambio ora il comportamento al
}

void secondSigIntHandler(int sig){
    //terminazioneSicura();
    printf("Il gioco è stato terminato.\n");
    exit(0);
}
#define BOARD_SIZE 9

int main(int argC, char * argV[]){
    
    char player1, player2;
    
    char *board;
    
    //Controllo sui parametri passati
    if(argC != 4){
        printf("Usage: %s <timeout> <SimboloPlayer1> <SimboloPlayer2>\n", argV[0]);
        return 1;
    }
    //Generazione della memoria condivisa
    
    int shmid = shmget(69, sizeof(char)*BOARD_SIZE, 0666|IPC_CREAT);
    if(shmid < 0){
        printf("Errore nella generazione della memoria condivisa\n");
        exit(0);
    }
    board = (char *) shmat(shmid, NULL, 0);
    /*
    if((int) *board < 0){
        printf("Errore nell'attach alla memoria condivisa\n");
        exit(0);
    }
    */

    //Inizializzazione array board

    for(int i = 0; i < BOARD_SIZE; i++){
        board[i] = ' ';
    }
    for(int i = 0; i < BOARD_SIZE; i++){
        if(i == 3){
            printf("\n-------\n");
        }
        printf("%c|", board[i]);
    }
    printf("\n");
    //Chiusura e pulizia della memoria condivisa con annessi attach
    shmdt((void *) board);
    shmctl(shmid, IPC_RMID, NULL);
}

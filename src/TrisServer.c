#include<stdio.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errExit.h"

#define BOARD_SIZE 9

//Definizione prototipi
void printBoard();
void terminazioneSicura();
void firstSigIntHandler(int sig);
void secondSigIntHandler(int sig);

void firstSigIntHandler(int sig){
    printf("È stato premuto CTRL-C.\nUna seconda pressione comporta la terminazione!");
    //Cambio ora il comportamento al segnale sigInt
    signal(SIGINT, secondSigIntHandler);
}

void secondSigIntHandler(int sig){
    terminazioneSicura();
    printf("Il gioco è stato terminato.\n");
    exit(0);
}

char *board;
int shmid;

int main(int argC, char * argV[]){
    
    char player1, player2;
    int timeOut;
    
    //Controllo sui parametri passati
    if(argC != 4){
        printf("Usage: %s <timeout> <SimboloPlayer1> <SimboloPlayer2>\n", argV[0]);
        return 1;
    }
    //Converto il valore di timeout in un intero
    timeOut = atoi(argV[1]);
    if(timeOut < 0){
        errExit("TimeOut > 0");
    }
    //Verifico la lunghezza dei simboli inseriti

    if(strlen(argV[2]) != 1 || strlen(argV[3])!= 1){
        errExit("Simboli devono essere caratteri");
    }
    player1 = argV[2][0];
    player2 = argV[3][0];

    //Setto il nuvo comportamento dei segnali
    signal(SIGINT, firstSigIntHandler);

    
    //Generazione della memoria condivisa
    shmid = shmget(69, sizeof(char)*BOARD_SIZE, 0666|IPC_CREAT);
    if(shmid < 0){
        errExit("Errore nella generazione della memoria condivisa\n");
    }
    
    //Attacco l'array board alla zona di memoria condivisa
    board = (char *) shmat(shmid, NULL, 0);
    if(board == (char *)-1){
        errExit("Errore nell'attach alla memoria condivisa\n");
    }
    
    //Inizializzazione array board
    for(int i = 0; i < BOARD_SIZE; i++){
        board[i] = ' ';
    }
    printBoard();
    do{
    }
    while (1);

    printf("\n");

    terminazioneSicura();   
}
void terminazioneSicura(){
    //Chiusura e pulizia della memoria condivisa con annessi attach
    shmdt((void *) board);
    shmctl(shmid, IPC_RMID, NULL);
}
void printBoard(){
    for(int i = 0; i < BOARD_SIZE; i++){
        if(i && i % 3 == 0){
            printf("\n-------\n");
        }
        printf("%c|", board[i]);
    }
}
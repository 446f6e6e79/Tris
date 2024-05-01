#include<stdio.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h> 
#include <sys/types.h> 
#include "errExit.h"


#define BOARD_SIZE 9

/*
    Definisco il tipo sharedData. 
*/
typedef struct{
    char player1;
    char player2;
    char * player1Name;
    char * player2Name;
    char board[BOARD_SIZE];
}sharedData;


//Definizione prototipi
void printBoard();
void terminazioneSicura();
void firstSigIntHandler(int sig);
void secondSigIntHandler(int sig);

int checKVerticalWin();
int checkHorizontalWin();
int checkDiagonalWin();
int checkFull();
int checkResult();

void firstSigIntHandler(int sig){
    printf("\nÈ stato premuto CTRL-C.\nUna seconda pressione comporta la terminazione!\n");
    //Cambio ora il comportamento al segnale sigInt
    signal(SIGINT, secondSigIntHandler);
}

void secondSigIntHandler(int sig){
    terminazioneSicura();
    printf("\nIl gioco è stato terminato.\n");
    exit(0);
}
//Cambio del turno alla ricezione del segnale SIGALRM
void sigAlarmHandler(int sig){
    
}

sharedData * sD;
int shmid;

int main(int argC, char * argV[]){

    int timeOut;
    
    //Controllo sui parametri passati
    if(argC != 4){
        printf("Usage: %s <timeout> <SimboloPlayer1> <SimboloPlayer2>\n", argV[0]);
        return 1;
    }
    //Converto il valore di timeout in un intero
    timeOut = atoi(argV[1]);
    if(timeOut < 0){
        errExit("TimeOut >= 0");
    }
    //Se timeOut != 0, imposto il sigALRM handler
    else if(timeOut){
        if (signal(SIGALRM, sigAlarmHandler) == SIG_ERR)
            errExit("change signal handler failed");
    }

    //Verifico la lunghezza dei simboli inseriti
    if(strlen(argV[2]) != 1 || strlen(argV[3])!= 1){
        errExit("Simboli devono essere caratteri");
    }
    
    //Setto il nuvo comportamento dei segnali
    signal(SIGINT, firstSigIntHandler);

    
    //Generazione della memoria condivisa
    shmid = shmget(1234, sizeof(sharedData), 0666|IPC_CREAT);
    if(shmid < 0){
        errExit("Errore nella generazione della memoria condivisa\n");
    }
    
    //Attacco l'array board alla zona di memoria condivisa
    sD = shmat(shmid, NULL, 0);
    if(sD == (void *)-1){
        errExit("Errore nell'attach alla memoria condivisa\n");
    }
    sD = (sharedData *)sD;
    
    //Inizializzazione della memoria condivisa
    sD -> player1 = argV[2][0];
    sD -> player2 = argV[3][0];
    printf("Player 1: %c\n", sD->player1);
    printf("Player 2: %c\n", sD->player2);
    for(int i = 0; i < BOARD_SIZE; i++){
        sD -> board[i] = ' ';
    }

    printBoard();

    for(int i=0; i<2; i++){
        pid_t pid = fork();
        if(pid<0){
            errExit("Errore nella creazione della fork");
        }
        if(pid == 0){
            //Apre un ulteriore terminal con sopra TrisClient
            execl("/usr/bin/x-terminal-emulator", "x-terminal-emulator", "-e", "./TrisClient", NULL);
            errExit("Errore nella exec");
        }
    }

    printf("\n");
    while(1);
    terminazioneSicura();   
}
void terminazioneSicura(){
    //Chiusura e pulizia della memoria condivisa con annessi attach
    shmdt((void *) sD);
    shmctl(shmid, IPC_RMID, NULL);
}
void printBoard(){
    for (int i = 0; i < BOARD_SIZE; i++){
        printf(" %c ", sD->board[i]);
        //Se sono nella cella più  a DX
        if ( (i + 1) % 3 == 0){
            printf("\n");
            //Se non sono nell'ultima riga
            if (i < BOARD_SIZE - 1){
                printf("---|---|---\n");
            }
        }
        else{
            printf("|");
        }
    }
}
/*Controlla il caso di vittoria verticale*/
/*Possibili return
    0 : nessuna vittoria
    1 : vittoria player1
    2 : vittoria player2
*/
int checkVerticalWin(){
    char * board = sD -> board;
    char player1 = sD -> player1;
    char player2 = sD -> player2;
    for(int i = 0; i < 3; i++){
        if(board[i] != ' ' && board[i] == board[i + 3] && board[i] == board[i + 6]){
            if(player1 == board[i]){
                return 1;
            }else{
                return 2;
            }
        }
    }
    return 0;
}

/*Controlla il caso di vittoria orizzontale*/
int checkHorizontalWin(){
    char * board = sD -> board;
    char player1 = sD -> player1;
    char player2 = sD -> player2;
    for(int i = 0; i < 3; i++){
        if(board[i * 3] != ' ' && board[i*3] == board[i*3 + 1] && board[i*3] == board[i*3 + 2]){
            if(player1 == board[i*3]){
                return 1;
            }else{
                return 2;
            }
        }
    }
    return 0;
}
int checkDiagonalWin(){

    char *board = sD->board;
    char player1 = sD -> player1;
    char player2 = sD -> player2;

    if (board[0] != ' ' && board[0] == board[4] && board[0] == board[8]){
        if(player1 == board[0]){
                return 1;
            }else{
                return 2;
            }
    }
    if (board[2] != ' ' && board[2] == board[4] && board[2] == board[6]){
        if(player1 == board[2]){
                return 1;
            }else{
                return 2;
            }
    }
    return 0;
}

int checkFull(){
    char *board = sD->board;
    for(int i=0;i<BOARD_SIZE;i++){
        if(board[i]==' '){
            return 0;
        }
    }
    return 1;
}
//Ritorna 0 in caso di gioco non terminato oppure 1/2 a seconda del player che ha vinto
int checkResult(){
    if(checkHorizontalWin()!=0){
        return checkHorizontalWin();
    }
    if(checkVerticalWin()!=0){
        return checkVerticalWin();
    }
    if(checkDiagonalWin()!=0){
        return checkDiagonalWin();
    }
    return checkFull;
}

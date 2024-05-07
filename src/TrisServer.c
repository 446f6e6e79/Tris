#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h> 
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h> 
 
#include "errExit.h"
#include "semaphore.h"
#include "utils.h"

/* Definizioni variabili globali */
sharedData * sD;
int shmid;
int semID;
int turn = 1;

//Definizione prototipi
void firstSigIntHandler(int sig);
void secondSigIntHandler(int sig);
void sigAlarmHandler(int sig);

void terminazioneSicura();

int checKVerticalWin();
int checkHorizontalWin();
int checkDiagonalWin();
int checkFull();
int checkResult();

void firstSigIntHandler(int sig){
    printf("\nÈ stato premuto CTRL-C.\nUna seconda pressione comporta la terminazione!\n");
    //Cambio ora il comportamento al segnale sigInt
    if (signal(SIGINT, firstSigIntHandler) == SIG_ERR) {
        terminazioneSicura();
        errExit("Error registering SIGINT handler");
    }
}

void secondSigIntHandler(int sig){
    printf("\nIl gioco è stato terminato.\n");
    terminazioneSicura();
    exit(0);
}

//Cambio del turno alla ricezione del segnale SIGALRM
void sigAlarmHandler(int sig){
    //Resetto il comportamento di CTRL-C
    signal(SIGINT, firstSigIntHandler);

    sD -> stato = 3;
    if (kill(sD->pids[sD->activePlayerIndex], SIGUSR1) == -1) {
        perror("Errore nella fine TimeOut");
        exit(EXIT_FAILURE);
    }
}

/*
    INIZIO MAIN
*/
int main(int argC, char * argV[]){

    //Setto il nuvo comportamento dei segnali
    if (signal(SIGINT, firstSigIntHandler) == SIG_ERR) {
        errExit("Errore nel SIGINT Handler");
    }

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
    
    //Generazione della memoria condivisa
    shmid = shmget(69, sizeof(sharedData), 0666 | IPC_CREAT | IPC_EXCL);
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
    sD -> player[0] = argV[2][0];
    sD -> player[1] = argV[3][0];
    sD -> activePlayer = 0;
    sD -> pids[0] = getpid();
    printf("Player 1: %c\n", sD->player[0]);
    printf("Player 2: %c\n", sD->player[1]);
    for(int i = 0; i < BOARD_SIZE; i++){
        sD -> board[i] = ' ';
    }

    //Iniziallizzazione dei semafori
    semID =  semget(70, NUM_SEM, IPC_CREAT | IPC_EXCL | 0666 );
    if(semID == -1){
        errExit("Errore nella get del semaforo\n");
    }

    unsigned short values[4] = {1, 0, 0, 0};
    union semun arg;
    arg.array = values;

    if (semctl(semID, 0, SETALL, arg) == -1){
        terminazioneSicura();
        printf("semctl GETALL failed");
        return 1;
    }
    
    /*
        Devo attendere che entrambi i processi siano collegati
    */
    s_wait(semID, 3);

    //Libero il semaforo del primoPlayer
    s_signal(semID, 1);

    /**************************************************+
                    INIZIO GIOCO
    **************************************************+*/

    do{
        //Resetta l'alarm precedente, se presente
        alarm(0);

        //Inizializzo un nuovo timer
        alarm(timeOut);
        
        //Attende fino a che activePlayer non ha eseguito la sua mossa


        int win = checkResult();
        //Se c'è un vincitore
        if(checkResult()){
            //Setto stato a vittoria
            sD -> stato = win;
        }
        else if(checkFull()){
            //Setto stato a pareggio
            sD -> stato = 0;
        }
        if (kill(sD->pids[1], SIGUSR1) == -1 || kill(sD->pids[2], SIGUSR1) == -1) {
            perror("Errore nell'invio del segnale al client");
        }
    }while(1);

    terminazioneSicura();   
}


void terminazioneSicura(){
    //Chiusura e pulizia della memoria condivisa con annessi attach
    printf("TERMINAZIONE SICURA AVVIATA\n");
    shmdt((void *) sD);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semID, 0, IPC_RMID, NULL);
}

/*Controlla il caso di vittoria verticale*/
/*Possibili return
    0 : nessuna vittoria
    1 : vittoria player1
    2 : vittoria player2
*/
int checkVerticalWin(){
    char * board = sD -> board;
    char player1 = sD -> player[0];
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
    char player1 = sD -> player[0];
    
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
    char player1 = sD -> player[0];

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
    return checkFull();
}

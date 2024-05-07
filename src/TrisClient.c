#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h> 
#include <string.h>
#include "errExit.h"
#include "semaphore.h"
#include "utils.h"

#define BOARD_SIZE 9
#define SEM_SERVER 3
#define BUFF_SIZE 64

//Definizione variabili globali
int shmid;
sharedData *sD;
int  playerIndex;
int otherPlayerIndex;
int semID;

//Definizione prototipi
void firstSigIntHandler(int sig);
void secondSigIntHandler(int sig);
void sigAlarmHandler(int sig);

void terminazioneSicura();
void printBoard();
int getPlayIndex();

void cleanInputBuffer();

void sigUser1Handler(int sig){
    //Resetto il comportamento di CTRL-C
    if (signal(SIGINT, firstSigIntHandler) == SIG_ERR) {
        errExit("Errore nel SIGINT Handler");
    }

    switch(sD->stato){
        case 0:
            printf("\nLa partita è terminata in pareggio!\n");
            break;
        case 1:
        case 2:
            if(sD->stato == playerIndex){
                printBoard();
                printf("\nComplimenti! Hai vinto!\n");
            }
            else{
                printBoard();
                printf("\nHai Perso!\n");
            }
            terminazioneSicura();
            break;
        
        case 3: //Fine TIME-OUT
            //Sblocco il processo SERVER    
            printBoard();
            printf("Time-out scaduto!\n");
            printf("\nIn attesa che %s faccia la sua mossa!\n", sD->playerName[otherPlayerIndex - 1]); 

            //Mi metto in attesa che, l'altro giocatore esegua la mossa
            s_wait(semID, playerIndex);
            printBoard();
            printf("Inserisci coordinate posizione (x y)\n");
            break;
    }
}

void firstSigIntHandler(int sig){
    printf("\nÈ stato premuto CTRL-C.\nUna seconda pressione comporta la terminazione!\n");
    //Cambio ora il comportamento al segnale sigInt
    if (signal(SIGINT, secondSigIntHandler) == SIG_ERR) {
        terminazioneSicura();
        errExit("Error registering SIGINT handler");
    }
}

void secondSigIntHandler(int sig){
    terminazioneSicura();
    printf("\nIl gioco è stato terminato.\n");
    exit(1);
}

int main(int argC, char * argV[]) {
    //Setto il nuvo comportamento dei segnali
    if (signal(SIGINT, firstSigIntHandler) == SIG_ERR) {
        errExit("Errore nel SIGINT Handler");
    }
    if (signal(SIGUSR1, sigUser1Handler) == SIG_ERR) {
        errExit("Errore nel SIGUSR1 Handler");
    }
    
    if(argC < 2 || argC > 3){
        printf("Usage: %s <nomeUtente>", argV[0]);
        return 1;
    }
    if(argC == 3 && argV[2][0] == '*'){
        //Gioca Contro il PC
    }
    
    //Inizialzzazione dei semadori
    semID =  semget(70, NUM_SEM , IPC_CREAT | 0666);
    if(semID == -1){
        errExit("Errore nella get del semaforo\n");
    }
    
    //Recupero lo shareMemoryID usando la systemCall shmget
    shmid = shmget(69, sizeof(sharedData), 0666);
    if (shmid < 0) {
        errExit("Errore nella generazione della memoria condivisa\n");
    }

    sD = (sharedData *)shmat(shmid, NULL, 0);
    if (sD == (void *)-1) {
        errExit("Errore nell'attach alla memoria condivisa\n");
    }

    /*
        player1 -> playerIndex = 1
        player2 -> playerIndex = 2
    */
    //P(s) sul primo semaforo, necessario per il set delle variabili (SEZIONE CRITICA)
    s_wait(semID, 0);
        sD->activePlayer++;
        playerIndex = sD->activePlayer;
        strcpy(sD->playerName[playerIndex - 1], argV[1]);
        sD->pids[playerIndex] = getpid();

        if(playerIndex == 2){
            semOp(semID, SEM_SERVER, +3);
        }
        else{
            printf("In attesa dell'altro giocatore!\n");
        }

    //Libero il semaforo di mutua esclusione al secondo processo
    s_signal(semID, 0);

    //Rimango in attesa, fino a che entrambi i giocatori sono attivi
    s_wait(semID, SEM_SERVER);

    /*
        La memoria condivisa è stata correttamente settata, può ora iniziare il gioco
    */
    
    otherPlayerIndex = (playerIndex%2)+1;
    do{    
        printBoard();
        printf("\nIn attesa che %s faccia la sua mossa!\n", sD->playerName[otherPlayerIndex - 1]); 
        //Attende il proprio turno
        s_wait(semID, playerIndex);
        printBoard();
        int index = getPlayIndex();
        sD->board[index] = sD->player[playerIndex - 1];

        //Avvisa il processo Server, che una mossa è stata eseguita
        s_signal(semID, SEM_SERVER);
    }while(1);
    
    terminazioneSicura();
    return 0;
}

void terminazioneSicura(){
    //Chiusura memoria condivisa con attach
    shmdt((void *) sD);
    exit(1);
}

int getPlayIndex(){
    int x,y, index;
    do{
        printf("Inserisci coordinate posizione (x y)\n");
        scanf("%d %d", &x, &y);
        index = (3 * (y - 1)) + x - 1;
        if((x >= 1 && x <= 3) && 
            (y >= 1 && y <= 3) &&
            sD->board[index] == ' '){
            return index;
        }
        
        printf("Input non valido!\n");
    }
    while(1);
}

void printBoard(){
    system("clear");
    for (int i = 0; i < BOARD_SIZE; i++){
        printf(" %c ", sD->board[i]);
        //Se sono nella cella più a DX
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


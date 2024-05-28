/************************************
*VR485945, VR485743
*Davide Donà, Andrea Blushi
*Data di realizzazione: 06-05-24
*************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h> 
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <wait.h>
#include "errExit.h"
#include "semaphore.h"
#include "utils.h"


#define PID_SERVER 0

//Definizione variabili globali
int shmid;
sharedData *sD;
int playerIndex;

int semID;

//Definizione prototipi
void firstSigIntHandler(int sig);
void secondSigIntHandler(int sig);
void sigAlarmHandler(int sig);
void sigUser1Handler(int sig);
void sigUser2Handler(int sig);
void cleanBuffer();
sharedData * getSharedMemoryPointer(int);
void terminazioneSicura();
void printBoard();
int getPlayIndex();
void comunicaDisconnessione();
int getBotPlay();

/*
    Definisco l'HANDLER per il segnale SIGUSR1. A seconda del valore della variabile stato, assume comportamenti diversi:à
        - stato = 0 -> Partita terminata in pareggio
        - stato = 1 or stato = 2 -> Uno dei due processi ha vinto
        - stato = 3 -> SERVER DISCONNESSO
        - stato = 4 -> l'altro giocatore si è disconnesso dalla partita
*/
void sigUser1Handler(int sig) {
    printBoard();
    switch(sD->stato) {
        // La partita termina in pareggio
        case 0: 
            printf("\nLa partita è terminata in pareggio!\nVerrà iniziata una nuova partita\n");
            break;

        // Nella partita avviene una vittoria    
        case 1: 
        case 2:
            if(sD->stato == playerIndex) {
                printf("\nComplimenti! Hai vinto!\n");
            } else {
                printf("\nHai Perso!\n");
            }
            break;

        // Processo server disconnesso
        case 3: 
            system("clear");
            printf("Il processo Server è stato terminato\n");
            break;
        // L'altro giocatore si è disconnesso
        case 4: 
            printf("L'altro giocatore ha abbandonato la partita\nHai vinto a tavolino!");
            break;
    }
    s_signal(semID, SEM_SERVER);
    terminazioneSicura();
}

/*
    Handler per il segnale SIGUSR2:
        nel processo giocatore, la ricezione di tale segnale rappresenta 
        la terminazione del tempo per eseguira la mossa.
*/
void sigUser2Handler(int sig){
    printBoard();
    printf("Time-out scaduto!\n");
    printf("\nIn attesa che %s faccia la sua mossa!\n", sD->playerName[getOtherPlayerIndex(playerIndex) - 1]); 
    //Mi metto in attesa e passo il turno attraverso il server
    s_signal(semID, SEM_SERVER);
    s_wait(semID, playerIndex);    
    printBoard();
    printf("Inserisci coordinate posizione (x y)\n");
}

/*
    Handler per il segnale SIGINT:
        una volta premuto CTRL-C per la prima volta imposto il comportamento per la seconda pressione dello stesso tasto
*/
void firstSigIntHandler(int sig){
    printf("\nÈ stato premuto CTRL-C.\nUna seconda pressione comporta la terminazione!\n");
    //Cambio ora il comportamento al segnale sigInt
    if (signal(SIGINT, secondSigIntHandler) == SIG_ERR) {
        terminazioneSicura();
        errExit("Error registering SIGINT handler");
    }
    /* Faccio partire un alarm di 5 secondi.
        Se arriva prima che il processo riprema ctrl + c, resetto il comportamento iniziale
    */ 
    alarm(5);
}

/*
    Handler per il secondo segnale SIGINT:
        - Avviso il processo server dell'abbandono della partita
        - Abbandono la partita, terminando in modo sicuro il processo.
*/
void secondSigIntHandler(int sig){
    s_wait(semID, SEM_MUTEX);
    //Aggiorno i valori della memoria condivisa
    sD->indexPlayerLefted = playerIndex;
 
    comunicaDisconnessione();
    
    //Sblocco il semaforo di MUTEX
    s_signal(semID, SEM_MUTEX);
    
    //Sveglio a metà il server in maniera tale da permettere la sua terminazione

    s_signal(semID, SEM_SERVER);

    printf("\nHai abbandonato la partita.\n");
    terminazioneSicura();
}

//Allo scadere dell'alarm, resetto il comportamento di CTRL-C
void sigAlarmHandler(int sig){
    //Resetto il comportamento di CTRL-C ad ogni turno
    if (signal(SIGINT, firstSigIntHandler) == SIG_ERR ) {
        errExit("Errore nel SIGINT Handler");
    }
    printf("Resettato il comportamento di CTRL-C\n");
}

/***********************
    INIZIO MAIN
************************/
int main(int argC, char * argV[]) {
    if (signal(SIGALRM, sigAlarmHandler) == SIG_ERR) {
            errExit("change signal handler failed");
    }
    //Setto il nuvo comportamento dei segnali
    if (signal(SIGINT, firstSigIntHandler) == SIG_ERR ) {
        errExit("Errore nel SIGINT Handler");
    }
    if (signal(SIGUSR1, sigUser1Handler) == SIG_ERR) {
        errExit("Errore nel SIGUSR1 Handler");
    }
    if (signal(SIGUSR2, sigUser2Handler) == SIG_ERR) {
        errExit("Errore nel SIGUSR2 Handler");
    }
    
    //In caso di errore nel passaggio dei parametri, segnalo all'utente il corretto funzionamento
    if(argC < 2 || argC > 3){
        printf("Usage: %s <nomeUtente>", argV[0]);
        return 1;
    }
    
    //Recupero l'id dei semafori
    semID = getSemaforeID();

    //Recupero lo shareMemoryID
    shmid = sharedMemoryAttach();

    //Ottengo il puntatore all'area di memoria condivisa
    sD = getSharedMemoryPointer(shmid);
    
    /***********************************
     *  Inizializzazione memoria CONDIVISA
     *      essendo una sezione critica, l'accesso deve essere protetto
     *      da un semaforo MUTEX. 
     *      Un solo processo alla volta può modificare la memoria condivisa.
    ************************************/
    s_wait(semID, SEM_MUTEX);

        if(sD->activePlayer >= 2){                               
            printf("E' già stato raggiunto il numero massimo di giocatori\n");
            s_signal(semID, SEM_MUTEX);                         //Sblocco il semaforo
            terminazioneSicura();                               //Termino il processo.
        }

        //Selezionato di giocare contro il BOT
        if(argC == 3 && argV[2][0] == '*'){
            sD->playAgainstBot = 1;
        }
        sD->activePlayer++;
        playerIndex = sD->activePlayer;                         //Salvo, nella variabile playerIndex l'indice del giocatore
        strcpy(sD->playerName[playerIndex - 1], argV[1]);       //Copio nella memoria condivisa il nome del giocatore, passato come parameteo
        sD->pids[playerIndex] = getpid();
                                                                //Inserisco nell'array pids il pid del giocatore
    s_signal(semID, SEM_MUTEX);


    //Sblocco il processo server nel caso fossi il primo player
    if(playerIndex == 1){
        s_signal(semID, SEM_INIZIALIZZAZIONE);
    }
    //Se sono il secondo giocatore, sblocco tutti i processi in attesa
    if(playerIndex == 2){                           
        s_signal(semID, SEM_SERVER);
    }
    //Altrimenti sto in attesa dell'arrivo del secondo player
    else{
        printf("In attesa dell'altro giocatore!\n");
    }
    
    /*
        Rimango in attesa che almeno due processi giocatore siano connessi
    */

    s_wait(semID, playerIndex);
    /***********************************
     *      INIZIO DEL GIOCO
    ************************************/
    do{
        printBoard();
        printf("\nIn attesa che %s faccia la sua mossa!\n", sD->playerName[getOtherPlayerIndex(playerIndex) - 1]);
        
        /*
            Giocatore rimane in attesa sul proprio semaforo.
            Dovrà attendere che l'altro giocatore esegua la mossa, per essere sbloccato
        */
        s_wait(semID, playerIndex);

        /*
            Mutua esclusione garantita:
                - un solo processo giocatore attivo
                - processo server bloccato
        */
        printBoard();

        /*
            Lettura da input delle coordinate:
                - in caso di clientAutomatico, genero una coordinata casuale
        */
        int index = getPlayIndex();
        
        sD->board[index] = sD->player[playerIndex - 1];
        //Avvisa il processo Server, che una mossa è stata eseguita
        s_signal(semID, SEM_SERVER);
    }while(1);
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
    system("clear");
    sD->activePlayer--;
    //Avviso il server che entrambi abbiamo abbandonato
    if(kill(sD->pids[PID_SERVER], SIGUSR1) == -1) {
        printf("Errore nella comunicazione terminazione\n");
    }
    
}

//Acquisisce in input i dati della mossa, attuandone un controllo su di esse
int getPlayIndex(){
    if(playerIndex == 2 && sD->playAgainstBot){
        return getBotPlay();
    } 

    int x,y, index;
    do{
        cleanBuffer();
        printf("Inserisci le coordinate: (x y)\n");

        char input[100];
        fgets(input, sizeof(input), stdin);

        // Parse integers from the input string
        if (sscanf(input, "%d %d", &x, &y) == 2) {
            index = (3 * (y - 1)) + x - 1;

            if((x >= 1 && x <= 3) && 
                (y >= 1 && y <= 3) &&
                sD->board[index] == ' '){
                return index;
            }
        }
        printf("Input non valido!\n");
    }
    while(1);
}

void printBoard(){
        system("clear");
        printf("\n");
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
/*
    Ritorna il puntatore alla zona di memoria condivisa
*/
sharedData * getSharedMemoryPointer(int shmid){
    sharedData* sD = (sharedData *)shmat(shmid, NULL, 0);
    if (sD == (void *)-1) {
        errExit("Errore nell'attach alla memoria condivisa\n");
    }
    return sD;
}

void cleanBuffer() {
    // Ottiene i flag correnti del file descriptor associato a stdin (standard input)
    int flags = fcntl(fileno(stdin), F_GETFL, 0);
    // Imposta il file descriptor di stdin in modalità non bloccante
    fcntl(fileno(stdin), F_SETFL, flags | O_NONBLOCK);

    int c;
    // Legge e scarta tutti i caratteri presenti nel buffer di input fino a quando non è vuoto (EOF)
    while ((c = getchar()) != EOF) {}

    // Ripristina i flag originali del file descriptor di stdin
    fcntl(fileno(stdin), F_SETFL, flags); 
}


int getBotPlay() {
    int index;
    do {
        index = rand() % BOARD_SIZE;
    } while (sD->board[index] != ' ');

    return index;;
}
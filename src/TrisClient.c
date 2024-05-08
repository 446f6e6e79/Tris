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

sharedData * getSharedMemoryPointer(int);
void terminazioneSicura();
void printBoard();
int getPlayIndex();
void comunicaDisconnessione();

/*
    Definisco l'HANDLER per il segnale SIGUSR1. A seconda del valore della variabile stato, assume comportamenti diversi:à
        - stato = 0 -> Partita terminata in pareggio
        - stato = 1 or stato = 2 -> Uno dei due processi ha vinto
        - stato = 3 -> timer scaduto
        - stato = 4 -> l'altro giocatore si è disconnesso dalla partita
*/
void sigUser1Handler(int sig){
    //Resetto il comportamento di CTRL-C
    if (signal(SIGINT, firstSigIntHandler) == SIG_ERR) {
        errExit("Errore nel SIGINT Handler");
    }

    switch(sD->stato){
        case 0://La partita termina in pareggio
            printf("\nLa partita è terminata in pareggio!\n");
            terminazioneSicura();
            break;
        case 1://Nella partita avviene una vittoria
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
            printf("\nIn attesa che %s faccia la sua mossa!\n", sD->playerName[getOtherPlayerIndex(playerIndex) - 1]); 

            //Mi metto in attesa che, l'altro giocatore esegua la mossa
            s_wait(semID, playerIndex);
         

            printBoard();
            printf("Inserisci coordinate posizione (x y)\n");
            break;
        
        case 4: //L'altro giocatore si è disconnesso
            system("clear");
            s_wait(semID, SEM_MUTEX);

            printf("%s si è disconnesso\nHai vinto a tavolino!\n",sD->playerName[getOtherPlayerIndex(playerIndex) - 1]);
            printf("\nDesideri giocare ancora? [S, N]:\n");
            char c;
            scanf("%c", &c);
            while(c != 'S' && c != 'N'){
                system("clear");
                printf("\nDesideri giocare ancora? [S, N]:\n");
            }

            if(c == 'S'){
                //Resetto l'area di memoria condivisa, mettendomi come giocatore1
                if(playerIndex != 1){
                    playerIndex = 1;
                    strcpy(sD->playerName[playerIndex - 1], sD->playerName[playerIndex - 1]);
                    sD->pids[playerIndex] = getpid();
                    s_signal(semID, SEM_MUTEX);
                    
                }
                //Altrimenti l'are di memoria è già correttamente settata
                //TO DO: AVVIO UN ALTRA PARTITA
            }
            //Caso in cui non voglio più giocare
            else{
                sD->activePlayer--;
                comunicaDisconnessione();
                s_signal(semID, SEM_MUTEX);
                terminazioneSicura();
            }
            break;
    }
}
/*
    Handler per il segnale SIGUSR2:
        nel processo giocatore, la ricezione di tale segnale rappresenta la chiusura del server
*/
void sigUser2Handler(int sig){
    system("clear");
    errExit("Il processo Server è stato terminato\n");
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
    printf("\nHai abbandonato la partita.\n");
    terminazioneSicura();
}

/***********************
    INIZIO MAIN
************************/
int main(int argC, char * argV[]) {

    int otherPlayerIndex;
    
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
    
    //Selezionato di giocare contro il BOT
    if(argC == 3 && argV[2][0] == '*'){
        //Creo un processo figlio, eseguirà TrisBot
        pid_t pid = fork();
        if(pid == 0){
            execl("./bin/TrisBot", "TrisBot", NULL);
            errExit("Errore nella exec\n");
        }else if(pid < 0){
            errExit("Errore nella creazione del BOT\n");
        }
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
        //Se sono già presenti due giocatori
        if(sD->activePlayer >= 2){                               
            printf("E' già stato raggiunto il numero massimo di giocatori\n");
            s_signal(semID, SEM_MUTEX);                         //Sblocco il semaforo
            terminazioneSicura();                               //Termino il processo.
        }
        sD->activePlayer++;                                     //Incremento il numero di giocatoriAttivi
        playerIndex = sD->activePlayer;                         //Salvo, nella variabile playerIndex l'indice del giocatore
        strcpy(sD->playerName[playerIndex - 1], argV[1]);       //Copio nella memoria condivisa il nome del giocatore, passato come parameteo
        sD->pids[playerIndex] = getpid();                       //Inserisco nell'array pids il pid del giocatore
        
        //Se sono il secondo giocatore, sblocco tutti i processi in attesa
        if(playerIndex == 2){                                    
            semOp(semID, SEM_SERVER, +3);
        }
        //Altrimenti sto in attesa dell'arrivo del secondo player
        else{
            printf("In attesa dell'altro giocatore!\n");
        }

    //FINE SEZIONE CRITICA. Sblocco il semaforo MUTEX
    s_signal(semID, SEM_MUTEX);

    //Rimango in attesa, fino a che entrambi i giocatori sono attivi
    s_wait(semID, SEM_SERVER);

    /***********************************
     *      INIZIO DEL GIOCO
    ************************************/
    otherPlayerIndex = getOtherPlayerIndex(playerIndex);
    
    do{    
        printBoard();
        printf("\nIn attesa che %s faccia la sua mossa!\n", sD->playerName[otherPlayerIndex - 1]); 
        
        /*
            Giocatore rimane in attesa sul proprio semaforo.
            Dovrà attendere che l'altro giocatore esegua la mossa, per essere sbloccato
        */
        s_wait(semID, playerIndex);

        printBoard();
        
        //Lettura da input delle coordinate
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

//Acquisisce in input i dati della mossa, attuandone un controllo su di esse
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

sharedData * getSharedMemoryPointer(int shmid){
    sharedData* sD = (sharedData *)shmat(shmid, NULL, 0);
    if (sD == (void *)-1) {
        errExit("Errore nell'attach alla memoria condivisa\n");
    }
    return sD;
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
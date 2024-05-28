/************************************
*VR485945, VR485743
*Davide Donà, Andrea Blushi
*Data di realizzazione: 06-05-24
*************************************/

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
int activePlayerIndex = 1;

//Definizione prototipi
void firstSigIntHandler(int);
void secondSigIntHandler(int);
void sigAlarmHandler(int);

void terminazioneSicura();

void initializeEmptyBoard();
int checKVerticalWin();
int checkHorizontalWin();
int checkDiagonalWin();
int checkFull();
int checkResult();

//Handler per il primo SIGINT dovuto a "CRTL-C"
void firstSigIntHandler(int sig){
    printf("\nÈ stato premuto CTRL-C.\nUna seconda pressione comporta la terminazione!\n");
    //Cambio ora il comportamento al segnale sigInt
    if (signal(SIGINT, secondSigIntHandler) == SIG_ERR) {
        printf("Errore nel SIGINT handler\n");
        terminazioneSicura();
    }
}

//Handler per il secondo SIGINT dovuto a "CRTL-C"
void secondSigIntHandler(int sig){
    printf("\nIl gioco è stato terminato.\n");
    //Avviso il Client della chiusura del server
    s_wait(semID, SEM_MUTEX);
    sD->stato = 3;
    s_signal(semID, SEM_MUTEX);
    //Avverto entrambi i giocatori di aver terminato forzatamente
    if (kill(sD->pids[1], SIGUSR1) == -1 || kill(sD->pids[2], SIGUSR1) == -1) {
        printf("Errore nell'invio del segnale al client\n");
        terminazioneSicura();
    }
    terminazioneSicura();
    
}

//Cambio del turno alla ricezione del segnale SIGALRM
void sigAlarmHandler(int sig){
    s_wait(semID, SEM_MUTEX);
        sD -> stato = 3; 
    s_signal(semID, SEM_MUTEX);
    if (kill(sD->pids[activePlayerIndex], SIGUSR2) == -1) {
        printf("Errore nel segnale dell'alarm\n");
        terminazioneSicura();
    }
}

/* Handler che gestisce il caso: uno dei due processi si è disconnesso*/
void sigUsr1Handler(int sig){
    //Controllo quanti giocatori sono rimasti:
    s_wait(semID, SEM_MUTEX);
    //C'è ancora un giocatore attivo, invio il segnale comunicando la sua vittoria
    if(sD->activePlayer == 1){
        //Disattivo il time-out
        alarm(0);
        sD->stato = 4;
        //Se il player a chiudersi è il player attivo, ai fini di evitare il deadlock svegli il processo in attesa
        if(activePlayerIndex == sD->indexPlayerLefted ){
            s_signal(semID, getOtherPlayerIndex(sD->indexPlayerLefted) );
        }
        //Manda il segnale al processo ancora attivo
        if (kill(sD->pids[getOtherPlayerIndex(sD->indexPlayerLefted)], SIGUSR1) == -1){
            errExit("Errore nell'invio del messaggio: sigUsr1, stato = 4\n");
        }
    }
    //Attendo che leggano i risultati entrambi i player e poi chiudo
    printf("ATTENDO SU SEM_SERVER\n");
    semOp(semID, SEM_SERVER, -1);
    printf("FINITO ATTESO");
    terminazioneSicura();
}

/*  
    INIZIO MAIN
*/
int main(int argC, char * argV[]){
    //Setto il nuvo comportamento dei segnali
    if (signal(SIGINT, firstSigIntHandler) == SIG_ERR) {
        errExit("Errore nel SIGINT Handler");
    }
    if (signal(SIGUSR1, sigUsr1Handler) == SIG_ERR) {
        errExit("Errore nel USR1 Handler");
    }
    
    //Controllo sui parametri passati
    if(argC != 4){
        printf("Usage: %s <timeout> <SimboloPlayer1> <SimboloPlayer2>\n", argV[0]);
        return 1;
    }
    
    int timeOut;
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
    shmid = shmget(MEMORY_KEY, sizeof(sharedData), 0666 | IPC_EXCL | IPC_CREAT);
    if(shmid < 0){
        errExit("Errore nella generazione della memoria condivisa\n");
    }
    

    //Attacco l'array board alla zona di memoria condivisa
    sD = shmat(shmid, NULL, 0);
    if(sD == (void *)-1){
        shmctl(shmid, IPC_RMID, NULL); //Pulisco memoria
        errExit("Errore nell'attach alla memoria condivisa\n");
    }
    sD = (sharedData *)sD;
    

    //Iniziallizzazione e ottenimento dei semafori
    semID =  semget(SEM_KEY, NUM_SEM, IPC_CREAT | IPC_EXCL | 0666 );
    if(semID == -1){
        shmdt((void *) sD);//Pulisco memoria
        shmctl(shmid, IPC_RMID, NULL);
        errExit("Errore nella get del semaforo\n");
    }
    unsigned short values[5] = {1, 0, 0, 0, 0};
    union semun arg;
    arg.array = values;
    
    if (semctl(semID, 0, SETALL, arg) == -1){
        printf("semctl SET fallita");
        terminazioneSicura();
    }

    //Inizializzazione della memoria condivisa
    s_wait(semID, SEM_MUTEX);
        sD -> player[0] = argV[2][0];
        sD -> player[1] = argV[3][0];
        sD -> activePlayer = 0;
        sD -> pids[0] = getpid();
    
        //Inizializzo la board di gioco vuota
        initializeEmptyBoard();
    s_signal(semID, SEM_MUTEX);

    /* Rimango in attesa che si sia connesso il primo giocatore */
    s_wait(semID, SEM_INIZIALIZZAZIONE);
        s_wait(semID, SEM_MUTEX);
        //Se vuole giocare contro il bot:
        if(sD->playAgainstBot){
             //Creo un processo figlio, eseguirà TrisBot
            pid_t pid = fork();
            if(pid == 0){
                execl("./bin/TrisClient", "TrisClient", "Computer", NULL);
                errExit("Errore nella exec\n");
            }
            else if(pid < 0){
                errExit("Errore nella creazione del BOT\n");
            }
        }
        s_signal(semID, SEM_MUTEX);
    
    //Inizializzazione terminata
    printf("\nServer avviato correttamente\n");

    //Rimango in attesa che il secondo giocatore si connetta
    s_wait(semID, SEM_SERVER);
    //Libero il semaforo del primoPlayer
    s_signal(semID, 1);
    s_signal(semID, 2);

    //Sveglio il player 1 per giocare
    s_signal(semID, 1);

    /**************************************************
                    INIZIO GIOCO
    ***************************************************/

    
    int win;
    do{
        alarm(timeOut);

        //Attende fino a che activePlayer non ha eseguito la sua mossa
        s_wait(semID, SEM_SERVER);

        //Resetta l'alarm precedente, se presente
        alarm(0);

        win = checkResult();
        
        //Se c'è un vincitore
        if(win >= 0){
            //Setto stato a vittoria
            sD -> stato = win;
            //Avviso i client
            if (kill(sD->pids[1], SIGUSR1) == -1 || kill(sD->pids[2], SIGUSR1) == -1) {
                printf("Errore nell'invio dei segnali\n");
                terminazioneSicura();
            }
            terminazioneSicura();
        }
        else{
            //Aggiorna activePlayerIndex
            activePlayerIndex = getOtherPlayerIndex(activePlayerIndex);
            //Sblocca il giocatore Successivo
            s_signal(semID, activePlayerIndex);
        }
        
    }while(1);

}


void terminazioneSicura(){
    //Chiusura e pulizia della memoria condivisa con annessi attach
    printf("Terminazione del server eseguita correttamente\n");
    shmdt((void *) sD);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semID, 0, IPC_RMID, NULL);
    exit(0);
}

/*  Controlla le varie situazioni di vittoria. Possibili return:
    0 : nessuna vittoria
    1 : vittoria player1
    2 : vittoria player2
*/
int checkVerticalWin(){
    char * board = sD -> board;
    char player1 = sD -> player[0];
    for(int i = 0; i < 3; i++){
        if(board[i] != EMPTY && board[i] == board[i + 3] && board[i] == board[i + 6]){
            if(player1 == board[i]){
                return 1;
            }else{
                return 2;
            }
        }
    }
    return 0;
}

int checkHorizontalWin(){
    char * board = sD -> board;
    char player1 = sD -> player[0];
    
    for(int i = 0; i < 3; i++){
        if(board[i * 3] != EMPTY && board[i*3] == board[i*3 + 1] && board[i*3] == board[i*3 + 2]){
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

    if (board[0] != EMPTY && board[0] == board[4] && board[0] == board[8]){
        if(player1 == board[0]){
                return 1;
            }else{
                return 2;
            }
    }
    if (board[2] != EMPTY && board[2] == board[4] && board[2] == board[6]){
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
        if(board[i]==EMPTY){
            return -1;
        }
    }
    return 0;
}

/*  
    Funzione che verifica lo stato di gioco. Ritorna:
    - (-1) in caso di gioco non terminato,
    - (0) in caso di pareggio,
    - (1) vittoria giocatore1,
    - (2) vittoria giocatore2
*/
int checkResult(){
    int result;
    
    result = checkHorizontalWin();
    if(result != 0){
        return result;
    }

    result = checkVerticalWin();
    if(result != 0){
        return result;
    }

    result = checkDiagonalWin();
    if(result != 0){
        return result;
    }
    
    return checkFull();
}

void initializeEmptyBoard(){
    for(int i = 0; i < BOARD_SIZE; i++){
        sD -> board[i] = EMPTY;
    }
}
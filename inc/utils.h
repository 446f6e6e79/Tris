#include <unistd.h>
#ifndef _UTILS_HH
#define _UTILS_HH

//DEFINIZIONE DELLE LUNGHEZZE
#define BOARD_SIZE 9
#define NUM_SEM 4
#define NUM_PROCESSES 3
#define STR_LEN 65

//DEFINIZIONE DELLE KEY
#define MEMORY_KEY 69
#define SEM_KEY 70

//DEFINIZIONE DELLE LUNGHEZZE DEL SEMAFORO
#define SEM_SERVER 3
#define SEM_MUTEX 0

/*Stato 0 -> pareggio
        1 -> vittoria player1
        2 -> vittoria player2
        3 -> timeOut      
*/
typedef struct SharedData{
    char player[2];                 //Simboli del player
    char playerName[2][STR_LEN];    //Nomi del player
    int activePlayer;               //Index del player attivo attualmente
    char board[BOARD_SIZE];         //Tabella di gioco
    pid_t pids[NUM_PROCESSES];      //Array dei pid dei processi
    int stato;                      //Stato del gioco
    int indexPlayerLefted;          //Nel caso di chiusura, salva il processo che la richiede
}sharedData;

union semun {
    int val;                /* Value for SETVAL */
    struct semid_ds *buf;   /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *array;  /* Array for GETALL, SETALL */
};

//Definizione prototipi
int getOtherPlayerIndex(int);
int getSemaforeID();
int sharedMemoryAttach();

#endif
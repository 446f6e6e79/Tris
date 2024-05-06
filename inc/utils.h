#include <unistd.h>
#ifndef _UTILS_HH
#define _UTILS_HH

#define BOARD_SIZE 9
#define NUM_SEM 4
#define NUM_PROCESSES 3
#define STR_LEN 65

typedef struct SharedData{
    char player[2];
    char playerName[2][STR_LEN];
    int activePlayer;
    int activePlayerIndex;
    char board[BOARD_SIZE];
    pid_t pids[NUM_PROCESSES];
    int stato;
}sharedData;

union semun {
    int val;                /* Value for SETVAL */
    struct semid_ds *buf;   /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *array;  /* Array for GETALL, SETALL */
};

/*Stato 0 -> pareggio
        1 -> vittoria player1
        2 -> vittoria player2
        3 -> timeOut      
*/
#endif
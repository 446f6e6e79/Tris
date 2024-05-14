/************************************
*VR485945, VR485743
*Davide Donà, Andrea Blushi
*Data di realizzazione: 06-05-24
*************************************/

#include <sys/sem.h>
#include <stdio.h>
#include "errExit.h"
#include <errno.h>
#include <signal.h>


/*
    Metodo che esegue un'operazione sul semaforo:
    
    PARAMETRI: semid -> ID del semaforo, ritornato dalla funzione semget;
               sem_num -> indice del semaforo, su cui vogliamo fare l'operazione;
               sem_op -> operazione che vogliamo fare sul semaforo (+1, -1. ecc) 
*/
void semOp(int semid, unsigned short sem_num, short sem_op) {
    struct sembuf op;
    op.sem_num = sem_num;
    op.sem_op = sem_op;    
    int result = semop(semid, &op, 1);
    //Questo controllo verifica che l'operazione(wait) non sia stata interrota da un segnale
    //e nel caso riprende il suo comportamento
    while( result == -1 && errno == EINTR ){
        result = semop(semid, &op, 1);
    }
}
/*
    Esegue una wait (P(s)) sul semaforo.
    Cerca quindi di decrementare di 1 il semaforo, per occuparlo
*/
void s_wait(int semid, unsigned short sem_num){
    semOp(semid, sem_num, -1);
}

/*
    Esegue una signal (V(s)) sul semaforo.
    Incrementa di 1 il semaforo, per liberarlo ai processi in attesa
*/
void s_signal(int semid, unsigned short sem_num){
    semOp(semid, sem_num, 1);
}

void s_print(int semid) {
    struct semid_ds buf;
    unsigned short values[5];

    // Get current values of all semaphores
    if (semctl(semid, 0, GETALL, values) == -1) {
        perror("Error getting semaphore values");
        return;
    }

    // Print values of all semaphores
    printf("Semaphore values:\n");
    printf("SEM_MUTEX: %u\n", values[0]);
    printf("SEM_ONE: %u\n", values[1]);
    printf("SEM_TWO: %u\n", values[2]);
    printf("SEM_SERVER: %u\n", values[3]);
    printf("SEM_INIZIALIZZAZIONE: %u\n", values[4]);
}


/************************************
*VR485945, VR485743
*Davide Donà, Andrea Blushi
*Data di realizzazione: 06-05-24
*************************************/

#include <sys/sem.h>
#include <stdio.h>
#include "errExit.h"

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
    semop(semid, &op, 1);
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


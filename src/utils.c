#include "utils.h"

int getOtherPlayerIndex(int index){
    return (index % 2) + 1;
}

int getSemaforeID(){
    int semID =  semget(SEM_KEY, NUM_SEM, 0666);
    if(semID == -1){
        errExit("Errore nella get del semaforo\n");
    }
    return semID;
}
    
int sharedMemoryAttach(){
    //Recupero lo shareMemoryID usando la systemCall shmget
    int shmid = shmget(MEMORY_KEY, sizeof(sharedData), 0666);
    if (shmid < 0) {
        errExit("Errore nella generazione della memoria condivisa\n");
    }
    return shmid;
}

/************************************
*VR485945, VR485743
*Davide Donà, Andrea Blushi
*Data di realizzazione: 06-05-24
*************************************/

#include "utils.h"
#include <sys/sem.h>
#include <stdlib.h>
#include <sys/shm.h>

int getOtherPlayerIndex(int index){
    return (index % 2) + 1;
}

int getSemaforeID(){
    key_t key = ftok(KEY_PATHNAME, 's');
    if (key == -1) {
        errExit("Error generating key using ftok");
    }
    int semID =  semget(key, NUM_SEM, 0666);
    if(semID == -1){
        //Questo errore si verifica solo nel caso il server non venga avviato o avviato correttamente
        //perciò si avvisa l'utente del malfunzionamento del server o della sua assenza
        printf("Non è stato avviato il server\n");
        exit(1);
    }
    return semID;
}
    
int sharedMemoryAttach(){
    key_t key = ftok(KEY_PATHNAME, 'm');
    if (key == -1) {
        errExit("Error generating key using ftok");
    }
    //Recupero lo shareMemoryID usando la systemCall shmget
    int shmid = shmget(key, sizeof(sharedData), 0666);
    if (shmid < 0) {
        printf("Non è stato avviato il server\n");
        exit(1);
    }
    return shmid;
}

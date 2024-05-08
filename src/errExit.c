/************************************
*VR485945, VR485743
*Davide Donà, Andrea Blushi
*Data di realizzazione: 06-05-24
*************************************/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

void errExit(const char *msg) {
    perror(msg);
    exit(1);
}

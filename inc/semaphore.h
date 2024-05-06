#ifndef _SEMAPHORE_HH
#define _SEMAPHORE_HH

void semOp(int semid, unsigned short sem_num, short sem_op);
void s_wait(int semid, unsigned short sem_num);
void s_signal(int semid, unsigned short sem_num);
#endif

#include "z_sem.h"
#include <semaphore.h>
#include<string.h>
#include <memory.h>
#include <stdlib.h>
#include <stdint.h>
struct z_sem_t{
    sem_t handle;
};

z_sem_t* z_sem_create(){
    z_sem_t* sem = (z_sem_t*)malloc(sizeof(z_sem_t));
    memset(sem,0,sizeof(z_sem_t));
    sem_init(&sem->handle,0,0);
    return sem;
}

void z_sem_notify(z_sem_t* sem){
    sem_post(&sem->handle);
}

void z_sem_wait(z_sem_t* sem){
    sem_wait(&sem->handle);
}

void z_sem_free(z_sem_t* sem){
    sem_destroy(&sem->handle);
    free(sem);
}

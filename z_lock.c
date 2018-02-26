#include "z_lock.h"
#include <pthread.h>
#include<string.h>
#include <memory.h>
#include <stdlib.h>
#include <stdint.h>
struct z_lock_t {
    pthread_mutex_t mutext;
};

z_lock_t* z_lock_create(){
    z_lock_t* lock = (z_lock_t*)malloc(sizeof(z_lock_t));
    pthread_mutex_init(&lock->mutext, NULL);
    return lock;
}

void z_lock_free(z_lock_t* lock){
    pthread_mutex_destroy(&lock->mutext);
    free(lock);
}

void z_lock_lock(z_lock_t* lock){
    pthread_mutex_lock(&lock->mutext);
}

void z_lock_unlock(z_lock_t* lock){
    pthread_mutex_unlock(&lock->mutext);
}

void* z_lock_get_handle(z_lock_t* lock){
    return &(lock->mutext);
}

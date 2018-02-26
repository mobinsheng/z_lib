#include "z_cond.h"
#include <pthread.h>
#include "z_lock.h"
#include<string.h>
#include <memory.h>
#include <stdlib.h>
#include <stdint.h>
struct z_cond_t{
    pthread_cond_t cond;
    z_lock_t* lock;
};

z_cond_t* z_cond_create(struct z_lock_t* lock){
    z_cond_t* c = (z_cond_t*)malloc(sizeof(z_cond_t));
    memset(c,0,sizeof(z_cond_t));
    c->lock = lock;
    pthread_cond_init(&c->cond,NULL);
    return c;
}

void z_cond_free(z_cond_t* c){
    pthread_cond_destroy(&c->cond);
    free(c);
}

void z_cond_wait(z_cond_t* c){
    pthread_cond_wait(&c->cond,(pthread_mutex_t*)z_lock_get_handle(c->lock));
}

void z_cond_signal(z_cond_t* c){
    pthread_cond_signal(&c->cond);
}

void z_cond_broadcast(z_cond_t* c){
    pthread_cond_broadcast(&c->cond);
}

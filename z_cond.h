#ifndef Z_COND_H
#define Z_COND_H

#ifdef __cplusplus
extern "C"{
#endif

#include "z_lock.h"

typedef struct z_cond_t z_cond_t;

z_cond_t* z_cond_create(struct z_lock_t* lock);
void z_cond_free(z_cond_t* c);

void z_cond_wait(z_cond_t* c);
void z_cond_signal(z_cond_t* c);
void z_cond_broadcast(z_cond_t* c);

/*
 * 使用方式：
z_lock_t* lock = z_lock_create();
z_cond_t* cond = z_cond_create(lock);
int flag = 0;
void* thread_func1(void*){
    z_lock_lock(lock);
    while(!flag){
        z_cond_wait(cond);
    }
    flag = 0;
    z_lock_unlock(lock);
}

void* thread_func2(void*){
    z_lock_lock(lock);
    flag = 1;
    z_cond_signal(cond);
    z_lock_unlock(lock);
}

z_lock_free(lock);
*/

#ifdef __cplusplus
}
#endif

#endif // Z_COND_H

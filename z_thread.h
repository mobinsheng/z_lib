#ifndef Z_THREAD_H
#define Z_THREAD_H

#ifdef __cplusplus
extern "C"{
#endif

#include <pthread.h>

typedef void* (*z_thread_func_t)(void*);

typedef struct z_thread_t z_thread_t;

typedef struct z_thread_pool_t z_thread_pool_t;

z_thread_t* z_thread_create(z_thread_func_t func,void* param);
void z_thread_free(z_thread_t* th);
void z_thread_join(z_thread_t* th);


z_thread_pool_t* z_thread_pool_create(int threadNum);

void z_thread_pool_start(z_thread_pool_t* pool);

void z_thread_pool_stop(z_thread_pool_t* pool);

void z_thread_pool_free(z_thread_pool_t* pool);

void z_thread_pool_push_task(z_thread_pool_t* pool,z_thread_func_t func,void* param);

#ifdef __cplusplus
}
#endif

#endif // Z_THREAD_H

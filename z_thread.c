#include "z_thread.h"
#include "z_list.h"
#include "z_sem.h"
#include "z_cond.h"
#include "z_lock.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

struct z_thread_task_t{
    z_thread_func_t func;
    void* param;
};

typedef struct z_thread_task_t z_thread_task_t;

typedef enum z_thread_pool_state{
    THREAD_POOL_STATE_INVALID,
    THREAD_POOL_STATE_INITED,
    THREAD_POOL_STATE_STARTED,
    THREAD_POOL_STATE_STOPED,
}z_thread_pool_state;

typedef enum z_thread_state{
    THREAD_STATE_INVALID,
    THREAD_STATE_STARTED,
    THREAD_STATE_STOPED,
}z_thread_state;

#define Z_MAX_THREAD_NUM 128
#define Z_MIN_THREAD_NUM 4

#define panic(fmt,arg...) do{ char new_fmt[256];sprintf(new_fmt,"[%s][%s]","%s:%d",fmt);\
    fprintf(stderr,new_fmt, __FILE__, __LINE__,##arg);\
    exit(-1);\
    }while(0);

struct z_thread_t{
    pthread_t handle;
    int state;
};

z_thread_t* z_thread_create(z_thread_func_t func,void* param){
    z_thread_t* th = (z_thread_t*)malloc(sizeof(z_thread_t));
    if(NULL == th){
        panic("malloc failed!");
    }
    memset(th,0,sizeof(z_thread_t));
    pthread_create(&th->handle,NULL,func,param);
    th->state = THREAD_STATE_STARTED;
    return th;
}

void z_thread_free(z_thread_t* th){
    if(th == NULL){
        return;
    }

    if(th->state == THREAD_STATE_STARTED){
        z_thread_join(th);
    }

    free(th);
}

void z_thread_join(z_thread_t* th){
    if(th == NULL){
        return;
    }
    if(th->state != THREAD_STATE_STARTED){
        return;
    }

    pthread_join(th->handle,NULL);
    th->state = THREAD_STATE_STOPED;
}
//=====================================
struct z_thread_pool_t{
    z_thread_pool_state state;

    z_lock_t* pool_lock;
    z_cond_t* pool_cond;

    z_list_t* thread_list;
    int thread_num;
    int stoped_thread_num;

    z_list_t* task_list;
    z_lock_t* task_lock;
    z_cond_t* task_cond;
};

static void* thread_pool_run(void* param);

z_thread_pool_t* z_thread_pool_create(int threadNum){
    if(threadNum < Z_MIN_THREAD_NUM){
        threadNum = Z_MIN_THREAD_NUM;
    }

    if(threadNum > Z_MAX_THREAD_NUM){
        threadNum = Z_MAX_THREAD_NUM;
    }

    z_thread_pool_t* pool = (z_thread_pool_t*)malloc(sizeof(z_thread_pool_t));
    if(pool == NULL){
        panic("malloc failed!");
        return NULL;
    }

    pool->pool_lock = z_lock_create();
    pool->pool_cond = z_cond_create(pool->pool_lock);

    z_lock_lock(pool->pool_lock);
    pool->thread_list = z_list_create();
    pool->thread_num = threadNum;
    pool->stoped_thread_num = 0;

    pool->task_list = z_list_create();
    pool->task_lock = z_lock_create();
    pool->task_cond = z_cond_create(pool->task_lock);

    for(int i =0; i < threadNum; ++i){
        z_thread_t* th = z_thread_create(thread_pool_run,pool);
        z_list_pushback(pool->thread_list ,th);
    }

    pool->state = THREAD_POOL_STATE_INITED;

    z_lock_unlock(pool->pool_lock);
    return pool;
}

void z_thread_pool_start(z_thread_pool_t* pool){
    if(pool == NULL){
        return;
    }

    if(pool->state != THREAD_POOL_STATE_INITED){
        return;
    }

    z_lock_lock(pool->pool_lock);
    pool->state = THREAD_POOL_STATE_STARTED;
    z_cond_broadcast(pool->pool_cond);
    z_lock_unlock(pool->pool_lock);
}

void z_thread_pool_stop(z_thread_pool_t* pool){
    if(pool == NULL){
        return;
    }

    if(pool->state != THREAD_POOL_STATE_STARTED){
        return;
    }

    // 往list中塞入NULL用于表示线程池将要结束
    for(int i = 0; i < pool->thread_num; ++i){
        z_thread_pool_push_task(pool,NULL,NULL);
    }

    z_lock_lock(pool->pool_lock);
    while (pool->stoped_thread_num < pool->thread_num) {
        z_cond_wait(pool->pool_cond);
    }z_lock_unlock(pool->pool_lock);

    pool->state = THREAD_POOL_STATE_STOPED;

    z_lock_unlock(pool->pool_lock);
}

void z_thread_pool_free(z_thread_pool_t* pool){
    z_thread_pool_stop(pool);

    z_lock_lock(pool->pool_lock);

    while(z_list_size(pool->thread_list)){
        z_thread_t* th = (z_thread_t*)z_list_head(pool->thread_list)->ptr_val;
        z_list_popfront(pool->thread_list);
        z_thread_join(th);
        z_thread_free(th);
    }
    z_list_free(pool->thread_list);

    for(int i = 0; i < z_list_size(pool->task_list); ++i){
        z_thread_task_t* task = (z_thread_task_t*)z_list_head(pool->task_list)->ptr_val;
        z_list_popfront(pool->task_list);
        if(task == NULL){
            continue;
        }
        free(task);
    }
    z_list_free(pool->task_list);

    z_cond_free(pool->task_cond);
    z_lock_free(pool->task_lock);

    z_cond_free(pool->pool_cond);

    z_lock_unlock(pool->pool_lock);

    z_lock_free(pool->pool_lock);

    free(pool);
}

void z_thread_pool_push_task(z_thread_pool_t* pool,z_thread_func_t func,void* param){
    z_thread_task_t* t = (z_thread_task_t*)malloc(sizeof(z_thread_task_t));
    t->func = func;
    t->param = param;

    z_lock_lock(pool->task_lock);

    z_list_pushback(pool->task_list,t);

    z_cond_signal(pool->task_cond);

    z_lock_unlock(pool->task_lock);
}

z_thread_task_t* z_thread_pool_pop_task(z_thread_pool_t* pool){
    z_thread_task_t* t = NULL;

    z_lock_lock(pool->task_lock);
    while (z_list_size(pool->task_list) == 0) {
        z_cond_wait(pool->task_cond);
    }
    t = (z_thread_task_t*)z_list_head(pool->task_list)->ptr_val;
    z_list_popfront(pool->task_list);
    z_lock_unlock(pool->task_lock);

    return t;
}

static void* thread_pool_run(void* param){
    z_thread_pool_t* pool = (z_thread_pool_t*)param;
    if(pool == NULL){
        return NULL;
    }

    // 等待线程池就绪
    z_lock_lock(pool->pool_lock);
    while (pool->state != THREAD_POOL_STATE_STARTED) {
        z_cond_wait(pool->pool_cond);
    }
    z_lock_unlock(pool->pool_lock);

    // 线程池已经就绪

    while (pool->state == THREAD_POOL_STATE_STARTED) {
        z_thread_task_t* task = z_thread_pool_pop_task(pool);
        if(task == NULL){
            break;
        }

        if(task->func == NULL){
            free(task);
            break;
        }

        task->func(task->param);
        free(task);
    }

    z_lock_lock(pool->pool_lock);
    pool->stoped_thread_num++;
    if(pool->stoped_thread_num == pool->thread_num){
        z_cond_signal(pool->pool_cond);
    }
    z_lock_unlock(pool->pool_lock);

    return NULL;
}

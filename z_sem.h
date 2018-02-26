#ifndef Z_SEM_H
#define Z_SEM_H

#ifdef __cplusplus
extern "C"{
#endif

typedef struct z_sem_t z_sem_t;

z_sem_t* z_sem_create();

void z_sem_notify(z_sem_t* sem);

void z_sem_wait(z_sem_t* sem);

void z_sem_free(z_sem_t* sem);

#ifdef __cplusplus
}
#endif

#endif // Z_SEM_H

#ifndef Z_LOCK_H
#define Z_LOCK_H

#ifdef __cplusplus
extern "C"{
#endif
typedef struct z_lock_t z_lock_t;

z_lock_t* z_lock_create();
void z_lock_free(z_lock_t* lock);

void z_lock_lock(z_lock_t* lock);
void z_lock_unlock(z_lock_t* lock);

void* z_lock_get_handle(z_lock_t* lock);
#ifdef __cplusplus
}
#endif

#endif // Z_LOCK_H

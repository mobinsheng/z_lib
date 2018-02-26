#ifndef QUEUE_BUFFER_H
#define QUEUE_BUFFER_H
#include<malloc.h>
#include<stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

typedef struct z_circlebuffer_t z_circlebuffer_t;


z_circlebuffer_t* z_circlebuffer_create(size_t size);
void z_circlebuffer_free(z_circlebuffer_t* fifo);
/*
 * 返回写入的长度
 * 返回0表示写入失败
 */
int z_circlebuffer_write(struct z_circlebuffer_t *fifo,const unsigned char* data,const size_t len);

/*
 * 返回读取的数据长度
 * 返回0表示读取失败
 */
int z_circlebuffer_read(struct z_circlebuffer_t *fifo,unsigned char* buf,const size_t bufSize);

int z_circlebuffer_size(struct z_circlebuffer_t *fifo);

int z_circlebuffer_empty_space(struct z_circlebuffer_t *fifo);

#ifdef __cplusplus
}
#endif


#endif // QUEUE_BUFFER_H

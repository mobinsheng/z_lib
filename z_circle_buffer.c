#include "z_circle_buffer.h"
#include <memory.h>

/*溢出标志：0-正常，-1-溢出*/
#define FLAGS_OVERRUN 0x0001

struct z_circlebuffer_t{
    unsigned char *buf;
    size_t write_pos;
    size_t read_pos;
    size_t size;
    size_t free;
    int flags;
};

/*初始化*/
z_circlebuffer_t* z_circlebuffer_create(size_t size){
    z_circlebuffer_t* cbuf = (z_circlebuffer_t*)malloc(sizeof(z_circlebuffer_t));
    memset(cbuf,0,sizeof(z_circlebuffer_t));
    unsigned char *buf = (unsigned char *)malloc(size);

    cbuf->buf=buf;
    cbuf->flags=0;
    cbuf->free=size;
    cbuf->size=size;
    cbuf->write_pos=0;
    cbuf->read_pos=0;
    return cbuf;
}

void z_circlebuffer_free(z_circlebuffer_t* fifo){
    free(fifo->buf);
    free(fifo);
}

/*向FIFO 中写入数据 */
inline static int z_circlebuffer_putchar(struct z_circlebuffer_t *fifo,unsigned char data){
    if(fifo->free==0){
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }

    fifo->buf[fifo->write_pos] = data;
    fifo->write_pos++;
    //循环队列缓冲区
    if(fifo->write_pos == fifo->size){
        fifo->write_pos = 0;
    }
    fifo->free--;

    return 0;
}

int z_circlebuffer_write(struct z_circlebuffer_t *fifo,const unsigned char* data,const size_t len){
    int writeLen = 0;
    for(int i = 0; i < len ; ++i){
        if(z_circlebuffer_putchar(fifo,data[i]) == -1){
            break;
        }
        ++writeLen;
    }
    return writeLen;
}

/*从FIFO 中取出一个数据 */
inline static int z_circlebuffer_getchar(struct z_circlebuffer_t *fifo){
    int data;
    if(fifo->free == fifo->size){
        return -1;
    }
    data = fifo->read_pos;
    fifo->read_pos++;
    if(fifo->read_pos == fifo->size){//用来实现循环
        fifo->read_pos = 0;
    }
    fifo->free++;

    return data;
}

int z_circlebuffer_read(struct z_circlebuffer_t *fifo,unsigned char* buf,const size_t bufSize){
    int readLen = 0;
    for(int i = 0; i < bufSize; ++i){
        int data = z_circlebuffer_getchar(fifo);
        if(data == -1){
            break;
        }
        ++readLen;
        buf[i] = data;
    }
    return readLen;
}

/*缓冲区被使用容量*/
int z_circlebuffer_size(struct z_circlebuffer_t *fifo){
    return fifo->size-fifo->free;
}

/*缓冲区剩余容量*/
int z_circlebuffer_empty_space(struct z_circlebuffer_t *fifo){
    return fifo->free;
}

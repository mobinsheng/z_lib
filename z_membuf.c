#include "z_membuf.h"
#include "z_list.h"
#include<string.h>
#include <memory.h>
#include <stdlib.h>
#include <stdint.h>
static const int SEGMENT_BUF_SIZE = 1024 * 8;
static const int FREE_SEGMENT_COUNT = 4;
static const int PREPARE_SEGMENT_COUNT = 4;

struct z_membuf_t{
    struct u_mem_segment_t* segment_head;
    struct u_mem_segment_t* segment_tail;
    struct u_mem_segment_t* free_segment_head;
    struct u_mem_segment_t* free_segment_tail;
    size_t size;
    size_t free_segment_count;
};

typedef struct u_mem_segment_t{
    uint8_t* buf;
    size_t capacity;
    size_t read_pos;
    size_t write_pos;
    struct u_mem_segment_t* next;
}u_mem_segment_t;



static void u_mem_segment_init(u_mem_segment_t* seg){
    seg->buf = (uint8_t*)malloc(SEGMENT_BUF_SIZE);
    seg->read_pos = seg->write_pos =0;
    seg->capacity = SEGMENT_BUF_SIZE;
    seg->next = NULL;
}

static void u_mem_segment_destroy(u_mem_segment_t* seg){
    free(seg->buf);
    seg->buf = NULL;
    seg->read_pos = seg->write_pos = seg->capacity = 0;
}

static void u_mem_segment_clear(u_mem_segment_t* seg){
    seg->next = NULL;
    seg->read_pos = 0;
    seg->write_pos = 0;
}

static size_t u_mem_segment_write(u_mem_segment_t* seg,uint8_t* data,size_t size){
    if(seg->write_pos == seg->capacity){
        return 0;
    }

    size_t empty_space = seg->capacity - seg->write_pos;
    size_t write_len = 0;

    if(empty_space >= size ){
        write_len = size;
    }
    else{
        write_len = empty_space;
    }

    memcpy(seg->buf+seg->write_pos,data,write_len);
    seg->write_pos += write_len;
    return write_len;
}

static size_t u_mem_segment_read(u_mem_segment_t* seg,uint8_t* data,size_t size){
    if(seg->read_pos == seg->write_pos){
        return 0;
    }

    size_t readable = seg->write_pos - seg->read_pos;
    size_t read_len = 0;
    if(readable >= size){
        read_len = size;
    }
    else{
        read_len = readable;
    }

    memcpy(data,seg->buf+seg->read_pos,read_len);
    seg->read_pos += read_len;

    return read_len;
}

static size_t u_mem_segment_peek(u_mem_segment_t* seg,uint8_t* data,size_t size){
    if(seg->read_pos == seg->write_pos){
        return 0;
    }

    size_t readable = seg->write_pos - seg->read_pos;
    size_t read_len = 0;
    if(readable >= size){
        read_len = size;
    }
    else{
        read_len = readable;
    }

    memcpy(data,seg->buf+seg->read_pos,read_len);

    //seg->read_pos += read_len;

    return read_len;
}

static size_t u_mem_segment_skip(u_mem_segment_t* seg,size_t size){
    if(seg->read_pos == seg->write_pos){
        return 0;
    }

    size_t readable = seg->write_pos - seg->read_pos;
    size_t read_len = 0;
    if(readable >= size){
        read_len = size;
    }
    else{
        read_len = readable;
    }

    seg->read_pos += read_len;

    return read_len;
}

static size_t u_mem_segment_size(u_mem_segment_t* seg);

static size_t u_mem_segment_peekall(u_mem_segment_t* seg,uint8_t* data ){
    if(seg->read_pos == seg->write_pos){
        return 0;
    }

    size_t read_len = u_mem_segment_size(seg);

    memcpy(data,seg->buf+seg->read_pos,read_len);

    return read_len;
}

static size_t u_mem_segment_size(u_mem_segment_t* seg){
    return seg->write_pos - seg->read_pos;
}

//==============================

static void recovery_segment(z_membuf_t* membuf,u_mem_segment_t* seg){
    if(membuf->free_segment_count >= FREE_SEGMENT_COUNT){
        u_mem_segment_destroy(seg);
        free(seg);
        return;
    }

    u_mem_segment_clear(seg);

    ++membuf->free_segment_count;

    if(membuf->free_segment_tail == NULL){
        membuf->free_segment_head = membuf->free_segment_tail = seg;
        return;
    }

    membuf->free_segment_tail->next = seg;
    membuf->free_segment_tail = seg;
}

static u_mem_segment_t* get_free_segmemnt(z_membuf_t* membuf){
    u_mem_segment_t* seg = NULL;

    // 保证了free-list一定有节点
    if(membuf->free_segment_count == 0){
        for(int i = 0; i < PREPARE_SEGMENT_COUNT; ++i){
            seg = (u_mem_segment_t*)malloc(sizeof(u_mem_segment_t));
            u_mem_segment_init(seg);
            recovery_segment(membuf,seg);
        }
    }

    seg = membuf->free_segment_head;
    membuf->free_segment_head = seg->next;

    // 如果free-list空了
    if(membuf->free_segment_head == NULL){
        membuf->free_segment_tail = NULL;
    }

    u_mem_segment_clear(seg);

    --membuf->free_segment_count;

    return seg;

}


z_membuf_t* z_membuf_create(){
    z_membuf_t* membuf = (z_membuf_t*)malloc(sizeof(z_membuf_t));
    memset(membuf,0,sizeof(z_membuf_t));
    return membuf;
}

void z_membuf_free(z_membuf_t* membuf){
    u_mem_segment_t* seg = NULL;

    seg = membuf->segment_head;
    while(seg){
        u_mem_segment_t* next = seg->next;
        u_mem_segment_destroy(seg);
        free(seg);
        seg = next;
    }

    seg = membuf->free_segment_head;
    while(seg){
        u_mem_segment_t* next = seg->next;
        u_mem_segment_destroy(seg);
        free(seg);
        seg = next;
    }

    free(membuf);
}

size_t z_membuf_write(z_membuf_t* membuf,uint8_t* data,size_t size){
    u_mem_segment_t* seg = membuf->segment_tail;

    /* head和tail都是NULL，有两种情况：
     * 1、起始写入数据时，都为空
     * 2、读取数据时，如果把所有的数据都读取完，那么也都是空
     */
    if(seg == NULL){
        seg = get_free_segmemnt(membuf);
        membuf->segment_tail = seg;
        membuf->segment_head = seg;
    }

    size_t write_len = 0;

    while (write_len < size) {
        write_len += u_mem_segment_write(seg,data + write_len,size - write_len);
        if(write_len == size){
            break;
        }

        // 如果还有数据没有写完，那么表示当前segment空间不够，需要新的segment
        seg = get_free_segmemnt(membuf);
        membuf->segment_tail->next = seg;
        membuf->segment_tail = seg;
    }

    membuf->size += size;

    return size;
}

size_t z_membuf_read(z_membuf_t* membuf,uint8_t* data,size_t size){
    u_mem_segment_t* seg = NULL;

    if(membuf->size == 0 || membuf->segment_head == NULL){
        return 0;
    }

    seg = membuf->segment_head;

    size_t read_len = 0;

    while (seg != NULL && read_len < size) {
        int ret = u_mem_segment_read(seg,data + read_len,size - read_len);

        read_len += ret;

        /* 从这个segment中没有读取到数据，表示这个segment空了
         * 可以进行回收，然后从下一个segment中读取
         */
        if(ret == 0){
            u_mem_segment_t* next = seg->next;
            recovery_segment(membuf,seg);
            seg = next;

            membuf->segment_head = next;
            if(membuf->segment_head == NULL){
                membuf->segment_tail = NULL;
            }
        }
    }

    membuf->size -= read_len;

    return read_len;
}

size_t z_membuf_peek(z_membuf_t* membuf,uint8_t* data,size_t size){
    u_mem_segment_t* seg = NULL;

    if(membuf->size == 0 || membuf->segment_head == NULL){
        return 0;
    }

    seg = membuf->segment_head;

    size_t read_len = 0;

    while (seg != NULL && read_len < size) {
        size_t seg_size = u_mem_segment_size(seg);

        size_t ret =  0;
        // 一次性读取剩余的数据
        if(seg_size < (size - read_len)){
            ret = u_mem_segment_peekall(seg,data + read_len);
            seg = seg->next;
        }
        else{
            ret = u_mem_segment_peek(seg,data + read_len,size - read_len);
            if(ret == 0){
                seg = seg->next;
            }
        }

        read_len += ret;
    }

    // membuf->size -= read_len;

    return read_len;
}

size_t z_membuf_skip(z_membuf_t* membuf,size_t size){
    u_mem_segment_t* seg = NULL;

    if(membuf->size == 0 || membuf->segment_head == NULL){
        return 0;
    }

    seg = membuf->segment_head;

    size_t read_len = 0;

    while (seg != NULL && read_len < size) {
        int ret = u_mem_segment_skip(seg,size - read_len);

        read_len += ret;

        /* 从这个segment中没有读取到数据，表示这个segment空了
         * 可以进行回收，然后从下一个segment中读取
         */
        if(ret == 0){
            u_mem_segment_t* next = seg->next;
            recovery_segment(membuf,seg);
            seg = next;

            membuf->segment_head = next;
            if(membuf->segment_head == NULL){
                membuf->segment_tail = NULL;
            }
        }
    }

    membuf->size -= read_len;

    return read_len;
}

size_t z_membuf_size(z_membuf_t* membuf){
    return membuf->size;
}

//===========================================
uint64_t z_membuf_read_u64(z_membuf_t* membuf){
    uint64_t v = 0;
    z_membuf_read(membuf,(uint8_t*)&v,sizeof(uint64_t));
    return v;
}

uint32_t z_membuf_read_u32(z_membuf_t* membuf){
    uint32_t v = 0;
    z_membuf_read(membuf,(uint8_t*)&v,sizeof(uint32_t));
    return v;
}

uint16_t z_membuf_read_u16(z_membuf_t* membuf){
    uint16_t v = 0;
    z_membuf_read(membuf,(uint8_t*)&v,sizeof(uint16_t));
    return v;
}

uint8_t z_membuf_read_u8(z_membuf_t* membuf){
    uint8_t v = 0;
    z_membuf_read(membuf,(uint8_t*)&v,sizeof(uint8_t));
    return v;
}

int64_t z_membuf_read_i64(z_membuf_t* membuf){
    int64_t v = 0;
    z_membuf_read(membuf,(uint8_t*)&v,sizeof(int64_t));
    return v;
}

int32_t z_membuf_read_i32(z_membuf_t* membuf){
    int32_t v = 0;
    z_membuf_read(membuf,(uint8_t*)&v,sizeof(int32_t));
    return v;
}

int16_t z_membuf_read_i16(z_membuf_t* membuf){
    int16_t v = 0;
    z_membuf_read(membuf,(uint8_t*)&v,sizeof(int16_t));
    return v;
}

int8_t z_membuf_read_i8(z_membuf_t* membuf){
    int8_t v = 0;
    z_membuf_read(membuf,(uint8_t*)&v,sizeof(int8_t));
    return v;
}
//=================================
void z_membuf_write_u64(z_membuf_t* membuf,uint64_t v){
    z_membuf_write(membuf,(uint8_t*)&v,sizeof(v));
}

void z_membuf_write_u32(z_membuf_t* membuf,uint32_t v){
    z_membuf_write(membuf,(uint8_t*)&v,sizeof(v));
}

void z_membuf_write_u16(z_membuf_t* membuf,uint16_t v){
    z_membuf_write(membuf,(uint8_t*)&v,sizeof(v));
}

void z_membuf_write_u8(z_membuf_t* membuf,uint8_t v){
    z_membuf_write(membuf,(uint8_t*)&v,sizeof(v));
}


void z_membuf_write_i64(z_membuf_t* membuf,int64_t v){
    z_membuf_write(membuf,(uint8_t*)&v,sizeof(v));
}

void z_membuf_write_i32(z_membuf_t* membuf,int32_t v){
    z_membuf_write(membuf,(uint8_t*)&v,sizeof(v));
}

void z_membuf_write_i16(z_membuf_t* membuf,int16_t v){
    z_membuf_write(membuf,(uint8_t*)&v,sizeof(v));
}

void z_membuf_write_i8(z_membuf_t* membuf,int8_t v){
    z_membuf_write(membuf,(uint8_t*)&v,sizeof(v));
}


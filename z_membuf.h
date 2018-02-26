#ifndef z_membuf_H
#define z_membuf_H
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"{
#endif

typedef struct z_membuf_t z_membuf_t;


z_membuf_t* z_membuf_create();
void z_membuf_free(z_membuf_t* membuf);

size_t z_membuf_write(z_membuf_t* membuf,uint8_t* data,size_t size);
size_t z_membuf_read(z_membuf_t* membuf,uint8_t* data,size_t size);
size_t z_membuf_peek(z_membuf_t* membuf,uint8_t* data,size_t size);
size_t z_membuf_skip(z_membuf_t* membuf,size_t size);
size_t z_membuf_size(z_membuf_t* membuf);

//================================
uint64_t z_membuf_read_u64(z_membuf_t* membuf);
uint32_t z_membuf_read_u32(z_membuf_t* membuf);
uint16_t z_membuf_read_u16(z_membuf_t* membuf);
uint8_t z_membuf_read_u8(z_membuf_t* membuf);

int64_t z_membuf_read_i64(z_membuf_t* membuf);
int32_t z_membuf_read_i32(z_membuf_t* membuf);
int16_t z_membuf_read_i16(z_membuf_t* membuf);
int8_t z_membuf_read_i8(z_membuf_t* membuf);
//=================================
void z_membuf_write_u64(z_membuf_t* membuf,uint64_t v);
void z_membuf_write_u32(z_membuf_t* membuf,uint32_t v);
void z_membuf_write_u16(z_membuf_t* membuf,uint16_t v);
void z_membuf_write_u8(z_membuf_t* membuf,uint8_t v);

void z_membuf_write_i64(z_membuf_t* membuf,int64_t v);
void z_membuf_write_i32(z_membuf_t* membuf,int32_t v);
void z_membuf_write_i16(z_membuf_t* membuf,int16_t v);
void z_membuf_write_i8(z_membuf_t* membuf,int8_t v);

#ifdef __cplusplus
}
#endif
#endif // z_membuf_H

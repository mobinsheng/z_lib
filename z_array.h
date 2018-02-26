#ifndef z_array_H
#define z_array_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

typedef union {
    int64_t i64_val;
    uint64_t u64_val;
    int32_t i32_val;
    uint32_t u32_val;
    int16_t i16_val;
    uint16_t u16_val;
    int8_t i8_val;
    uint8_t u8_val;
    void* ptr_val;
} z_array_data_t;


typedef  struct z_array_t{
    z_array_data_t* elements;
    size_t size;
    size_t capacity;
}z_array_t;

z_array_t* z_array_create();
void z_array_free(z_array_t* array);
void z_array_pushback(z_array_t* array,z_array_data_t data);
z_array_data_t z_array_popback(z_array_t* array);
z_array_data_t z_array_at(z_array_t* array,size_t pos);
size_t z_array_size(z_array_t* array);

#ifdef __cplusplus
}
#endif

#endif // z_array_H

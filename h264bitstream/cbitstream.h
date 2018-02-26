#ifndef CBITSTREAM_H
#define CBITSTREAM_H
#include <stdlib.h>
#include <stdint.h>

typedef struct cbitstream{
    uint8_t* begin;
    uint8_t* p;
    uint8_t* end;

}cbitstream_t;
#endif // CBITSTREAM_H

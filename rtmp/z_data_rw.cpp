#include "z_data_rw.h"
#include <string.h>
#include <stdio.h>

#include "z_data_rw.h"


typedef union endian_checker endian_checker_t;

union endian_checker
{
    long endian_value;
    char byte_array[sizeof(long)];
};

static endian_checker_t EndianChecker = {1};


int z_is_little_endian(void)
{
    return EndianChecker.byte_array[0];
}


int z_is_big_endian(void)
{
    return !EndianChecker.byte_array[0];
}


int z_read_be16int(unsigned char *data)
{
    int value;

    if (z_is_little_endian()) {
        value  = data[0] << 8;
        value += data[1];
    } else {
        value  = data[0];
        value += data[1] <<  8;
    }
    return value;
}


int z_read_be24int(unsigned char *data)
{
    int value;

    if (z_is_little_endian()) {
        value  = data[0] << 16;
        value += data[1] <<  8;
        value += data[2];
    } else {
        value  = data[0];
        value += data[1] <<  8;
        value += data[2] << 16;
    }
    return value;
}


int z_read_le32int(unsigned char *data)
{
    int value;

    if (z_is_little_endian()) {
        value  = data[0];
        value += data[1] <<  8;
        value += data[2] << 16;
        value += data[3] << 24;
    } else {
        value  = data[0] << 24;
        value += data[1] << 16;
        value += data[2] <<  8;
        value += data[3];
    }
    return value;
}


int z_read_be32int(unsigned char *data)
{
    int value;

    if (z_is_little_endian()) {
        value  = data[0] << 24;
        value += data[1] << 16;
        value += data[2] <<  8;
        value += data[3];
    } else {
        value  = data[0];
        value += data[1] <<  8;
        value += data[2] << 16;
        value += data[3] << 24;
    }
    return value;
}


double z_read_be64double(unsigned char *data)
{
    double value;
    unsigned char number_data[8];

    if (z_is_little_endian()) {
        number_data[0] = data[7];
        number_data[1] = data[6];
        number_data[2] = data[5];
        number_data[3] = data[4];
        number_data[4] = data[3];
        number_data[5] = data[2];
        number_data[6] = data[1];
        number_data[7] = data[0];
        memmove(&value, number_data, 8);
    } else {
        memmove(&value, data, 8);
    }

    return value;
}


void z_write_be64double(unsigned char *data, double value)
{
    unsigned char number_data[8];

    if (z_is_little_endian()) {
        memmove(number_data, &value, 8);
        data[0] = number_data[7];
        data[1] = number_data[6];
        data[2] = number_data[5];
        data[3] = number_data[4];
        data[4] = number_data[3];
        data[5] = number_data[2];
        data[6] = number_data[1];
        data[7] = number_data[0];
    } else {
        memmove(data, &value, 8);
    }
}


void z_write_le32int(unsigned char *data, int value)
{
    if (z_is_little_endian()) {
        data[0] = value & 0xFF;
        data[1] = (value >> 8) & 0xFF;
        data[2] = (value >> 16) & 0xFF;
        data[3] = value >> 24;
    } else {
        data[0] = value >> 24;
        data[1] = (value >> 16) & 0xFF;
        data[2] = (value >> 8) & 0xFF;
        data[3] = value & 0xFF;
    }
}


void z_write_be32int(unsigned char *data, int value)
{
    if (z_is_little_endian()) {
        data[0] = value >> 24;
        data[1] = (value >> 16) & 0xFF;
        data[2] = (value >> 8) & 0xFF;
        data[3] = value & 0xFF;
    } else {
        data[0] = value & 0xFF;
        data[1] = (value >> 8) & 0xFF;
        data[2] = (value >> 16) & 0xFF;
        data[3] = value >> 24;
    }
}


void z_write_be24int(unsigned char *data, int value)
{
    if (z_is_little_endian()) {
        data[0] = value >> 16;
        data[1] = (value >> 8) & 0xFF;
        data[2] = value & 0xFF;
    } else {
        memmove(data, &value, 3);
    }
}


void z_write_be16int(unsigned char *data, int value)
{
    unsigned char int_data[2];

    if (z_is_little_endian()) {
        memmove(int_data, &value, 2);
        data[0] = int_data[1];
        data[1] = int_data[0];
    } else {
        memmove(data, &value, 2);
    }
}

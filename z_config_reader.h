#ifndef z_config_reader_H
#define z_config_reader_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#ifdef __cplusplus
extern "C"{
#endif

/** format
name:  jack  #comment
age: 18 // comment
id: jack123456
floatvalue: 1.236
 */
typedef struct z_config_reader_t z_config_reader_t;

z_config_reader_t* z_config_reader_create(const char* fileName);
void z_config_reader_free(z_config_reader_t* cfg);

///< write
/// return value: 0 success,1 failed
///int z_config_reader_write_int(z_config_reader_t* cfg,const char* key,int64_t value);
///int z_config_reader_write_float(z_config_reader_t* cfg,const char* key,double value);
///int z_config_reader_write_string(z_config_reader_t* cfg,const char* key,const char* value);

int z_config_reader_read_int(z_config_reader_t* cfg,const char* key,int64_t* value);
int z_config_reader_read_float(z_config_reader_t* cfg,const char* key,double* value);
int z_config_reader_read_string(z_config_reader_t* cfg,const char* key,char* value);


#ifdef __cplusplus
}
#endif

#endif // z_config_reader_H

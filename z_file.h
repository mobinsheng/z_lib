#ifndef Z_FILE_H
#define Z_FILE_H


#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

size_t z_file_size(const char* fileName);
int z_file_is_exist(const char* fileName);
int z_file_mkdir(const char* path);
int z_file_copy(const char* from,const char* to);
int z_file_move(const char* oldPath,const char* newPath);
int z_file_remove(const char* path);

#ifdef __cplusplus
}
#endif

#endif // Z_FILE_H

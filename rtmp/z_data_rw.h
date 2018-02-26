#ifndef Z_DATA_RW_H
#define Z_DATA_RW_H


/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif


extern int z_is_little_endian(void);
extern int z_is_big_endian(void);

extern int z_read_be16int(unsigned char *data);
extern int z_read_le32int(unsigned char *data);
extern int z_read_be32int(unsigned char *data);
extern int z_read_be24int(unsigned char *data);
extern double z_read_be64double(unsigned char *data);

extern void z_write_be16int(unsigned char *data, int value);
extern void z_write_be24int(unsigned char *data, int value);
extern void z_write_le32int(unsigned char *data, int value);
extern void z_write_be32int(unsigned char *data, int value);
extern void z_write_be64double(unsigned char *data, double value);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // Z_DATA_RW_H

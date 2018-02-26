#ifndef Z_AMF_PACKET_H
#define Z_AMF_PACKET_H


#include "z_rtmp.h"


/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif


#define AMF_CHANK_SIZE 128

typedef enum amf_datatype
{
    AMF_DATATYPE_NUMBER       = 0x00,
    AMF_DATATYPE_BOOLEAN      = 0x01,
    AMF_DATATYPE_STRING       = 0x02,
    AMF_DATATYPE_OBJECT       = 0x03,
    AMF_DATATYPE_MOVIECLIP    = 0x04,
    AMF_DATATYPE_NULL         = 0x05,
    AMF_DATATYPE_UNDEFINED    = 0x06,
    AMF_DATATYPE_REFERENCE    = 0x07,
    AMF_DATATYPE_ECMA_ARRAY   = 0x08,
    AMF_DATATYPE_OBJECT_END   = 0x09,
    AMF_DATATYPE_STRICT_ARRAY = 0x0A,
    AMF_DATATYPE_DATE         = 0x0B,
    AMF_DATATYPE_LONG_STRING  = 0x0C,
    AMF_DATATYPE_UNSUPPORTED  = 0x0D,
    AMF_DATATYPE_RECORDSET    = 0x0E,
    AMF_DATATYPE_XML_DOCUMENT = 0x0F,
    z_amf_datatype_tYPED_OBJECT = 0x10,
}z_amf_datatype_t;

typedef struct z_amf_packet_number_t z_amf_packet_number_t;
typedef struct z_amf_packet_boolean_t z_amf_packet_boolean_t;
typedef struct z_amf_packet_string_t z_amf_packet_string_t;
typedef struct z_amf_packet_object_property_t z_amf_packet_object_property_t;
typedef struct z_amf_packet_object_t z_amf_packet_object_t;
typedef struct z_amf_packet_null_t z_amf_packet_null_t;
typedef struct z_amf_packet_undefined_t z_amf_packet_undefined_t;
typedef struct z_amf_packet_ecma_array_property_t z_amf_packet_ecma_array_property_t;
typedef struct z_amf_packet_ecma_array_t z_amf_packet_ecma_array_t;
typedef struct z_amf_packet_object_end_t z_amf_packet_object_end_t;
typedef union z_amf_packet_t z_amf_packet_t;

struct z_amf_packet_number_t
{
    z_amf_datatype_t datatype;
    double value;
};

struct z_amf_packet_boolean_t
{
    z_amf_datatype_t datatype;
    int value;
};

struct z_amf_packet_string_t
{
    z_amf_datatype_t datatype;
    char *value;
};

struct z_amf_packet_object_property_t
{
    char *key;
    z_amf_packet_t *value;
    z_amf_packet_object_property_t *next;
};

struct z_amf_packet_object_t
{
    z_amf_datatype_t datatype;
    z_amf_packet_object_property_t *properties;
};

struct z_amf_packet_null_t
{
    z_amf_datatype_t datatype;
};

struct z_amf_packet_undefined_t
{
    z_amf_datatype_t datatype;
};

struct z_amf_packet_ecma_array_t
{
    z_amf_datatype_t datatype;
    int num;
    z_amf_packet_object_property_t *properties;
};

struct z_amf_packet_object_end_t
{
    z_amf_datatype_t datatype;
};

union z_amf_packet_t
{
    z_amf_datatype_t datatype;
    z_amf_packet_number_t number;
    z_amf_packet_boolean_t boolean;
    z_amf_packet_string_t string;
    z_amf_packet_object_t object;
    z_amf_packet_null_t null;
    z_amf_packet_undefined_t undefined;
    z_amf_packet_ecma_array_t ecma_array;
    z_amf_packet_object_end_t object_end;
};


extern z_amf_packet_t *z_amf_packet_analyze_data(
    unsigned char *data, size_t data_size, size_t *packet_size);

extern z_amf_packet_t *z_amf_packet_create_number(double number);
extern z_amf_packet_t *z_amf_packet_create_boolean(int boolean);
extern z_amf_packet_t *z_amf_packet_create_string(const char *string);
extern z_amf_packet_t *z_amf_packet_create_null(void);
extern z_amf_packet_t *z_amf_packet_create_undefined(void);
extern z_amf_packet_t *z_amf_packet_create_object(void);
extern z_rtmp_result_t z_amf_packet_add_property_to_object(
    z_amf_packet_t *amf, const char *key, z_amf_packet_t *value);

extern size_t z_amf_packet_get_size(z_amf_packet_t *amf);

extern size_t z_amf_packet_serialize(
    z_amf_packet_t *amf,
    unsigned char *output_buffer, size_t output_buffer_size);

extern void z_amf_packet_free(z_amf_packet_t *amf);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // Z_AMF_PACKET_H

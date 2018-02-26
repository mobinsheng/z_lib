#ifndef Z_RTMP_PACKET_H
#define Z_RTMP_PACKET_H


#include "z_rtmp.h"
#include "z_amf_packet.h"


/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif


#define HEADER_MAGIC_01 3
#define HEADER_MAGIC_04 2
#define HEADER_MAGIC_08 1
#define HEADER_MAGIC_12 0

typedef enum rtmp_datatype
{
    RTMP_DATATYPE_CHUNK_SIZE    = 0x01,
    RTMP_DATATYPE_UNKNOWN_0     = 0x02,
    RTMP_DATATYPE_BYTES_READ    = 0x03,
    RTMP_DATATYPE_PING          = 0x04,
    RTMP_DATATYPE_SERVER_BW     = 0x05,
    RTMP_DATATYPE_CLIENT_BW     = 0x06,
    RTMP_DATATYPE_UNKNOWN_1     = 0x07,
    RTMP_DATATYPE_AUDIO_DATA    = 0x08,
    RTMP_DATATYPE_VIDEO_DATA    = 0x09,
    RTMP_DATATYPE_UNKNOWN_2     = 0x0A,
    RTMP_DATATYPE_UNKNOWN_3     = 0x0B,
    RTMP_DATATYPE_UNKNOWN_4     = 0x0C,
    RTMP_DATATYPE_UNKNOWN_5     = 0x0D,
    RTMP_DATATYPE_UNKNOWN_6     = 0x0E,
    RTMP_DATATYPE_FLEX_STREAM   = 0x0F,
    RTMP_DATATYPE_FLEX_SHARED_OBJECT = 0x10,
    RTMP_DATATYPE_MESSAGE       = 0x11,
    RTMP_DATATYPE_NOTIFY        = 0x12,
    RTMP_DATATYPE_SHARED_OBJECT = 0x13,
    RTMP_DATATYPE_INVOKE        = 0x14,
    RTMP_DATATYPE_FLV_DATA      = 0x16,
}z_rtmp_datatype_t;

typedef enum rtmp_body_type
{
    RTMP_BODY_TYPE_AMF,
    RTMP_BODY_TYPE_DATA
}z_rtmp_body_type_t;

typedef struct z_rtmp_packet_inner_amf_t z_rtmp_packet_inner_amf_t;

struct z_rtmp_packet_inner_amf_t
{
    z_amf_packet_t *amf;
    z_rtmp_packet_inner_amf_t *next;
};

typedef struct z_rtmp_packet_t z_rtmp_packet_t;

struct z_rtmp_packet_t
{
    char object_id;
    long timer;
    z_rtmp_datatype_t data_type;
    long stream_id;
    z_rtmp_body_type_t body_type;
    z_rtmp_packet_inner_amf_t *inner_amf_packets;
    unsigned char *body_data;
    size_t body_data_length;
};


extern z_rtmp_packet_t *z_rtmp_packet_create(void);
extern void z_rtmp_packet_free(z_rtmp_packet_t *packet);

extern z_rtmp_result_t z_rtmp_packet_add_amf(
    z_rtmp_packet_t *packet,
    z_amf_packet_t *amf);
extern void z_rtmp_packet_cleanup(z_rtmp_packet_t *packet);

extern z_rtmp_result_t z_rtmp_packet_analyze_data(
    z_rtmp_packet_t *packet,
    unsigned char *data, size_t data_size,
    size_t amf_chunk_size,
    size_t *packet_size);

extern z_rtmp_result_t z_rtmp_packet_serialize(
    z_rtmp_packet_t *packet,
    unsigned char *output_buffer, size_t output_buffer_size,
    size_t amf_chunk_size,
    size_t *packet_size);

extern z_rtmp_result_t z_rtmp_packet_allocate_body_data(
    z_rtmp_packet_t *packet, size_t length);

extern void z_rtmp_packet_retrieve_status_info(
    z_rtmp_packet_t *packet, char **code, char **level);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // Z_RTMP_PACKET_H

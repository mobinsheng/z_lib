#ifndef Z_RTMP_H
#define Z_RTMP_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MACOS_OPENTRANSPORT
#include <OpenTransport.h>
#include <OpenTptInternet.h>
#else
#if defined(__WIN32__) || defined(WIN32)
#define __USE_W32_SOCKETS
#include <windows.h>
#ifdef __CYGWIN__
#include <netinet/in.h>
#endif
#else /* UNIX */
#ifdef __OS2__
#include <types.h>
#include <sys/ioctl.h>
#endif
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#ifndef __BEOS__
#include <arpa/inet.h>
#endif
#ifdef linux /* FIXME: what other platforms have this? */
#include <netinet/tcp.h>
#endif
#include <netdb.h>
#include <sys/socket.h>
#endif /* WIN32 */
#endif /* Open Transport */



#define RTMP_HANDSHAKE_SIZE 1536
#define RTMP_BUFFER_SIZE 4096


typedef struct z_rtmp_server_client_t z_rtmp_server_client_t;

/* 表示客户端
 */
struct z_rtmp_server_client_t
{
    unsigned int conn_sock;
    struct sockaddr_in conn_sockaddr;
    unsigned char received_buffer[RTMP_BUFFER_SIZE];
    size_t received_size;
    unsigned char will_send_buffer[RTMP_BUFFER_SIZE];
    size_t will_send_size;
    size_t amf_chunk_size;
    void *data;
    void (*process_message)(z_rtmp_server_client_t *rsc);
    unsigned char handshake[RTMP_HANDSHAKE_SIZE];
    z_rtmp_server_client_t *prev;
    z_rtmp_server_client_t *next;
};

typedef struct z_rtmp_server_t z_rtmp_server_t;

/* rtmp服务器
 */
struct z_rtmp_server_t
{
    int listen_fd;
    struct sockaddr_in listen_addr;
    /* 很少用
     */
    int stand_by_socket;
    /* 客户端队列
     */
    z_rtmp_server_client_t *client_working;
    /* 空选客户端队列
     */
    z_rtmp_server_client_t *client_pool;
};

/* 创建rtmp服务器
 */
extern z_rtmp_server_t * z_rtmp_server_create(unsigned short port_number);
/* rtmp服务器 消息处理
 */
extern void z_rtmp_server_process_message(z_rtmp_server_t *rs);
extern void z_rtmp_server_free(z_rtmp_server_t *rs);


#define DEFAULT_AMF_CHUNK_SIZE 128


typedef enum z_rtmp_result
{
    RTMP_SUCCESS,
    RTMP_ERROR_UNKNOWN,
    RTMP_ERROR_BUFFER_OVERFLOW,
    RTMP_ERROR_BROKEN_PACKET,
    RTMP_ERROR_DIVIDED_PACKET,
    RTMP_ERROR_MEMORY_ALLOCATION,
    RTMP_ERROR_LACKED_MEMORY,
    RTMP_ERROR_DISCONNECTED,
}z_rtmp_result_t;


typedef struct z_rtmp_event_t z_rtmp_event_t;

struct z_rtmp_event_t
{
    char *code;
    char *level;
    z_rtmp_event_t *next;
};

typedef struct z_rtmp_client_t z_rtmp_client_t;

struct z_rtmp_client_t
{
    int conn_sock;
    unsigned char received_buffer[RTMP_BUFFER_SIZE];
    size_t received_size;
    unsigned char will_send_buffer[RTMP_BUFFER_SIZE];
    size_t will_send_size;
    void (*process_message)(z_rtmp_client_t *client);
    void *data;
    char *url;
    char *protocol;
    char *host;
    int port_number;
    char *path;
    size_t amf_chunk_size;
    unsigned char handshake[RTMP_HANDSHAKE_SIZE];
    long message_number;
    z_rtmp_event_t *events;
};


z_rtmp_client_t *z_rtmp_client_create(const char *url);
extern void z_rtmp_client_free(z_rtmp_client_t *client);

extern z_rtmp_event_t *z_rtmp_client_get_event(z_rtmp_client_t *client);
extern void z_rtmp_client_delete_event(z_rtmp_client_t *client);

extern void z_rtmp_client_connect(
        z_rtmp_client_t *client
        /* FIXME: take URL */);
extern void z_rtmp_client_create_stream(z_rtmp_client_t *client);
extern void z_rtmp_client_play(z_rtmp_client_t *client, const char *file_name);
extern void z_rtmp_server_client_send_server_bandwidth(z_rtmp_server_client_t *rsc);
extern void z_rtmp_server_client_send_client_bandwidth(z_rtmp_server_client_t *rsc);
extern void z_rtmp_server_client_send_ping(z_rtmp_server_client_t *rsc);
extern void z_rtmp_server_client_send_chunk_size(z_rtmp_server_client_t *rsc);
extern void z_rtmp_server_client_send_connect_result(
        z_rtmp_server_client_t *rsc, double number);
extern void z_rtmp_server_client_send_create_stream_result(
        z_rtmp_server_client_t *rsc, double number);
extern void z_rtmp_server_client_send_play_result_error(
        z_rtmp_server_client_t *rsc, double number);
extern void z_rtmp_server_client_send_play_result_success(
        z_rtmp_server_client_t *rsc, double number);

extern void z_rtmp_client_process_message(z_rtmp_client_t *client);

#ifdef __cplusplus
}
#endif

#endif // Z_RTMP_H

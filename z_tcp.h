#ifndef Z_TCP_H
#define Z_TCP_H

#ifdef __cplusplus
extern "C"{
#endif

#define SRV_EACCEPT   1
#define SRV_ESOCKET   2
#define SRV_ENOBLOCK  4
#define SRV_EEVINIT   8
#define SRV_EEVADD    16
#define SRV_ECLOSE    32
#define SRV_ESHUT     64

#define SRV_EVENTRD   1
#define SRV_EVENTWR   2

typedef struct z_tcpserver_t {
    int listen_fd;
    int max_events;
    int backlog;
    int read_buf_size;
    int write_buf_size;
    unsigned int newfd_event_flags;

    int (*handle_read)(struct z_tcpserver_t *, int);
    int (*handle_write)(struct z_tcpserver_t *, int);
    int (*handle_accept)(struct z_tcpserver_t *, int, char *, int);
    int (*handle_hup)(struct z_tcpserver_t *, int);
    int (*handle_rdhup)(struct z_tcpserver_t *, int);
    int (*handle_error)(struct z_tcpserver_t *, int, int);

    /* Pointer to the event_t structure declared in srv_run() */
    void *ev;
} z_tcpserver_t;

int z_tcpserver_init(z_tcpserver_t *);

int z_tcpserver_run(z_tcpserver_t *, char *, char *);
int z_tcp_read(int, char *, int);
int z_tcp_write(int, char *, int);
int z_tcp_readall(int, char *, int);
int z_tcp_writeall(int, char *, int);

int z_tcp_connect(char *, char *);
int z_tcp_closeconn(int);

int z_tcpserver_set_backlog(z_tcpserver_t *, int);
int z_tcpserver_set_maxevents(z_tcpserver_t *, int);

int z_tcpserver_notify_event(z_tcpserver_t *, int, unsigned int);
int z_tcpserver_newfd_notify_event(z_tcpserver_t *, unsigned int);

int z_tcpserver_set_read_handle(z_tcpserver_t *, int (*)(z_tcpserver_t *, int));
int z_tcpserver_set_write_handle(z_tcpserver_t *, int (*)(z_tcpserver_t *, int));
int z_tcpserver_set_accept_handle(z_tcpserver_t *, int (*)(z_tcpserver_t *, int, char *, int));
int z_tcpserver_set_hup_handle(z_tcpserver_t *, int (*)(z_tcpserver_t *, int));
int z_tcpserver_set_rdhup_handle(z_tcpserver_t *, int (*)(z_tcpserver_t *, int));
int z_tcpserver_set_error_handle(z_tcpserver_t *, int (*)(z_tcpserver_t *, int, int));

int z_tcpserver_get_listenerfd(z_tcpserver_t *);

#ifdef __cplusplus
}
#endif

#endif // Z_TCP_H

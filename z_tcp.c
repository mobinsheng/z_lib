#include "z_tcp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>

#ifdef __linux__
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#ifdef _MSC_VER
#define inline __inline
#else
#define inline __inline__
#endif

#ifdef _WIN32

#include <Winsock2.h>
#include <Ws2tcpip.h>

#define close closesocket
#define SHUT_RDWR SD_BOTH

static inline int read(int fd, char *buffer, int size) {
    return recv(fd, buffer, size, 0);
}

static inline int write(int fd, char *buffer, int size) {
    return send(fd, buffer, size, 0);
}
#endif

/* Detect the best event notification mechanism available */
#ifdef __linux__
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,44)
#define EPOLL
#else
/* TODO: Check if poll, kqueue or IOCP is avalable.
If not, fallback to select */
#define SELECT
#endif
#else
#ifdef _WIN32
/* TODO: Check if IOCP exists. Otherwise, fallback to select */
#define SELECT
#endif
#endif

#ifdef EPOLL
#include <sys/epoll.h>
#endif

#ifdef SELECT
#ifdef _WIN32
#define FD_SETSIZE 10000 /* TODO: Find the max # of open sockets instead of a hardcoded value */
#else
#define FD_SETSIZE FOPEN_MAX
#endif
#ifdef __linux__
#include <sys/select.h>
#endif
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

#ifdef __GNUC__
#define likely(x) __builtin_expect((x),1)
#define unlikely(x) __builtin_expect((x),0)
#else
#define likely(x) x
#define unlikely(x) x
#endif

#ifndef SOCK_NONBLOCK
#define SOCK_NONBLOCK 1
#endif

/* 设置socket为非阻塞模式
 * @fd             套接字
 * @return      操作结果（fcntl的返回结果）
 */
static int setnoblock(int fd) {
#ifndef _WIN32
    int flags;
    flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;

    return fcntl(fd, F_SETFL, flags);
#else
    u_long argp = 1;
    return ioctlsocket(fd, FIONBIO, &argp);
#endif
}

/* 创建监听socket
 * @ctx                 服务器对象
 * @hostname        主机名，如果是在本机器上监听，可以把hostname设置为NULL
 * @post                端口
 * @return          监听套接字，失败则返回-1
 */
static int tcp_create_listener(z_tcpserver_t *ctx, char *hostname, char *port) {
    int status, fd, reuse_addr;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if(!hostname)
        hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(hostname, port, &hints, &servinfo);
    if(status) {
        /* TODO: Check the error code and set errno appropriately */
        return -1;
    }

    fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(fd == -1)
        return -1;

    /* Make the socket available for reuse immediately after it's closed */
    reuse_addr = 1;
#ifdef _WIN32
    /* NOTE: It turns out Win32 definition of SO_REUSEADDR is not the same as the POSIX definition.
       Instead we use SO_EXCLUSIVEADDRUSE */
    status = setsockopt(fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char *)&reuse_addr, sizeof(reuse_addr));
#else
    status = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
#endif
    if(status == -1)
        return -1;

    /* Bind the socket to the address */
    status = bind(fd, servinfo->ai_addr, servinfo->ai_addrlen);
    if(status == -1)
        return -1;

    /* Listen for incoming connections */
    status = listen(fd, ctx->backlog);
    if(status == -1)
        return -1;

    return fd;
}

/* 用于客户端连接服务器
 * @hostname    服务器主机名字
 * @port            服务器端口
 * @return          客户端套接字
 */
int z_tcp_connect(char *hostname, char *port) {
    int status, fd;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(hostname, port, &hints, &servinfo);
    if(status) {
        /* TODO: Check the error code and set errno appropriately */
        return -1;
    }

    fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(fd == -1)
        return -1;

    status = connect(fd, servinfo->ai_addr, servinfo->ai_addrlen);
    if(status == -1)
        return -1;

    freeaddrinfo(servinfo);

    return fd;
}

/* 关闭连接，一般是客户端使用
 * @fd      套接字
 * @return  close的返回值
 */
int z_tcp_closeconn(int fd) {
    /* TODO: Remove from the event mechanism */
    return close(fd);
}

/* 接受一个客户端的连接，其实客户端早已连接，并在内核就绪了
 * @fd          监听套接字
 * @ip          存放客户端ip的buffer，可以为NULL，表示不需要知道客户端的ip
 * @port        存放客户端的端口
 * @flag        标志位，例如SOCK_NONBLOCK之类的
 * @retuen      客户端套接字
 */
static int tcp_accept(int fd, char *ip, int *port, int flags) {
    int fd_new;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);

#ifdef linux
    fd_new = accept4(fd, (struct sockaddr *) &addr, &addrlen, flags);
#else
    /* Fallback to accept if accept4 is not implemented */
    fd_new = accept(fd, (struct sockaddr *) &addr, &addrlen);
#endif

    if(fd_new == -1) {
        return -1;
    }
    else {
#ifndef linux
        /* accept() doesn't accept flags so we need to set non-blocking mode manually */
        if(flags == SOCK_NONBLOCK)
            setnoblock(fd_new);
#endif
        /* Fill in address info buffers */
        if(addr.ss_family == AF_INET) {
            /* IPv4 */
            struct sockaddr_in *s = (struct sockaddr_in *) &addr;

            if(ip) inet_ntop(AF_INET, &s->sin_addr, ip, INET6_ADDRSTRLEN);
            if(port) *port = ntohs(s->sin_port);
        }
        else {
            /* IPv6 */
            struct sockaddr_in6 *s = (struct sockaddr_in6 *) &addr;

            if(ip) inet_ntop(AF_INET6, &s->sin6_addr, ip, INET6_ADDRSTRLEN);
            if(port) *port = ntohs(s->sin6_port);
        }

        return fd_new;
    }
}

/* 读取套接字对应的内核缓冲区的数据
 * @fd          套接字
 * @buf         数据存放的buffer
 * @size        buffer的长度
 * @return      读取的数据长度
 */
int z_tcp_read(int fd, char *buf, int size) {
    /* TODO: WSAGetLastError */
    return read(fd, buf, size);
}

/* 把套接字对应的内核缓冲区的数据全部读取出来
 * @fd              套接字
 * @buf             数据缓冲区
 * @size            缓冲区长度
 * @return          读取的数据长度
 */
int z_tcp_readall(int fd, char *buf, int size) {
    /* TODO: Handle EINTR */
    int nread, total_read = 0;

    /* Make sure 'size' bytes are read */
    while(total_read != size) {
        nread = read(fd, buf, size - total_read);

        if(nread <= 0)
            return total_read;

        total_read += nread;
        buf += nread;
    }
    return total_read;
}

/* 写数据到套接字的内核缓冲区中
 * @fd          套接字
 * @buf         数据
 * @size        数据长度
 * @return      写入的数据长度
 */
int z_tcp_write(int fd, char *buf, int size) {
    /* TODO: WSAGetLastError */
    return write(fd, buf, size);
}

/* 把所有数据写入套接字的内核缓冲区中
 * @fd          套接字
 * @buf         数据
 * @size        数据长度
 * @return      写入的数据长度
 */
int z_tcp_writeall(int fd, char *buf, int size) {
    /* TODO: Handle EINTR */
    int nwritten, total_written = 0;

    /* Make sure 'size' bytes are written */
    while(total_written != size) {
        nwritten = write(fd, buf, size - total_written);

        switch(nwritten) {
        case 0: return total_written; break;
        case -1: return -1; break;
        }

        total_written += nwritten;
        buf += nwritten;
    }
    return total_written;
}

#ifdef EPOLL

/* 读就绪 */
#define EVENTRD    EPOLLIN
/* 写完成 */
#define EVENTWR    EPOLLOUT
/* 客户端意外断开连接（断电）*/
#define EVENTHUP   EPOLLHUP
/* 客户端断开连接 */
#define EVENTRDHUP EPOLLRDHUP
/* 错误 */
#define EVENTERR   EPOLLERR

/* 反应器-reactor
 * 表示对发生的事件产生反应
 */
typedef struct {
    /*
     * 它存放epoll已经发生的事件
     */
    struct epoll_event *events;
    int epoll_fd;
    int fd_count;
    int fd_index;
    int max_events;
} z_reactor_t;

/* 初始化epoll对象，创建epoll套接字
 */
static inline int z_reactor_init(z_reactor_t *reactor, int max_events) {
    reactor->epoll_fd = epoll_create1(0); // 创建epoll套接字
    reactor->fd_count = 0;
    reactor->fd_index = 0;
    reactor->max_events = max_events;

    reactor->events = (struct epoll_event*)calloc(max_events, sizeof(struct epoll_event));
    if(reactor->events == NULL)
        return -1;

    return reactor->epoll_fd;
}

static inline int z_reactor_add_fd(z_reactor_t *reactor, int fd, uint32_t flags) {
    struct epoll_event tmp_event;

    tmp_event.data.fd = fd;
    tmp_event.events = flags;

    return epoll_ctl(reactor->epoll_fd, EPOLL_CTL_ADD, fd, &tmp_event);
}

static inline int z_reactor_mod_fd(z_reactor_t *reactor, int fd, uint32_t flags) {
    struct epoll_event tmp_event;
    tmp_event.data.fd = fd;
    tmp_event.events = flags;

    return epoll_ctl(reactor->epoll_fd, EPOLL_CTL_MOD, fd, &tmp_event);
}

static inline int z_reactor_remove_fd(z_reactor_t *reactor, int fd) {
    struct epoll_event tmp_event; /* Required for linux versions before 2.6.9 */

    return epoll_ctl(reactor->epoll_fd, EPOLL_CTL_DEL, fd, &tmp_event);
}

static inline int z_reactor_wait(z_reactor_t *reactor, int *event_fd, int *event_type) {
    if(reactor->fd_count == 0) {
        /* All events processed so far. Wait for new events */
        reactor->fd_index = 0;
        reactor->fd_count = epoll_wait(reactor->epoll_fd, reactor->events, reactor->max_events, -1);
    }

    /* Preserve the errno and notify the caller that an error has occured */
    if(reactor->fd_count <= 0) {
        /* Error occured or there are no events waiting to be handled */
        *event_type = 0;
        return reactor->fd_count; /* Return value is -1 on error */
    }

    /* Pass the next event to the caller */
    *event_type = reactor->events[reactor->fd_index].events;
    *event_fd = reactor->events[reactor->fd_index].data.fd;

    /* Point to the next event to be handled */
    if(reactor->fd_index < reactor->fd_count)
        reactor->fd_index++;
    else {
        reactor->fd_count = 0;
        *event_type = 0;
        return 0; /* All events have been handled */
    }

    /* Return the number of events waiting to be handled */
    return (reactor->fd_count - reactor->fd_index);
}

static inline int z_reactor_free(z_reactor_t *reactor) {
    free(reactor->events);

    if(close(reactor->epoll_fd) == -1)
        return -1;
}
#endif

#ifdef SELECT

#define EVENTRD     1
#define EVENTWR     2
#define EVENTHUP    4
#define EVENTRDHUP  8
#define EVENTERR   16

typedef struct {
    fd_set fds_read_master, fds_read, fds_write_master, fds_write;
    int fdmax, fd_count, fd_index;
} z_reactor_t;

static inline int z_reactor_init(z_reactor_t *ev, int max_events) {
    /* Initialize the fd sets */
    FD_ZERO(&(ev->fds_read_master));
    FD_ZERO(&(ev->fds_read));

    FD_ZERO(&(ev->fds_write_master));
    FD_ZERO(&(ev->fds_write));

    ev->fdmax = 0;
    ev->fd_count = 0;
    ev->fd_index = 0;

    return 0;
}

static inline int z_reactor_add_fd(z_reactor_t *ev, int fd, uint32_t flags) {
    if(fd >= FD_SETSIZE) {
        errno = ENOSPC; /* fd set is full */
        return -1;
    }

    if(flags & EVENTRD) {
        /* Add the fd to the read fd_set */
        FD_SET(fd, &(ev->fds_read_master));
    }

    if(flags & EVENTWR) {
        /* Add the fd to the write fd_set */
        FD_SET(fd, &(ev->fds_write_master));
    }

    /* Update fdmax */
    if(fd > ev->fdmax)
        ev->fdmax = fd;

    return 0;
}

static inline int z_reactor_mod_fd(z_reactor_t *ev, int fd, uint32_t flags) {
    if(flags & EVENTRD) {
        /* Add the fd to the read fd_set */
        FD_SET(fd, &(ev->fds_read_master));
    }
    else {
        /* Remove the fd from the read fd_set */
        FD_CLR(fd, &(ev->fds_read_master));
    }

    if(flags & EVENTWR) {
        /* Add the fd to the write fd_set */
        FD_SET(fd, &(ev->fds_write_master));
    }
    else {
        /* Remove the fd from the write fd_set */
        FD_CLR(fd, &(ev->fds_write_master));
    }

    return 0;
}

static inline int z_reactor_remove_fd(z_reactor_t *ev, int fd) {
    /* Remove from all fd sets */
    FD_CLR(fd, &(ev->fds_read_master));
    FD_CLR(fd, &(ev->fds_write_master));
    FD_CLR(fd, &(ev->fds_read));
    FD_CLR(fd, &(ev->fds_write));

    /* Update fdmax */
    if(fd == ev->fdmax) {
        ev->fdmax--;

        /* The fd that is being handled has just been removed. All the previous
fds have already been handled */
        if(fd == ev->fd_index)
            ev->fd_count = 0;
    }

    return 0;
}

static inline int z_reactor_wait(z_reactor_t *ev, int *event_fd, int *event_type) {
    if(ev->fd_count == 0) {
        /* All events processed so far. Wait for new events */
        ev->fds_read = ev->fds_read_master;
        ev->fds_write = ev->fds_write_master;
        ev->fd_index = 0;

        ev->fd_count = select(ev->fdmax + 1, &(ev->fds_read),
                              &(ev->fds_write), NULL, NULL);
    }

    if(ev->fd_count == -1) {
        /* An error occured */
        *event_type = EVENTERR;
        ev->fd_count = 0;

        return -1;
    }
    else if(ev->fd_count == 0) {
        /* No events waiting to be processed */
        *event_type = 0;

        return 0;
    }

    /* Find the next ready fd */
    for(;(ev->fd_index <= ev->fdmax) && (ev->fd_count > 0); ev->fd_index++) {
        *event_type = 0;
        if(FD_ISSET(ev->fd_index, &(ev->fds_read))) {
            /* fd ready for read */
            *event_fd = ev->fd_index;
            *event_type |= EVENTRD;
            ev->fd_count--;
            FD_CLR(ev->fd_index, &(ev->fds_read));
        }

        if(FD_ISSET(ev->fd_index, &(ev->fds_write))) {
            /* fd ready for write */
            *event_fd = ev->fd_index;
            *event_type |= EVENTWR;
            ev->fd_count--;
        }

        if(*event_type) {
            /* TODO: Notify the caller about the event */
            break;
        }
    }

    return ev->fd_count;
}

static inline int z_reactor_free(z_reactor_t *ev) {
    /* Nothing to free */
    return 0;
}

#endif

/* tcp server 初始化
 */
int z_tcpserver_init(z_tcpserver_t *ctx) {
    if(!ctx) {
        errno = EINVAL;
        return -1;
    }

    /* Set default values for the options */
    ctx->backlog = 1;
    ctx->max_events = 1000; /* Good enough? */
    ctx->read_buf_size  = 512;
    ctx->write_buf_size = 512;

    /* Initialize handler pointers to 0 */
    ctx->handle_accept = 0;
    ctx->handle_read   = 0;
    ctx->handle_write  = 0;
    ctx->handle_hup    = 0;
    ctx->handle_rdhup  = 0;
    ctx->handle_error  = 0;

    /* By default, only read events are reported for new fds */
    ctx->newfd_event_flags = EVENTRD; // 读就绪

    return 0;
}

/* TODO: WSACleanup on error */
int z_tcpserver_run(z_tcpserver_t *ctx, char *hostname, char *port) {
    z_reactor_t reactor;

    int event_fd, cli_fd, event_type;

    int  cli_port;
    char cli_addr[INET6_ADDRSTRLEN];

#ifdef _WIN32
    WSADATA wsaData;
#endif

    if(!ctx) {
        errno = EINVAL;
        return -1;
    }

    /* Pointer to th z_reactor_t structure. Needed for srv_notify_event()
           and srv_newfd_notify_event() */
    ctx->ev = (void *) &reactor;

#ifdef _WIN32
    if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        /* TODO: Set errno */
        return -1;
    }
#endif

    /* We must have a read handler */
    if(ctx->handle_read == NULL) {
        errno = EINVAL; /* Invalid argument */
        return -1;
    }

    /* Create a listener socket */
    if((ctx->listen_fd = tcp_create_listener(ctx, hostname, port)) == -1)
        return -1;

    /* The listener must not block */
    if(setnoblock(ctx->listen_fd) == -1)
        return -1;

    /* Initialize the event notification mechanism */
    if(z_reactor_init(&reactor, ctx->max_events) == -1)
        return -1;

    /* Request read event notifications for the listener */
    if(z_reactor_add_fd(&reactor, ctx->listen_fd, EVENTRD) == -1)
        return -1;

    /* Event loop */
    while(1) {
        if(z_reactor_wait(&reactor, &event_fd, &event_type) == -1) {
            /* TODO: Handle EINTR */
            return -1;
        }

        /* Handle the event */
        if (!event_type) {
            /* No events. We should never get here in the first place */
            continue;
        }
        /* 如果出错 */
        if (event_type & EVENTERR) {
            /* An error has occured */

            /* Notify the caller */
            if(ctx->handle_error)
                (*(ctx->handle_error))(ctx, event_fd, 0); /* TODO: Return the proper error no */

            z_reactor_remove_fd(&reactor, event_fd);
            close(event_fd);
        }
        /* 客户端意外的关闭（例如断电） */
        if (event_type & EVENTHUP) {
            /* The connection has been shutdown unexpectedly */

            /* Notify the caller */
            if(ctx->handle_hup)
                (*(ctx->handle_hup))(ctx, event_fd);

            z_reactor_remove_fd(&reactor, event_fd);
            close(event_fd);
        }
        /* 客户端断开连接 */
        if (event_type & EVENTRDHUP) {
            /* The client has closed the connection */

            /* Notify the caller */
            if(ctx->handle_rdhup)
                (*(ctx->handle_rdhup))(ctx, event_fd);

            z_reactor_remove_fd(&reactor, event_fd);
            close(event_fd);
        }
        /* 读事件 */
        if(event_type & EVENTRD) {
            if(event_fd == ctx->listen_fd) {
                /* Incoming connection */
                while(1) {
                    /* Accept the connection */
                    /* TODO: Notify the caller and request permission to accept, maybe? */
                    cli_fd = tcp_accept(ctx->listen_fd, (char *)&cli_addr,
                                        (int *)&cli_port, SOCK_NONBLOCK);

                    if(cli_fd == -1) {
#ifdef _WIN32
                        if(WSAGetLastError() == WSAEWOULDBLOCK) {
#else
                        if(likely((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
#endif
                            /* We've processed all incoming connections */
                            break;
                        }
                        else {
                            /* accept returned error */
                            if(ctx->handle_error)
                                ((*ctx->handle_error))(ctx, cli_fd, SRV_EACCEPT);
                            break;
                        }
                    }

                    /* Add the new fd to the event list */
                    z_reactor_add_fd(&reactor, cli_fd, ctx->newfd_event_flags); /* TODO: Error handling */

                    /* Accepted connection. Call the accept handler */
                    if(ctx->handle_accept != NULL) {
                        (*(ctx->handle_accept))(ctx, cli_fd, cli_addr, cli_port);
                    }
                }
            }
            else {
                /* Data available for read */
                if((*(ctx->handle_read))(ctx, event_fd)) {
                    /* Handler requested the connection to be closed. */

                    /* Remove the fd from the event list */
                    /* TODO: Removal might be expensive on some event notification
                           mechanisms. Don't remove the fd if keeping it will be less expensive. */
                    z_reactor_remove_fd(&reactor, event_fd);
                    close(event_fd);
                }
            }
        }

        /* 写完成事件 */
        if(event_type & EVENTWR) {
            /* Socket ready for write */
            if(ctx->handle_write) {
                if((*(ctx->handle_write))(ctx, event_fd)) {
                    /* Handler requested the connection to be closed */

                    /* Remove the fd from the event list */
                    /* TODO: Removal might be expensive on some event notification
                           mechanisms. Don't remove the fd if keeping it will be less expensive. */
                    z_reactor_remove_fd(&reactor, event_fd);
                    close(event_fd);
                }
            }
        }
    }

    /* Close the listener socket */
    if(shutdown(ctx->listen_fd, SHUT_RDWR) == -1)
        return -1;

    if(close(ctx->listen_fd) == -1)
        return -1;

    /* Deinitialize the event mechanism */
    if(z_reactor_free(&reactor) == -1)
        return -1;

#ifdef _WIN32
    WSACleanup();
#endif
    return 0; /* Terminated succesfully */
}

int z_tcpserver_set_read_handle(z_tcpserver_t *ctx, int (*h)(z_tcpserver_t *, int)) {
    if(!ctx) {
        errno = EINVAL;
        return -1;
    }

    ctx->handle_read = h;
    return 0;
}

int z_tcpserver_set_write_handle(z_tcpserver_t *ctx, int (*h)(z_tcpserver_t *, int)) {
    if(!ctx) {
        errno = EINVAL;
        return -1;
    }

    ctx->handle_write = h;
    return 0;
}

int z_tcpserver_set_accept_handle(z_tcpserver_t *ctx, int (*h)(z_tcpserver_t *, int, char *, int)) {
    if(!ctx) {
        errno = EINVAL;
        return -1;
    }

    ctx->handle_accept = h;
    return 0;
}

int z_tcpserver_set_hup_handle(z_tcpserver_t *ctx, int (*h)(z_tcpserver_t *, int)) {
    if(!ctx) {
        errno = EINVAL;
        return -1;
    }

    ctx->handle_hup = h;
    return 0;
}

int z_tcpserver_set_rdhup_handle(z_tcpserver_t *ctx, int (*h)(z_tcpserver_t *, int)) {
    if(!ctx) {
        errno = EINVAL;
        return -1;
    }

    ctx->handle_rdhup = h;
    return 0;
}

int z_tcpserver_set_error_handle(z_tcpserver_t *ctx, int (*h)(z_tcpserver_t *, int, int)) {
    if(!ctx) {
        errno = EINVAL;
        return -1;
    }

    ctx->handle_error = h;
    return 0;
}

int z_tcpserver_set_backlog(z_tcpserver_t *ctx, int backlog) {
    if(!ctx) {
        errno = EINVAL;
        return -1;
    }

    ctx->backlog = backlog;
    return 0;
}

int z_tcpserver_set_max_events(z_tcpserver_t *ctx, int n) {
    if(!ctx) {
        errno = EINVAL;
        return -1;
    }

    ctx->max_events = n;
    return 0;
}

int z_tcpserver_notify_event(z_tcpserver_t *ctx, int fd, unsigned int flags) {
    uint32_t f;

    if(!ctx) {
        errno = EINVAL;
        return -1;
    }

    f = 0;
    if(flags & SRV_EVENTRD)
        f |= EVENTRD;
    if(flags & SRV_EVENTWR)
        f |= EVENTWR;

    return z_reactor_mod_fd((z_reactor_t *) ctx->ev, fd, f);
}

int z_tcpserver_newfd_notify_event(z_tcpserver_t *ctx, unsigned int flags) {
    uint32_t f;

    if(!ctx) {
        errno = EINVAL;
        return -1;
    }

    f = 0;
    if(flags & SRV_EVENTRD)
        f |= EVENTRD;
    if(flags & SRV_EVENTWR)
        f |= EVENTWR;

    ctx->newfd_event_flags = f;
    return 0;
}

int z_tcpserver_get_listenerfd(z_tcpserver_t *ctx) {
    if(!ctx) {
        errno = EINVAL;
        return -1;
    }

    return ctx->listen_fd;
}

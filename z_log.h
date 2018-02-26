#ifndef Z_LOG_H
#define Z_LOG_H

#ifdef __cplusplus
extern "C"{
#endif

typedef  enum z_log_level_t {
    Z_LOG_DEBUG = 1,
    Z_LOG_TRACE = 2,
    Z_LOG_NOTICE = 3,
    Z_LOG_WARNING = 4,
    Z_LOG_ERROR = 5,
}z_log_level_t;

typedef struct z_log_t z_log_t;

z_log_t* z_log_init(const char* p_logdir,const char* p_logfilename,z_log_level_t l );

void z_log_destroy(z_log_t* writer);

void z_log_write(z_log_t* writer,z_log_level_t l, int is_freq_record,char* logformat,...);

#define LOG_ERROR(writer,log_fmt, log_arg...) \
    do{ \
        z_log_write(writer,Z_LOG_ERROR,   0, "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##log_arg); \
    } while (0)

#define LOG_WARN(writer,log_fmt, log_arg...) \
    do{ \
        z_log_write(writer,Z_LOG_WARNING,  0,  "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##log_arg); \
    } while (0)

#define LOG_NOTICE(writer,log_fmt, log_arg...) \
    do{ \
        z_log_write(writer,Z_LOG_NOTICE,   0, "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##log_arg); \
    } while (0)

#define LOG_TRACE(writer,log_fmt, log_arg...) \
    do{ \
        z_log_write(writer,Z_LOG_TRACE,  0,  "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##log_arg); \
    } while (0)

#define LOG_DEBUG(writer,log_fmt, log_arg...) \
    do{ \
        z_log_write(writer,Z_LOG_DEBUG,  0, "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##log_arg); \
    } while (0)

//===============================
#define LOG_ERROR_FILTER(writer,log_fmt, log_arg...) \
    do{ \
        z_log_write(writer,Z_LOG_ERROR,  1, "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##log_arg); \
    } while (0)

#define LOG_WARN_FILTER(writer,log_fmt, log_arg...) \
    do{ \
        z_log_write(writer,Z_LOG_WARNING, 1,  "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##log_arg); \
    } while (0)

#define LOG_NOTICE_FILTER(writer,log_fmt, log_arg...) \
    do{ \
        z_log_write(writer,Z_LOG_NOTICE,1,   "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##log_arg); \
    } while (0)

#define LOG_TRACE_FILTER(writer,log_fmt, log_arg...) \
    do{ \
        z_log_write(writer,Z_LOG_TRACE, 1,  "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##log_arg); \
    } while (0)

#define LOG_DEBUG_FILTER(writer,log_fmt, log_arg...) \
    do{ \
        z_log_write(writer,Z_LOG_DEBUG, 1,  "[%s:%d][%s] " log_fmt "\n", \
                     __FILE__, __LINE__, __FUNCTION__, ##log_arg); \
    } while (0)


#ifdef __cplusplus
}
#endif

#endif // Z_LOG_H

#include "z_log.h"

#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef WIN32
#include <io.h>
#include <direct.h>
#else
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<unistd.h>
#endif

#define __USE_GNU

#if defined(__USE_BSD) || defined(__USE_GNU)
#define S_IREAD S_IRUSR
#define S_IWRITE S_IWUSR
#define S_IEXEC S_IXUSR
#endif


static int wrap_access(const char* name){
#ifdef WIN32
    return _access(name,0);
#else
    return access(name,W_OK);
#endif
}

static int wrap_mkdir(const char* name){
#ifdef WIN32
    return _mkdir (name);
#else
    return mkdir (name, S_IREAD | S_IWRITE);
#endif
}

#define   _LOG_BUFFSIZE  1024*1024*4
#define   _SYS_BUFFSIZE  1024*1024*8
#define	  _LOG_PATH_LEN  250
#define   _LOG_MODULE_LEN 32

#define _RECORD_FREQ 50


typedef enum{
    z_false = 0,
    z_true = 1,
}z_bool;

struct z_log_t{
    z_log_level_t level;
    FILE* fp_log;
    int is_sync;
    int is_append;
    char logfile_location[_LOG_PATH_LEN];
    char log_buffer[_LOG_BUFFSIZE];

    unsigned int record_count; // 为了避免应用程序的大量错误写入日志中，提供了按照频率写入的功能
   unsigned int record_frequency;

    pthread_mutex_t mutex;
};

const char* level2string(z_log_level_t l) ;
int make_log_prefix(char* st_logBuffer, z_log_level_t l);
z_log_t* writer_alloc();
void writer_free(z_log_t* writer);
z_bool check_level(z_log_t* writer,z_log_level_t l);
z_bool logfile_init(z_log_t* writer,z_log_level_t l, const  char *filelocation, z_bool append, z_bool issync);
z_bool log_write(z_log_t* writer,char *_pbuffer, int len);
z_log_level_t get_level(z_log_t* writer);
z_bool logfile_close(z_log_t* writer);
//============extend===================
#define MACRO_RET(condition, return_val) {\
    if (condition) {\
    return return_val;\
    }\
    }

#define MACRO_WARN(condition, log_fmt, log_arg...) {\
    if (condition) {\
    LOG_WARN( log_fmt, ##log_arg);\
    }\
    }

#define MACRO_WARN_RET(condition, return_val, log_fmt, log_arg...) {\
    if ((condition)) {\
    LOG_WARN( log_fmt, ##log_arg);\
    return return_val;\
    }\
    }

//=============================================================
z_log_t* writer_alloc(){
    z_log_t* writer = (z_log_t*)malloc(sizeof(z_log_t));
    memset(writer,0,sizeof(z_log_t));
    writer->level = Z_LOG_NOTICE;
    writer->fp_log = NULL;
    writer->is_sync = 0;
    writer->is_append = 1;
    writer->logfile_location[0] ='\0';

    writer->record_frequency = _RECORD_FREQ;

    pthread_mutex_init(&writer->mutex, NULL);
    return writer;
}

void writer_free(z_log_t* writer){
    logfile_close(writer);
    free(writer);
}

const char* level2string(z_log_level_t l) {
    switch ( l ) {
    case Z_LOG_DEBUG:
        return "DEBUG";
    case Z_LOG_TRACE:
        return "TRACE";
    case Z_LOG_NOTICE:
        return "NOTICE";
    case Z_LOG_WARNING:
        return "WARN" ;
    case Z_LOG_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

z_bool check_level(z_log_t* writer,z_log_level_t l)
{
    if(l >= writer->level)
        return z_true;
    else
        return z_false;
}

z_bool logfile_init(z_log_t* writer,z_log_level_t l, const  char *filelocation, z_bool append, z_bool issync)
{
    MACRO_RET(NULL != writer->fp_log, z_false);
    writer->level = l;
    writer->is_append = append;
    writer->is_sync = issync;
    if(strlen(filelocation) >= (sizeof(writer->logfile_location) -1))
    {
        fprintf(stderr, "the path of log file is too long:%d limit:%d\n", strlen(filelocation), sizeof(writer->logfile_location) -1);
        exit(0);
    }
    //本地存储filelocation  以防止在栈上的非法调用调用
    strncpy(writer->logfile_location, filelocation, sizeof(writer->logfile_location));
    writer->logfile_location[sizeof(writer->logfile_location) -1] = '\0';

    if('\0' == writer->logfile_location[0])
    {
        writer->fp_log = stdout;
        fprintf(stderr, "now all the running-information are going to put to stderr\n");
        return z_true;
    }

    writer->fp_log = fopen(writer->logfile_location, append ? "a":"w");
    if(writer->fp_log == NULL)
    {
        fprintf(stderr, "cannot open log file,file location is %s\n", writer->logfile_location);
        exit(0);
    }
    //setvbuf (fp, io_cached_buf, _IOLBF, sizeof(io_cached_buf)); //buf set _IONBF  _IOLBF  _IOFBF
    setvbuf (writer->fp_log,  (char *)NULL, _IOLBF, 0);
    fprintf(stderr, "now all the running-information are going to the file %s\n", writer->logfile_location);
    return z_true;
}

int make_log_prefix(char* st_logBuffer, z_log_level_t l)
{
    time_t now;
    now = time(&now);;
    struct tm vtm;
    localtime_r(&now, &vtm);
    return snprintf(st_logBuffer, _LOG_BUFFSIZE, "%s: %02d-%02d %02d:%02d:%02d ", level2string(l),
                    vtm.tm_mon + 1, vtm.tm_mday, vtm.tm_hour, vtm.tm_min, vtm.tm_sec);
}

z_bool log_write(z_log_t* writer,char *_pbuffer, int len)
{
    if(0 != wrap_access(writer->logfile_location))
    {
        //锁内校验 access 看是否在等待锁过程中被其他线程logfile_init了  避免多线程多次close 和init
        if(0 != wrap_access(writer->logfile_location))
        {
            logfile_close(writer);
            logfile_init(writer,writer->level, writer->logfile_location, (z_bool)writer->is_append, (z_bool)writer->is_sync);
        }
    }

    if(1 == fwrite(_pbuffer, len, 1, writer->fp_log)) //only write 1 item
    {
        if(writer->is_sync)
            fflush(writer->fp_log);
        *_pbuffer='\0';
    }
    else
    {
        int x = errno;
        fprintf(stderr, "Failed to write to logfile. errno:%s    message:%s", strerror(x), _pbuffer);
        return z_false;
    }

    return z_true;
}

z_log_level_t get_level(z_log_t* writer)
{
    return writer->level;
}

z_bool logfile_close(z_log_t* writer)
{
    if(writer->fp_log == NULL)
        return z_false;
    fflush(writer->fp_log);
    fclose(writer->fp_log);
    writer->fp_log = NULL;
    return z_true;
}

//============================================
z_log_t* z_log_init(const char* p_logdir,const char* p_logfilename,z_log_level_t level )
{
    z_log_t* writer = writer_alloc();

    //如果路径存在文件夹，则判断是否存在
    if (access (p_logdir, 0) == -1){
        if (wrap_mkdir (p_logdir ) < 0)
            fprintf(stderr, "create folder failed\n");
    }

    char _location_str[_LOG_PATH_LEN];
    snprintf(_location_str, _LOG_PATH_LEN, "%s/%s.log", p_logdir, p_logfilename);
    logfile_init(writer,level, _location_str,z_true,z_false);
    return writer;
}

void z_log_write(z_log_t* writer,z_log_level_t l, int is_freq_record,char* logformat,...)
{
    MACRO_RET(!check_level(writer,l), );
    int _size;
    int prestrlen = 0;

    pthread_mutex_lock(&writer->mutex);

    int need_write = 0;

    if(is_freq_record == 0){
        need_write = 1;
    }
    else{
        ++writer->record_count;
        if(writer->record_count == writer->record_frequency){
            need_write = 1;
            writer->record_count = 0;
        }
    }

    if(need_write){
        char * star = writer->log_buffer;
        prestrlen = make_log_prefix(star, l);
        star += prestrlen;

        va_list args;
        va_start(args, logformat);
        _size = vsnprintf(star, _LOG_BUFFSIZE - prestrlen, logformat, args);
        va_end(args);

        if(NULL == writer->fp_log)
            fprintf(stderr, "%s", writer->log_buffer);
        else
            log_write(writer,writer->log_buffer, prestrlen + _size);
    }
    pthread_mutex_unlock(&writer->mutex);
}

void z_log_destroy(z_log_t* writer){
    writer_free(writer);
}

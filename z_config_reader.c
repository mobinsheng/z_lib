#include "z_config_reader.h"

#define KEY_DEFAULT_SIZE (1024*2)
#define VALUE_DEFAULT_SIZE (1024*4)

#ifndef WIN32
     #define _atoi64(val)     strtoll(val, NULL, 10)
#endif

typedef struct kv_t{
    char* key;
    int key_len;
    char* value;
    int value_len;
}kv_t;

struct z_config_reader_t{
    FILE* fp_cfg;
    kv_t** kv_list;
    size_t kv_list_capacity;
    size_t kv_list_size;
};

static kv_t* alloc_kv(){
    kv_t* kv = malloc(sizeof(kv_t));
    if(kv == NULL){
        return NULL;
    }
    memset(kv,0,sizeof(kv_t));

    kv->key = malloc(KEY_DEFAULT_SIZE);
    if(kv->key == NULL){
        free(kv);
        return NULL;
    }
    kv->value = malloc(VALUE_DEFAULT_SIZE);
    if(kv->value == NULL){
        free(kv->key);
        free(kv);
        return NULL;
    }
    return kv;
}

static void free_kv(kv_t* kv){
    if(kv == NULL){
        return;
    }

    if(kv->key){
        free(kv->key);
        kv->key = NULL;
    }

    if(kv->value){
        free(kv->value);
        kv->value = NULL;
    }

    free(kv);

    return;
}

static void trim(char *strIn, char *strOut);
static int findChar(const char* str, char ch);
static int findCharReverse(const char* str, char ch);

static int z_config_reader_read(z_config_reader_t* cfg,kv_t* kv);
static void z_config_reader_readall(z_config_reader_t* cfg);

///========================
static void trim(char *strIn, char *strOut){
    int inLen = strlen(strIn);
    int i, j ;
    i = 0;
    j = inLen - 1;

    while(i < inLen && isspace(strIn[i]))
        ++i;
    while(j >=0 && isspace(strIn[j]))
        --j;
    memmove(strOut, strIn + i , j - i + 1);//strncpy
    strOut[j - i + 1] = '\0';
}

static int findChar(const char* str, char ch){
    if(str == NULL){
        return -1;
    }

    int len = strlen(str);
    int index = -1;
    for(int i = 0; i < len; ++i){
        if(str[i] == ch){
            index = i;
            break;
        }
    }

    return index;
}

static int findCharReverse(const char* str, char ch){
    if(str == NULL){
        return -1;
    }
    // "qwer"
    int len = strlen(str);
    int index = -1;
    for(int i = len - 1; i >= 0; --i){
        if(str[i] == ch){
            index = i;
            break;
        }
    }

    return index;
}
///========================
z_config_reader_t* z_config_reader_create(const char* fileName){
    if(fileName == NULL || strlen(fileName) == 0){
        return NULL;
    }
    z_config_reader_t* cfg = (z_config_reader_t*)malloc(sizeof(z_config_reader_t));
    if(cfg == NULL){
        return NULL;
    }
    memset(cfg,0,sizeof(z_config_reader_t));

    cfg->fp_cfg = fopen(fileName,"rb");
    if(cfg->fp_cfg == NULL){
        free(cfg);
        return NULL;
    }

    cfg->kv_list_capacity = 16;
    cfg->kv_list_size = 0;

    cfg->kv_list = (kv_t**)malloc(cfg->kv_list_capacity * sizeof(kv_t*));

    if(cfg->kv_list == NULL){
        fclose(cfg->fp_cfg);
        free(cfg);
        return NULL;
    }
    memset(cfg->kv_list,0,cfg->kv_list_capacity * sizeof(kv_t*));

    z_config_reader_readall(cfg);

    return cfg;
}

void z_config_reader_free(z_config_reader_t* cfg){
    if(cfg == NULL){
        return;
    }

    if(cfg->fp_cfg != NULL){
        fclose(cfg->fp_cfg);
        cfg->fp_cfg = NULL;
    }

    for(size_t i = 0; i < cfg->kv_list_size; ++i){
        kv_t* kv = cfg->kv_list[i];
        if(kv == NULL){
            continue;
        }
        free_kv(kv);
    }

    if(cfg->kv_list != NULL){
        free(cfg->kv_list);
        cfg->kv_list = NULL;
    }

    free(cfg);
}

///< read
/// return line length
static int z_config_reader_read(z_config_reader_t* cfg,kv_t* kv){
    if(cfg == NULL || kv == NULL || cfg->fp_cfg == NULL){
        return 0;
    }

    const char RETURN_CHAR = '\n';
    const char COLON_CHAR = ':';
    const char COMMENT_CHAR = '/';

    char* buf = (char*)malloc(1024*64);
    if(buf == NULL){
        return -1;
    }

    memset(buf,0,1024*64);

    char* ret = fgets(buf,1024*64,cfg->fp_cfg);
    if(ret == NULL){
        free(buf);
        return -1;
    }

    int keyMaxLen = 0;
    int valueMaxLen = 0;
    /// ":" index
    int colonIndex = findChar(buf,COLON_CHAR);
    if(colonIndex < 0){
        keyMaxLen = strlen(buf) + 8;
        valueMaxLen = 8;
    }
    else if(colonIndex == 0){
        keyMaxLen = 8;
        valueMaxLen = strlen(buf) + 8;
    }
    else{
        keyMaxLen = (colonIndex + 1) + 8;
        valueMaxLen = strlen(buf) - (colonIndex + 1) + 8;
    }

    if(keyMaxLen > KEY_DEFAULT_SIZE){
        kv->key = (char*)realloc(kv->key,keyMaxLen);
        if(kv->key == NULL){
            free_kv(kv);
            free(buf);
            return -1;
        }
    }

    if(valueMaxLen > VALUE_DEFAULT_SIZE){
        kv->value = (char*)realloc(kv->value,valueMaxLen);
        if(kv->value == NULL){
            free_kv(kv);
            free(buf);
            return -1;
        }
    }

    int valueLen = 0;
    int keyLen = 0;
    int index = 0;
    char lastChar = 0;
    char currChar = 0;

    int seeColon = 0;
    int seeComment = 0;

    while (buf[index] != RETURN_CHAR) {
        currChar = buf[index];
        ++index;

        kv->value[valueLen] = 0;
        kv->key[keyLen] = 0;

        if(currChar == COLON_CHAR && seeColon == 0){
            seeColon = 1;
            lastChar = currChar;
            continue;
        }

        if(currChar == COMMENT_CHAR && lastChar == COMMENT_CHAR){
            seeComment = 1;
            break;
        }

        if(currChar == COMMENT_CHAR && buf[index] != 0){
            if(buf[index] == COMMENT_CHAR){
                lastChar = currChar;
                continue;
            }
        }

        if(seeColon == 0){
            kv->key[keyLen] = currChar;
            ++keyLen;
        }
        else{
            kv->value[valueLen] = currChar;
            ++valueLen;
        }
        lastChar = currChar;
    }

    kv->value[valueLen++] = 0;
    kv->key[keyLen++] = 0;

    trim(kv->key,kv->key);
    trim(kv->value,kv->value);

    kv->key_len = strlen(kv->key)+1;
    kv->value_len = strlen(kv->value)+1;

    free(buf);

    return index;
}

static void z_config_reader_readall(z_config_reader_t* cfg){
    while(!feof(cfg->fp_cfg)){
        kv_t* kv = alloc_kv();
        if(kv == NULL){
            break;
        }

        int ret = z_config_reader_read(cfg,kv);
        if(ret < 0){
            if(kv->key){
                free(kv->key);
            }

            if(kv->value){
                free(kv->value);
            }

            free(kv);
            break;
        }

        if(strlen(kv->key) == 0){
            if(kv->key){
                free(kv->key);
            }

            if(kv->value){
                free(kv->value);
            }

            free(kv);
            continue;
        }
        ++cfg->kv_list_size;

        if(cfg->kv_list_size == cfg->kv_list_capacity){
            cfg->kv_list_capacity += 64;
            cfg->kv_list = realloc(cfg->kv_list,cfg->kv_list_capacity * sizeof(kv_t*));
        }
        cfg->kv_list[cfg->kv_list_size - 1] = kv;
    }
}


int z_config_reader_read_int(z_config_reader_t* cfg,const char* key,int64_t* value){
    int findValue = -1;

    for(size_t i = 0; i < cfg->kv_list_size; ++i){
        kv_t* kv = cfg->kv_list[i];
        if(kv == NULL){
            continue;
        }

        if(0 != strcmp(key,kv->key)){
            continue;
        }

        *value = _atoi64(kv->value);

        findValue = 0;
        break;
    }
    return findValue;
}

int z_config_reader_read_float(z_config_reader_t* cfg,const char* key,double* value){
    int findValue = -1;
    for(size_t i = 0; i < cfg->kv_list_size; ++i){
        kv_t* kv = cfg->kv_list[i];
        if(kv == NULL){
            continue;
        }

        if(0 != strcmp(key,kv->key)){
            continue;
        }

        *value = atof(kv->value);

        findValue = 0;
        break;
    }
    return findValue;
}

int z_config_reader_read_string(z_config_reader_t* cfg,const char* key,char* value){
    int findValue = -1;
    for(size_t i = 0; i < cfg->kv_list_size; ++i){
        kv_t* kv = cfg->kv_list[i];
        if(kv == NULL){
            continue;
        }

        if(0 != strcmp(key,kv->key)){
            continue;
        }

        memcpy(value,kv->value,kv->value_len);

        findValue = 0;
        break;
    }
    return findValue;
}


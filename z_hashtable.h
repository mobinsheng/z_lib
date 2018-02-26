#ifndef Z_HASHTABLE_H
#define Z_HASHTABLE_H

#ifdef __cplusplus
extern "C"{
#endif

/* 开放地址的哈希表 */

/* 哈希表接口返回值
 */
typedef enum {
    Z_HT_STATUS_OK,
    Z_HT_STATUS_ERR,
    Z_HT_STATUS_MINSIZE  //一般用于插入或者删除，如果返回值等于Z_HT_STATUS_MINSIZE表示扩张空间失败
} z_hashtable_status;

/* key的类型
 */
typedef enum{
    Z_HT_KEY_STRING, // 字符串类型，注意，如果是字符串，哈希表内部会复制key，这是为了防止用户使用栈字符串作为key
    Z_HT_KEY_VOIDPTR, // void*类型，这种类型不会复制内存
}z_hashkey_type_t;

typedef struct z_hashtable_t z_hashtable_t;

/* 用户可以自定义哈希函数
 */
typedef unsigned long (*z_hashfunc_t) (void *);

/* 用户可以自定义key比较函数
 */
typedef int (*z_hashcmpfunc_t) (void *, void *);

z_hashtable_t* z_hashtable_create (z_hashkey_type_t keytype);
z_hashtable_t* z_hashtable_create_ex (z_hashfunc_t hfun, z_hashcmpfunc_t cfun, unsigned long initsize,char free_element_count );

z_hashtable_status z_hashtable_put (z_hashtable_t **ht, void *key, void *value);
void * z_hashtable_get (z_hashtable_t **ht, void *key);
z_hashtable_status z_hashtable_delete (z_hashtable_t **ht, void *key);

void z_hashtable_free (z_hashtable_t **ht);
void z_hashtable_clear (z_hashtable_t **ht);

#ifdef __cplusplus
}
#endif

#endif // Z_HASHTABLE_H

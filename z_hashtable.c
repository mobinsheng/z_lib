#include "z_hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include<string.h>
#include <memory.h>
#include <stdlib.h>
#include <stdint.h>

static unsigned long string_hash_func (void *str);
static int string_equal_func (void *a, void *b);
static unsigned long long_hash_func (void *n);
static int long_equal_func (void *a, void *b);

static unsigned long string_hash_func (void *p){
    unsigned long hash = 5381;
    int c;
    char* str = (char*)p;
    while ( (c = *str++) )
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return(hash);
}

static int string_equal_func (void *a, void *b){
    return(!strcmp ((char *)a, (char *)b));
}

static unsigned long long_hash_func (void *n){
    unsigned long l_n;
    l_n = *(int *)n;
    return(l_n);
}

static int long_equal_func (void *a, void *b){
    return((*(int*)a == *(int*)b));
}

//===============================
typedef struct cell {
    void *key;
    void *value;
}cell;

/* Open addressing hashtable implementation
   The capacity of the table is doubled when element_count/capacity > 0.75
   The capacity of the table is halved when element_count/capacity < 0.25
   The capacity can never be less than mincapacity
   hashfunc is a pointer to a function that generates the hash from a key
   key_equal_func returns 1 when its two arguments are equal
   When inserting a key, if the key already exists, the value is overwritten*/
struct z_hashtable_t {
    unsigned long element_count;
    unsigned long capacity;
    unsigned long min_capacity;
    z_hashfunc_t hash_func;
    z_hashcmpfunc_t key_equal_func;
    cell *cells;
    short free_element_count;
    z_hashkey_type_t key_type;
};

static char* alloc_string(void* str){
    char* src = (char*)str;
    if(src == NULL){
        return NULL;
    }

    size_t len = strlen(src);
    char* dst = (char*)malloc(len + 1);
    strcpy(dst,src);
    return dst;
}

static void free_string(void* str){
    free(str);
}

void z_hashtable_free (z_hashtable_t **ht)
{
    if(ht == NULL || *ht == NULL){
        return;
    }

    if((*ht)->cells != NULL && (*ht)->key_type == Z_HT_KEY_STRING){
        cell* curcell = (*ht)->cells + (*ht)->capacity;
        while (curcell-- != (*ht)->cells){
            if (curcell->key){
                free_string(curcell->key);
                curcell->key = NULL;
            }
        }
    }

    free ((*ht)->cells);
    free (*ht);
    *ht = NULL;
    return;
}

void z_hashtable_clear (z_hashtable_t **ht)
{
    cell *curcell;
    curcell = (*ht)->cells + (*ht)->capacity;
    if ((*ht)->free_element_count) {
        while (curcell-- != (*ht)->cells){
            if (curcell->key) {
                if((*ht)->key_type == Z_HT_KEY_STRING){
                    free_string(curcell->key);
                }
                curcell->key = NULL;
            }
        }
    }
    else {
        memset((*ht)->cells,0, sizeof(cell) * (*ht)->capacity);
    }
    (*ht)->element_count = 0;
    return;
}

static z_hashtable_status copy_table (z_hashtable_t **dest, z_hashtable_t **orig)
{
    z_hashtable_status st;
    cell *curcell;
    if ((*dest)->capacity < (*orig)->element_count){
        return(Z_HT_STATUS_ERR);
    }

    curcell = (*orig)->cells + (*orig)->capacity;

    while (curcell-- != (*orig)->cells){
        if (curcell->key){
            if ( (st = z_hashtable_put (dest, curcell->key, curcell->value)) != Z_HT_STATUS_OK ) {
                return(st);
            }
        }
    }
    return(Z_HT_STATUS_OK);
}

static z_hashtable_status increase_table (z_hashtable_t **ht)
{
    z_hashtable_t *newht;
    z_hashtable_status st;
    if ( (newht = z_hashtable_create_ex ((*ht)->hash_func, (*ht)->key_equal_func, (*ht)->capacity*2 + 1, (*ht)->free_element_count)) == NULL ){
        return(Z_HT_STATUS_ERR);
    }

    newht->key_type = (*ht)->key_type;

    newht->min_capacity = (*ht)->min_capacity;
    if ( (st = copy_table (&newht,ht)) != Z_HT_STATUS_OK ) {
        z_hashtable_free (&newht);
        return(st);
    }
    z_hashtable_free (ht);
    *ht = newht;
    return(Z_HT_STATUS_OK);
}

static z_hashtable_status decrease_table (z_hashtable_t **ht)
{
    z_hashtable_t *newht;
    unsigned long newcapacity;
    z_hashtable_status st;
    /* The capacity cannot be less than min_capacity
       If capacity is equal to min_capacity do nothing
       If capacity is less than min_capacity, set capacity to min_capacity */
    if ((*ht)->capacity == (*ht)->min_capacity)
        return(Z_HT_STATUS_MINSIZE);
    newcapacity = (*ht)->capacity / 2;
    if (newcapacity < (*ht)->min_capacity)
        newcapacity = (*ht)->min_capacity;
    if (newcapacity % 2 == 0)
        newcapacity++;
    if ( (newht = z_hashtable_create_ex ((*ht)->hash_func, (*ht)->key_equal_func, newcapacity, (*ht)->free_element_count)) == NULL){
        return(Z_HT_STATUS_ERR);
    }
    newht->key_type = (*ht)->key_type;
    newht->min_capacity = (*ht)->min_capacity;
    if ( (st = copy_table (&newht, ht))  != Z_HT_STATUS_OK ) {
        z_hashtable_free (&newht);
        return(st);
    }
    z_hashtable_free (ht);
    *ht = newht;
    return(Z_HT_STATUS_OK);
}

/*@null@*/
z_hashtable_t* z_hashtable_create_ex (z_hashfunc_t hfun, z_hashcmpfunc_t cfun, unsigned long initcapacity,char free_element_count )
{
    z_hashtable_t *ht = NULL;

    if ( (ht = (z_hashtable_t *) malloc (sizeof(z_hashtable_t))) == NULL )
        return(NULL);

    if (initcapacity % 2 == 0)
        initcapacity++;

    if ( (ht->cells = (cell*)malloc (sizeof(cell) * initcapacity)) == NULL ) {
        free (ht);
        return(NULL);
    }

    ht->element_count = 0;
    ht->capacity = ht->min_capacity = initcapacity;
    ht->hash_func = hfun;
    ht->key_equal_func = cfun;
    ht->free_element_count = free_element_count;

    memset(ht->cells,0, sizeof(cell) * initcapacity);
    return(ht);
}

z_hashtable_t* z_hashtable_create (z_hashkey_type_t keytype){
    unsigned long initcapacity = 256;
    char free_element_count = 5;
    z_hashtable_t* ht = NULL;
    if(keytype == Z_HT_KEY_STRING){
        ht = z_hashtable_create_ex(string_hash_func,string_equal_func,initcapacity,free_element_count);
    }
    else if(keytype == Z_HT_KEY_VOIDPTR){
        ht = z_hashtable_create_ex(long_hash_func,long_equal_func,initcapacity,free_element_count);
    }

    if(ht){
        ht->key_type = keytype;
    }

    return ht;
}


z_hashtable_status z_hashtable_put (z_hashtable_t **ht, void *key, void *value)
{
    unsigned long index;
    cell *curcell;
    index = (*ht)->hash_func (key) % (*ht)->capacity;
    curcell = &((*ht)->cells[index]);

    /* key == NULL表示当前位置没有元素
     * 判断条件：当前位置的key的哈希值不同
     */
    while (curcell->key &&!(*ht)->key_equal_func (key, curcell->key)){
        if (++curcell == (*ht)->cells + (*ht)->capacity){
            curcell = (*ht)->cells;
        }
    }

    curcell->key = key;

    if((*ht)->key_type == Z_HT_KEY_STRING){
        curcell->key = alloc_string(key);
    }

    curcell->value = value;

    if ((double)++((*ht)->element_count) / (double)(*ht)->capacity > 0.75){
        return(increase_table (ht));
    }

    return(Z_HT_STATUS_OK);
}

/*@null@*/
void * z_hashtable_get (z_hashtable_t **ht, void *key)
{
    unsigned long index;
    index = (*ht)->hash_func (key) % (*ht)->capacity;
    /* Search the occupied slots for a match with the key */
    while ((*ht)->cells[index].key) {
        if ((*ht)->key_equal_func (key, (*ht)->cells[index].key))
            return((*ht)->cells[index].value);
        index = (index + 1) % (*ht)->capacity;
    }
    return(NULL);
}

/*@null@*/
z_hashtable_status z_hashtable_delete (z_hashtable_t **ht, void *key)
{
    int found;
    unsigned long index;
    unsigned long hash;
    cell *curcell;
    cell *oldcell;
    cell *hashcell;
    z_hashtable_status st;
    hash = (*ht)->hash_func (key);
    index = hash % (*ht)->capacity;
    curcell = &((*ht)->cells[index]);
    found = 0;
    st = Z_HT_STATUS_MINSIZE;
    while (curcell->key) {
        if (!found && (*ht)->key_equal_func (key, curcell->key)) {
            if((*ht)->key_type == Z_HT_KEY_STRING){
                free_string(curcell->key);
            }

            curcell->key = NULL;
            oldcell = curcell;
            found = 1;
            if ((double)--(*ht)->element_count / (double)(*ht)->capacity < 0.25) {
                st = decrease_table (ht);
            }
        }
        else if (st == Z_HT_STATUS_MINSIZE && found) {
            /* Get the pointer to the first valid position
               of the element in this cell */
            hashcell = &((*ht)->cells[(*ht)->hash_func (curcell->key) %  (*ht)->capacity]);
            /* This expression avoids hashcell being between
               curcell (low) and oldcell (high) */
            if ((curcell < oldcell && curcell < hashcell && hashcell <= oldcell) ||
                    (curcell > oldcell && (curcell < hashcell || hashcell <= oldcell))) {

                if((*ht)->key_type == Z_HT_KEY_STRING){
                    free_string(oldcell->key);
                    oldcell->key = NULL;
                }

                oldcell->key = curcell->key;
                oldcell->value = curcell->value;
                oldcell = curcell;


                curcell->key = NULL;
            }
        }

        if (st != Z_HT_STATUS_MINSIZE){
            return(st);
        }

        if (++curcell == (*ht)->cells + (*ht)->capacity){
            curcell = (*ht)->cells;
        }
    }

    return(Z_HT_STATUS_OK);
}

//====================================================

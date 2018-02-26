#ifndef z_list_H
#define z_list_H


#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

/*
 * 同时提供链表、队列、栈的功能
 */

typedef enum z_list_type{
    z_list_type_int,
    z_list_type_float,
    z_list_type_pvoid,
}z_list_type;

/*****************定义void指针链表以及接口**********************/
struct z_list_node_t{
    void* ptr_val;
    struct z_list_node_t* prev;
    struct z_list_node_t* next;
};
typedef struct z_list_node_t z_list_node_t;
typedef struct z_list_t z_list_t;

#define z_list_foreach(list,node) for(node = z_list_head(list);node != NULL;node = node->next)

z_list_t* z_list_create();
void z_list_free(z_list_t* list);

z_list_node_t* z_list_pushback(z_list_t* list,void* data);
z_list_node_t* z_list_pushfront(z_list_t* list,void* data);

void z_list_popback(z_list_t* list);
void z_list_popfront(z_list_t* list);

z_list_node_t* z_list_head(z_list_t* list);
z_list_node_t* z_list_tail(z_list_t* list);

z_list_node_t* z_list_insert_after(z_list_t* list,z_list_node_t* pos,void* data);
z_list_node_t* z_list_insert_before(z_list_t* list,z_list_node_t* pos,void* data);

void z_list_remove(z_list_t* list,z_list_node_t* node);

size_t z_list_size(z_list_t* list);

/*****************定义浮点型链表以及接口**********************/
struct z_list_float_node_t{
    double float_val;
    struct z_list_float_node_t* prev;
    struct z_list_float_node_t* next;
};
typedef struct z_list_float_node_t z_list_float_node_t;
typedef struct z_list_float_t z_list_float_t;

#define z_list_float_foreach(list,node) for(node = z_list_float_head(list);node != NULL;node = node->next)

z_list_float_t* z_list_float_create();
void z_list_float_free(z_list_float_t* list);

z_list_float_node_t* z_list_float_pushback(z_list_float_t* list,double data);
z_list_float_node_t* z_list_float_pushfront(z_list_float_t* list,double data);

void z_list_float_popback(z_list_float_t* list);
void z_list_float_popfront(z_list_float_t* list);

z_list_float_node_t* z_list_float_head(z_list_float_t* list);
z_list_float_node_t* z_list_float_tail(z_list_float_t* list);

z_list_float_node_t* z_list_float_insert_after(z_list_float_t* list,z_list_float_node_t* pos,double data);
z_list_float_node_t* z_list_float_insert_before(z_list_float_t* list,z_list_float_node_t* pos,double data);

void z_list_float_remove(z_list_float_t* list,z_list_float_node_t* node);

size_t z_list_float_size(z_list_float_t* list);
/**********************定义整数链表以及接口***********************************/
struct z_list_int_node_t{
    int64_t int_val;
    struct z_list_int_node_t* prev;
    struct z_list_int_node_t* next;
};
typedef struct z_list_int_node_t z_list_int_node_t;
typedef struct z_list_int_t z_list_int_t;

#define z_list_int_foreach(list,node) for(node = z_list_int_head(list);node != NULL;node = node->next)

z_list_int_t* z_list_int_create();
void z_list_int_free(z_list_int_t* list);

z_list_int_node_t* z_list_int_pushback(z_list_int_t* list,int64_t data);
z_list_int_node_t* z_list_int_pushfront(z_list_int_t* list,int64_t data);

void z_list_int_popback(z_list_int_t* list);
void z_list_int_popfront(z_list_int_t* list);

z_list_int_node_t* z_list_int_head(z_list_int_t* list);
z_list_int_node_t* z_list_int_tail(z_list_int_t* list);

z_list_int_node_t* z_list_int_insert_after(z_list_int_t* list,z_list_int_node_t* pos,int64_t data);
z_list_int_node_t* z_list_int_insert_before(z_list_int_t* list,z_list_int_node_t* pos,int64_t data);

void z_list_int_remove(z_list_int_t* list,z_list_int_node_t* node);

size_t z_list_int_size(z_list_int_t* list);


#ifdef __cplusplus
}
#endif

#endif // z_list_H

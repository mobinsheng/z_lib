#include "z_list.h"
#include<string.h>
#include <memory.h>

struct z_list_float_t{
    z_list_float_node_t* head;
    z_list_float_node_t* tail;
    size_t size;
};

z_list_float_t* z_list_float_create(){
    z_list_float_t* list = (z_list_float_t*)malloc(sizeof(z_list_float_t));
    list->head = list->tail = NULL;
    list->size = 0;
    return list;
}

void z_list_float_free(z_list_float_t* list){
    z_list_float_node_t* node = list->head;
    while (node) {
        z_list_float_node_t* next = node->next;

        free(node);

        node = next;
    }

    list->head = list->tail = NULL;
    list->size = 0;
    free(list);
}

z_list_float_node_t* z_list_float_pushback(z_list_float_t* list,double data){
    z_list_float_node_t* node = (z_list_float_node_t*)malloc(sizeof(z_list_float_node_t));
    memcpy(&node->float_val,&data,sizeof(data));
    node->next = node->prev = NULL;

    if(list->tail == NULL){
        list->head = list->tail = node;
    }
    else{
        list->tail->next = node;
        node->prev = list->tail;

        list->tail = node;
    }
    ++list->size;
    return node;
}

z_list_float_node_t* z_list_float_pushfront(z_list_float_t* list,double data){
    z_list_float_node_t* node = (z_list_float_node_t*)malloc(sizeof(z_list_float_node_t));
    memcpy(&node->float_val,&data,sizeof(data));
    node->next = node->prev = NULL;

    if(list->head == NULL){
        list->head = list->tail = node;
    }
    else{
        node->next = list->head;
        list->head->prev = node;

        list->head = node;
    }
    ++list->size;
    return node;
}

void z_list_float_popback(z_list_float_t* list){
    z_list_float_node_t* node = list->tail;
    z_list_float_remove(list,node); // 内部有对size的操作
}

void z_list_float_popfront(z_list_float_t* list){
    z_list_float_node_t* node = list->head;
    z_list_float_remove(list,node); // 内部有对size的操作
}

z_list_float_node_t* z_list_float_head(z_list_float_t* list){
    return list->head;
}

z_list_float_node_t* z_list_float_tail(z_list_float_t* list){
    return list->tail;
}

z_list_float_node_t* z_list_float_insert_after(z_list_float_t* list,z_list_float_node_t* pos,double data){
    if(pos->next == NULL){
        return z_list_float_pushback(list,data); // 内部有对size的操作
    }

    z_list_float_node_t* node = (z_list_float_node_t*)malloc(sizeof(z_list_float_node_t));
    memcpy(&node->float_val,&data,sizeof(data));
    node->next = node->prev = NULL;

    // 记录pos的下一个节点
    z_list_float_node_t* org_next = pos->next;

    pos->next = node;
    node->prev = pos;

    node->next = org_next;
    org_next->prev = node;

    ++list->size;
    return node;
}

z_list_float_node_t* z_list_float_insert_before(z_list_float_t* list,z_list_float_node_t* pos,double data){
    if(pos->prev == NULL){
        return z_list_float_pushfront(list,data); // 内部有对size的操作
    }

    z_list_float_node_t* node = (z_list_float_node_t*)malloc(sizeof(z_list_float_node_t));
    memcpy(&node->float_val,&data,sizeof(data));
    node->next = node->prev = NULL;

    z_list_float_node_t* org_prev = pos->prev;

    // 把node连接到pos的前面
    node->next = pos;
    pos->prev = node;

    // 把node连接到org-prev的后面
    org_prev->next = node;
    node->prev = org_prev;

    ++list->size;
    return node;
}

void z_list_float_remove(z_list_float_t* list,z_list_float_node_t* node){
    z_list_float_node_t* org_prev = node->prev;
    z_list_float_node_t* org_next = node->next;

    --list->size;

    if(node == list->head && node == list->tail){
        list->head = list->tail = NULL;

        free(node);

        return;
    }

    if(node == list->head){
        org_next->prev = NULL;
        list->head = org_next;
    }
    else if(node == list->tail){
        org_prev->next = NULL;
        list->tail = org_prev;
    }
    else{
        org_next->prev = org_prev;
        org_prev->next = org_next;
    }
    free(node);
}

size_t z_list_float_size(z_list_float_t* list){
    return list->size;
}

/*************************************************************/


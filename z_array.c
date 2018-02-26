#include "z_array.h"

z_array_t* z_array_create(){
    z_array_t* array = (z_array_t*)malloc(sizeof(z_array_t));
    array->elements = NULL;
    array->capacity = array->size = 0;
    return array;
}

void z_array_free(z_array_t* array){
    if(array->elements){
        free(array->elements);
    }
    array->elements = NULL;
    array->capacity = array->size = 0;
    free(array);
}

void z_array_pushback(z_array_t* array,z_array_data_t data){
    if(array->elements == NULL){
        array->capacity = 4;
        array->elements = (z_array_data_t*)malloc(sizeof(z_array_data_t) * array->capacity);
    }

    if(array->capacity == array->size){
        array->capacity *= 2;
        array->elements = (z_array_data_t*)realloc(array->elements,array->capacity * sizeof(z_array_data_t));
    }

    array->elements[array->size++] = data;
}

z_array_data_t z_array_popback(z_array_t* array){

    z_array_data_t data = array->elements[--array->size];

    if(array->size  < array->capacity / 4){
        array->capacity /= 2;
        array->elements = (z_array_data_t*)realloc(array->elements,array->capacity* sizeof(z_array_data_t));
    }

    return data;
}

z_array_data_t z_array_at(z_array_t* array,size_t pos){
    return array->elements[pos];
}

size_t z_array_size(z_array_t* array){
    return array->size;
}

#include "drpc_array.h"
#include "drpc_que.h"

#include <assert.h>
#include <stdlib.h>

struct d_array* new_d_array(size_t start_cappacity){

    if(start_cappacity == 0) start_cappacity = 1;

    struct d_array* new = malloc(sizeof(*new));
    assert(new);

    new->lookup_size = start_cappacity;

    new->lookup_table = calloc(new->lookup_size,sizeof(*new->lookup_table));
    assert(new->lookup_table);

    new->storage = drpc_que_create();

    return new;
}

void d_array_set_internal(struct d_array* darray, size_t index, void* el){
    if(index >= darray->lookup_size){
        void* realloced = realloc(darray->lookup_table,sizeof(*darray->lookup_table) * (index+1));
        assert(realloced);

        darray->lookup_table = realloced;
        darray->lookup_size = index+1;
    }

    struct drpc_que_el* lookup_el = drpc_que_push_rp(darray->storage,el);
    darray->lookup_table[index] = lookup_el;
}

void* d_array_get_internal(struct d_array* darray, size_t index){
    if(index >= darray->lookup_size) return NULL;

    return darray->lookup_table[index];
}

void* d_array_del_internal(struct d_array* darray, size_t index){
    if(index >= darray->lookup_size) return NULL;
    if(darray->lookup_table[index] == NULL) return NULL;

    struct drpc_que_el* el = darray->lookup_table[index];

    void* ret = darray->lookup_table[index]->ptr;
    darray->lookup_table[index] = NULL;

    uint64_t q = drpc_que_get_len(darray->storage);
    for(uint64_t i = 0; i < q; i++){
        void* pop = drpc_que_pop_el(darray->storage);
        if(el == pop) break;
        else          drpc_que_push_el(darray->storage,pop);
    }

    void* freep = el;
    free(freep);

    return ret;

}

struct drpc_que* d_array_free_internal(struct d_array* darray){
    void* ret = darray->storage;

    free(darray->lookup_table);
    free(darray);

    return ret;
}

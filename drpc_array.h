#include <sys/types.h>
#include "drpc_que.h"

struct d_array{
    size_t lookup_size;
    struct drpc_que_el** lookup_table;

    struct drpc_que* storage; //i want to save some memory for "low density" arrays
};

struct d_array* new_d_array(size_t start_cappacity);


void d_array_set_internal(struct d_array* darray, size_t index, void* el);
void* d_array_get_internal(struct d_array* darray, size_t index);
void* d_array_del_internal(struct d_array* darray, size_t index);
struct drpc_que* d_array_free_internal(struct d_array* darray); //return queue with user data pointers

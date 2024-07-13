#include <stdint.h>
struct md_array{
  struct md_array *next,*child;
  void* endpoint;
  uint64_t elen;
};
struct md_index{
    struct md_index* next;
    uint64_t index;
};
int __md_array_create(struct md_array* start,struct md_index* sub_dims,uint64_t endpoint_len,uint64_t dim_amm);
char* __md_array_get_lastdim_at(struct md_array* md_arr, struct md_index* index);
void __md_array_free(struct md_array* md_arr,char flag);

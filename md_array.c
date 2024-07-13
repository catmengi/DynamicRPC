#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "tqueque/tqueque.h"
#include <stdint.h>

struct md_array{
  struct md_array *next,*child;
  char* endpoint;
};
struct md_index{
    struct md_index* next;
    uint64_t index;
};
int __md_array_create(struct md_array* start,struct md_index* sub_dims,uint64_t endpoint_len,uint64_t sdim_amm){
    assert(start != NULL);
    assert(sub_dims != NULL || (sdim_amm == 0 && sub_dims == NULL));
    struct tqueque* sque = tqueque_create();
    tqueque_push(sque,start,0,"len");
    struct md_array* cur = start;
    int dbg = 0;
    //dim_amm -= 1;
    int id = 0;
    for(id = 0;sdim_amm > 0; sdim_amm--,id++){
        for(uint64_t iter = tqueque_get_tagamm(sque,"len"); iter > 0; --iter){
            cur = tqueque_pop(sque,NULL,"len");
            tqueque_push(sque,cur,0,"child");
            for(uint64_t i = 0; i < sub_dims->index-1 && sub_dims->index > 0;i++){
                cur->next = calloc(1,sizeof(*cur));
                assert(cur->next);
                tqueque_push(sque,cur->next,0,"child");
                cur = cur->next;
            }
            cur = NULL;
        }
        sub_dims = sub_dims->next;
        //assert(sub_dims);
        for(uint64_t iter = tqueque_get_tagamm(sque,"child"); iter > 0 && sdim_amm - 1 > 0; --iter){
            cur = tqueque_pop(sque,NULL,"child");
            cur->child = calloc(1,sizeof(*cur));
            assert(cur->child);
            tqueque_push(sque,cur->child,0,"len");
            dbg++;
        }
    }
    cur = NULL;
    while((cur = tqueque_pop(sque,NULL,0)) != NULL){
        cur->endpoint = calloc(endpoint_len,sizeof(char));
        assert(cur->endpoint != NULL);
    }
    tqueque_free(sque);
    return 0;
}

/*char* __md_array_get_lastdim_at(struct md_array* md_arr, struct md_index* index){
    //require md_array_dim_amm - 1 sized index or more, if more it will stop on endpoint detection(child == NULL)
    struct md_array* cur = md_arr;
        while(cur != NULL && cur->child != NULL && index){
            for(uint64_t i = 0; i < index->index && cur != NULL; i++){
                if(cur == NULL)           //index overflow detection
                    return NULL;
                cur = cur->next;
            }
            cur = cur->child;
            if(index->next == NULL)       //index have less dimensions that md_array; Wont work vice-verse, it will stop on last index when endpoint is detected
                return NULL;
            index = index->next;
        }
    if(index != NULL) for(uint64_t i = 0; i < index->index && cur != NULL; i++,cur = cur->next);
    return cur->endpoint;
}*/

char* __md_array_get_lastdim_at(struct md_array* md_arr, struct md_index* index){
    assert(md_arr);
    struct md_array* cur = md_arr;
    if(!index) return cur->endpoint;
    while(cur->child != NULL && index != NULL){
        for(int i = 0; i < index->index; i++, cur = cur->next) if(cur == NULL) return NULL;
        cur = cur->child;
        index = index->next;
    }
    if(!index) return NULL;
    for(uint64_t i = 0; i < index->index; i++) {cur = cur->next; if(cur == NULL) return NULL;}
    return cur->endpoint;
}

void __md_array_free_traverse(struct md_array* start, struct tqueque* fque){
    while(start){
        tqueque_push(fque,start,0,NULL);
        if(start->child)
            __md_array_free_traverse(start->child,fque);   /*RECURSION! MAY BREAK ON TOO DEEP ARRAYS*/
        start = start->next;
    }
}

void __md_array_free(struct md_array* md_arr,char flag){
    if(md_arr == NULL)
        return;
    struct tqueque* fque = tqueque_create();
    __md_array_free_traverse(md_arr,fque);
    struct md_array* cur;
    while((cur = tqueque_pop(fque,NULL,NULL)) != NULL){
        if(flag) free(cur->endpoint);
        free(cur);
    }
    tqueque_free(fque);
}
int main(){

        struct md_index i4 = {NULL,129};
        struct md_index i3 = {&i4,15};
        struct md_index i2 = {&i3,17};
        struct md_index i1 = {&i2,3};
        struct md_array* start = calloc(1,sizeof(*start));
        __md_array_create(start,&i1,32,4);

}

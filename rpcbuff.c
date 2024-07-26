#include <assert.h>
#include <stdint.h>
#include <libubox/utils.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "tqueque/tqueque.h"
#include <stdint.h>
#include <string.h>
#include "rpctypes.h"

struct rpcbuff* rpcbuff_create(uint64_t* dimsizes,uint64_t dimsizes_len,uint64_t lastdim_len){
    struct __rpcbuff_el* md_array = calloc(1,sizeof(struct __rpcbuff_el));
    struct rpcbuff* cont = NULL;
    if(dimsizes != NULL || dimsizes_len == 0){
        cont = calloc(1,sizeof(struct rpcbuff));
        assert(cont);
    }
    assert(md_array);
    if(dimsizes_len > 0){
        cont->dimsizes = calloc(dimsizes_len, sizeof(uint64_t));
        assert(cont->dimsizes);
        memcpy(cont->dimsizes,dimsizes, sizeof(uint64_t) *dimsizes_len);
    }
    cont->dimsizes_len = dimsizes_len;
    cont->start = md_array;
    cont->lastdim_len = lastdim_len;
    if(dimsizes != NULL && dimsizes_len > 0){
        struct tqueque* que = tqueque_create();
        assert(que);
        assert(tqueque_push(que,md_array,sizeof(struct __rpcbuff_el*),NULL) == 0);
        for(uint64_t i = 0; i < dimsizes_len; i++){
            struct __rpcbuff_el* cur = NULL;
            uint64_t iter = tqueque_get_tagamm(que,NULL);
            for(uint64_t j = 0; j < iter; j++){
                cur = tqueque_pop(que,NULL,NULL);
                cur->childs = calloc(dimsizes[i],sizeof(struct __rpcbuff_el));
                assert(cur->childs);
                for(uint64_t k = 0; k <dimsizes[i]; k++){
                    assert(tqueque_push(que,&cur->childs[k], sizeof(struct __rpcbuff_el*), NULL) == 0);
                }
            }
        }
        struct __rpcbuff_el* cur = NULL;
        while((cur = tqueque_pop(que,NULL,NULL)) != NULL){
            cur->endpoint = (char*)0xCAFE;
        }
        tqueque_free(que);
        return cont;
    }
    md_array->endpoint = (char*)0xCAFE;
    md_array->elen = 0;
    assert(md_array->endpoint);
    return cont;
}
void _rpcbuff_free(struct rpcbuff* rpcbuff){
    assert(rpcbuff);
    struct tqueque* tque = tqueque_create();
    struct tqueque* fque = tqueque_create();
    assert(tque != NULL && fque != NULL);
    tqueque_push(tque,rpcbuff->start,sizeof(struct __rpcbuff_el*),NULL);
    for(uint64_t i = 0; i < rpcbuff->dimsizes_len; i++){
        uint64_t iter = tqueque_get_tagamm(tque,NULL);
        for(uint64_t j = 0; j < iter; j++){
            struct __rpcbuff_el* cur = tqueque_pop(tque,NULL,NULL);
            tqueque_push(fque,cur->childs,sizeof(struct __rpcbuff_el*),NULL);
            for(uint64_t k = 0; k < rpcbuff->dimsizes[i]; k++){
                if(cur->childs[k].endpoint != NULL)  tqueque_push(fque,cur->childs[k].endpoint,sizeof(struct __rpcbuff_el*),NULL);
                tqueque_push(tque,&cur->childs[k],sizeof(struct __rpcbuff_el*),NULL);
            }
        }
    }
    tqueque_free(tque);
    void* fcur = NULL;
    while((fcur = tqueque_pop(fque,NULL,NULL)) != NULL){
        if(fcur != (void*)0xCAFE) free(fcur);
    }
    if(rpcbuff->dimsizes == NULL) free(rpcbuff->start->endpoint);
    tqueque_free(fque);
    free(rpcbuff->start);
    free(rpcbuff->dimsizes);
    free(rpcbuff);
}
char* rpcbuff_getlast_from(struct  rpcbuff* rpcbuff, uint64_t* index, uint64_t index_len,uint64_t* outlen){
    assert(rpcbuff);
    assert(rpcbuff->start);
    if(rpcbuff->dimsizes_len != index_len) return NULL;
    if(rpcbuff->dimsizes_len != 0 && index == NULL) return NULL;
    struct __rpcbuff_el* cur = rpcbuff->start;
    for(uint64_t i = 0; i < rpcbuff->dimsizes_len; i++){
        if(index[i] >= rpcbuff->dimsizes[i]) return NULL;
        cur = &cur->childs[index[i]];
    }
    if(cur == NULL) return NULL;
    if(cur->endpoint == (void*)0xCAFE){
        cur->endpoint = calloc(rpcbuff->lastdim_len,sizeof(char*));
        cur->elen = rpcbuff->lastdim_len;
        assert(cur->endpoint);
    }
    if(outlen) *outlen = cur->elen;
    return cur->endpoint;
}
struct __rpcbuff_el* __rpcbuff_el_getlast_from(struct  rpcbuff* rpcbuff, uint64_t* index, uint64_t index_len){
    assert(rpcbuff);
    assert(rpcbuff->start);
    if(rpcbuff->dimsizes_len != index_len) return NULL;
    if(rpcbuff->dimsizes_len != 0 && index == NULL) return NULL;
    struct __rpcbuff_el* cur = rpcbuff->start;
    for(uint64_t i = 0; i < rpcbuff->dimsizes_len; i++){
        if(index[i] >= rpcbuff->dimsizes[i]) return NULL;
        cur = &cur->childs[index[i]];
    }
    if(cur == NULL) return NULL;
    return cur;
}
int rpcbuff_pushto(struct rpcbuff* rpcbuff, uint64_t* index, uint64_t index_len, char* data, uint64_t data_len){
    assert(rpcbuff);
    struct __rpcbuff_el* got = __rpcbuff_el_getlast_from(rpcbuff,index,index_len);
    assert(got);
    if(got->elen >= data_len){memset(got->endpoint,0,got->elen);memcpy(got->endpoint, data, data_len); return 0;}
    if(got->elen < data_len && got->elen != 0){
        got->elen = data_len;
        got->endpoint = realloc(got->endpoint,got->elen);
        assert(got->endpoint);
        memset(got->endpoint,0,got->elen);
        memcpy(got->endpoint, data, got->elen);
        return 0;
    }
    got->endpoint = malloc(data_len);
    got->elen = data_len;
    memcpy(got->endpoint, data, data_len);
    return 0;
}
struct _prpcbuff_dim{
    char* data;
    uint64_t elen;
    struct _prpcbuff_dim* next;
};
char* rpcbuff_to_arr(struct rpcbuff* rpcbuff,uint64_t* buflen){
    assert(rpcbuff);
    struct tqueque* tque = tqueque_create();
    struct tqueque* fque = tqueque_create();
    assert(tque != NULL && fque != NULL);
    tqueque_push(tque,rpcbuff->start,sizeof(struct __rpcbuff_el*),NULL);
    if(rpcbuff->dimsizes_len == 0 || rpcbuff->dimsizes == NULL){
        tqueque_push(fque,rpcbuff->start,sizeof(struct __rpcbuff_el*),NULL);
    }
    for(uint64_t i = 0; i < rpcbuff->dimsizes_len; i++){
        uint64_t iter = tqueque_get_tagamm(tque,NULL);
        for(uint64_t j = 0; j < iter; j++){
            struct __rpcbuff_el* cur = tqueque_pop(tque,NULL,NULL);
            for(uint64_t k = 0; k < rpcbuff->dimsizes[i]; k++){
                if(cur->childs[k].endpoint != NULL) tqueque_push(fque,&cur->childs[k],sizeof(struct __rpcbuff_el*),NULL);
                tqueque_push(tque,&cur->childs[k],sizeof(struct __rpcbuff_el*),NULL);
            }
        }
    }
    struct __rpcbuff_el* fcur = NULL;
    char* out = NULL;
    char* ret = NULL;
    struct _prpcbuff_dim* start = calloc(1,sizeof(*start));
    assert(start);
    struct _prpcbuff_dim* cur = start;
    if((fcur = tqueque_pop(fque,NULL,NULL)) != NULL){
            cur->data = fcur->endpoint;
            cur->elen = fcur->elen;
        while((fcur = tqueque_pop(fque,NULL,NULL)) != NULL){
            cur->next = calloc(1,sizeof(*cur));
            assert(cur->next);
            cur = cur->next;
            cur->data = fcur->endpoint;
            cur->elen = fcur->elen;
        }
        cur = start;
        uint64_t outbuflen = sizeof(uint64_t)+sizeof(uint64_t)+(sizeof(uint64_t) * (rpcbuff->dimsizes_len)) + 1;
        while(cur){
            if(cur->data == (char*)0xCAFE) outbuflen++;
            else outbuflen += (sizeof(uint64_t) + cur->elen + 1);
            cur = cur->next;
        }
        cur = start;
        out = calloc(outbuflen, sizeof(char));
        assert(out);
        ret = out;
        char* servdata = ret;
        out += sizeof(uint64_t) + (sizeof(uint64_t) * (rpcbuff->dimsizes_len + 1));
        uint64_t dimsizeslen_be64 = cpu_to_be64(rpcbuff->dimsizes_len);
        uint64_t lastdim_len_be64 = cpu_to_be64(rpcbuff->lastdim_len);
        memcpy(servdata, &dimsizeslen_be64, sizeof(uint64_t));
        servdata += sizeof(uint64_t);
        memcpy(servdata, &lastdim_len_be64, sizeof(uint64_t));
        servdata += sizeof(uint64_t);
        for(uint64_t i = 0; i <rpcbuff->dimsizes_len; i++){
            uint64_t be64 = cpu_to_be64(rpcbuff->dimsizes[i]);
            memcpy(servdata, &be64, sizeof(uint64_t));
            servdata += sizeof(uint64_t);
         }
        while(cur){
            if(cur->data == (void*)0xCAFE) {*out = 'S';out++;}else{
                *out = 'N'; out++;
                uint64_t be64elen = cpu_to_be64(cur->elen);
                memcpy(out,&be64elen,sizeof(uint64_t));
                out += sizeof(uint64_t);
                memcpy(out, cur->data,cur->elen);
                out += cur->elen;
            }
            struct _prpcbuff_dim* next = cur->next;
            struct _prpcbuff_dim* freep = cur;
            free(freep);
            cur = next;
        }
        *buflen = outbuflen;
        tqueque_free(tque);
        tqueque_free(fque);
        return ret;
    }
    return NULL;
}
struct rpcbuff* buf_to_rpcbuff(char* buf){
    assert(buf);
    uint64_t dimsizeslen_be64; memcpy(&dimsizeslen_be64,buf,sizeof(uint64_t));
    buf += sizeof(uint64_t);
    uint64_t dimsizes_len = be64_to_cpu(dimsizeslen_be64);
    uint64_t lastdim_len_be64; memcpy(&lastdim_len_be64,buf,sizeof(uint64_t));
    buf += sizeof(uint64_t);
    uint64_t lastdim_len = be64_to_cpu(lastdim_len_be64);
    uint64_t dimdata[dimsizes_len];
    for(uint64_t i = 0; i < dimsizes_len; i++){
        uint64_t be64;memcpy(&be64, buf, sizeof(uint64_t));
        buf += sizeof(uint64_t);
        dimdata[i] = be64_to_cpu(be64);
    }
    struct rpcbuff* rpcbuff = rpcbuff_create(dimdata, dimsizes_len, lastdim_len);
    struct tqueque* tque = tqueque_create();
    struct tqueque* fque = tqueque_create();
    assert(tque != NULL && fque != NULL);
    if(rpcbuff->dimsizes_len == 0 || rpcbuff->dimsizes == NULL){
        tqueque_push(fque,rpcbuff->start,sizeof(struct __rpcbuff_el*),NULL);
    }
    tqueque_push(tque,rpcbuff->start,sizeof(struct __rpcbuff_el*),NULL);
    for(uint64_t i = 0; i < rpcbuff->dimsizes_len; i++){
        uint64_t iter = tqueque_get_tagamm(tque,NULL);
        for(uint64_t j = 0; j < iter; j++){
            struct __rpcbuff_el* cur = tqueque_pop(tque,NULL,NULL);
            for(uint64_t k = 0; k < rpcbuff->dimsizes[i]; k++){
                if(cur->childs[k].endpoint != NULL) tqueque_push(fque,&cur->childs[k],sizeof(struct __rpcbuff_el*),NULL);
                tqueque_push(tque,&cur->childs[k],sizeof(struct __rpcbuff_el*),NULL);
            }
        }
    }
    struct __rpcbuff_el* fcur = NULL;
    while((fcur = tqueque_pop(fque,NULL,NULL)) != NULL){
        if(*buf == 'S') {buf++;continue;}
        if(*buf == 'N'){
            buf++;
            uint64_t be64elen; memcpy(&be64elen,buf,sizeof(uint64_t));
            buf += sizeof(uint64_t);
            uint64_t elen = be64_to_cpu(be64elen);
            fcur->endpoint = calloc(elen, sizeof(char));
            fcur->elen = elen;
            memcpy(fcur->endpoint, buf, elen);
            buf += elen;
        }
    }
    tqueque_free(tque); tqueque_free(fque);
    return rpcbuff;
}

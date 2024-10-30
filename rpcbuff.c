#include <assert.h>
#include "rpcpack.h"
#include <stdint.h>
#include "lb_endian.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "tqueque/tqueque.h"
#include <stdint.h>
#include <string.h>
#include "rpctypes.h"

struct rpcbuff* rpcbuff_create(uint64_t* dimsizes,uint64_t dimsizes_len){
    struct rpcbuff_el* md_array = calloc(1,sizeof(struct rpcbuff_el));
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
    if(dimsizes != NULL && dimsizes_len > 0){
        struct tqueque* que = tqueque_create();
        assert(que);
        assert(tqueque_push(que,md_array,sizeof(struct rpcbuff_el*),NULL) == 0);
        for(uint64_t i = 0; i < dimsizes_len; i++){
            struct rpcbuff_el* cur = NULL;
            uint64_t iter = tqueque_get_tagamm(que,NULL);
            for(uint64_t j = 0; j < iter; j++){
                cur = tqueque_pop(que,NULL,NULL);
                cur->childs = calloc(dimsizes[i],sizeof(struct rpcbuff_el));
                assert(cur->childs);
                for(uint64_t k = 0; k <dimsizes[i]; k++){
                    assert(tqueque_push(que,&cur->childs[k], sizeof(struct rpcbuff_el*), NULL) == 0);
                }
            }
        }
        struct rpcbuff_el* cur = NULL;
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
void rpcbuff_free_internals(struct rpcbuff* rpcbuff){
    assert(rpcbuff);
    struct tqueque* tque = tqueque_create();
    struct tqueque* fque = tqueque_create();
    assert(tque != NULL && fque != NULL);
    tqueque_push(tque,rpcbuff->start,sizeof(struct rpcbuff_el*),NULL);
    for(uint64_t i = 0; i < rpcbuff->dimsizes_len; i++){
        uint64_t iter = tqueque_get_tagamm(tque,NULL);
        for(uint64_t j = 0; j < iter; j++){
            struct rpcbuff_el* cur = tqueque_pop(tque,NULL,NULL);
            tqueque_push(fque,cur->childs,sizeof(struct rpcbuff_el*),NULL);
            for(uint64_t k = 0; k < rpcbuff->dimsizes[i]; k++){
                if(cur->childs[k].endpoint != NULL)  tqueque_push(fque,cur->childs[k].endpoint,sizeof(struct rpcbuff_el*),NULL);
                tqueque_push(tque,&cur->childs[k],sizeof(struct rpcbuff_el*),NULL);
            }
        }
    }
    tqueque_free(tque);
    void* fcur = NULL;
    while((fcur = tqueque_pop(fque,NULL,NULL)) != NULL){
        if(fcur != (void*)0xCAFE) free(fcur);
    }
    if(rpcbuff->dimsizes == NULL) if(rpcbuff->start->endpoint != (void*)0xCAFE) free(rpcbuff->start->endpoint);
    tqueque_free(fque);
    free(rpcbuff->start);
    free(rpcbuff->dimsizes);
}
void rpcbuff_free(struct rpcbuff* rpcbuff){
    rpcbuff_free_internals(rpcbuff);
    free(rpcbuff);
}

struct rpcbuff_el* rpcbuff_el_getlast_from(struct  rpcbuff* rpcbuff, uint64_t* index, size_t index_len){
    assert(rpcbuff);
    assert(rpcbuff->start);
    if(rpcbuff->dimsizes_len != index_len) return NULL;
    if(rpcbuff->dimsizes_len != 0 && index == NULL) return NULL;
    struct rpcbuff_el* cur = rpcbuff->start;
    for(uint64_t i = 0; i < rpcbuff->dimsizes_len; i++){
        if(index[i] >= rpcbuff->dimsizes[i]) return NULL;
        cur = &cur->childs[index[i]];
    }
    if(cur == NULL) return NULL;
    return cur;
}

int rpcbuff_getlast_from(struct  rpcbuff* rpcbuff, uint64_t* index, size_t index_len,void* otype,uint64_t* otype_len,enum rpctypes type){
    assert(rpcbuff);
    struct rpcbuff_el* got = rpcbuff_el_getlast_from(rpcbuff,index,index_len);
    assert(got);
    if(got->endpoint == (void*)0xCAFE) {return 1;}
    if(got->type != type) return 1;
    struct rpctype ptype = {0};
    arr_to_type(got->endpoint,&ptype);
    if(type == CHAR){
        char ch = type_to_char(&ptype);
        free(ptype.data);
        *(char*)otype = ch;
        return 0;
    }
    if(type == UINT16){
        uint16_t ch = type_to_uint16(&ptype);
        free(ptype.data);
        *(uint16_t*)otype = ch;
        return 0;
    }
    if(type == INT16){
        int32_t ch = type_to_int32(&ptype);
        free(ptype.data);
        *(int32_t*)otype = ch;
        return 0;
    }
    if(type == UINT32){
        uint32_t ch = type_to_uint32(&ptype);
        free(ptype.data);
        *(uint32_t*)otype = ch;
        return 0;
    }
    if(type == INT32){
        int32_t ch = type_to_int32(&ptype);
        free(ptype.data);
        *(int32_t*)otype = ch;
        return 0;
    }
    if(type == UINT64){
        uint64_t ch = type_to_uint64(&ptype);
        free(ptype.data);
        *(uint64_t*)otype = ch;
        return 0;
    }
    if(type == INT64){
        int64_t ch = type_to_int64(&ptype);
        free(ptype.data);
        *(int64_t*)otype = ch;
        return 0;
    }
    if(type == FLOAT){
        float ch = type_to_float(&ptype);
        free(ptype.data);
        *(float*)otype = ch;
        return 0;
    }
    if(type == DOUBLE){
        double ch = type_to_double(&ptype);
        free(ptype.data);
        *(double*)otype = ch;
        return 0;
    }
    if(type == STR){
        char* ch = unpack_str_type(&ptype);
        char* imm = malloc(strlen(ch) + 1);
        assert(imm);
        strcpy(imm,ch);
        free(ptype.data);
        *(char**)otype = imm;
        return 0;
    }
    if(type == SIZEDBUF){
        uint64_t tmp = 0;
        uint64_t* len = NULL;
        if(otype_len != NULL) len = otype_len;
        else len = &tmp;
        void* ch = unpack_sizedbuf_type(&ptype,len);
        void* imm = malloc(*len);
        assert(imm);
        memcpy(imm,ch,*len);
        free(ptype.data);
        *(void**)otype = imm;
        return 0;
    }
    if(type == RPCBUFF){
        struct rpcbuff* ch = unpack_rpcbuff_type(&ptype);
        free(ptype.data);
        *(struct rpcbuff**)otype = ch;
        return 0;
    }
    if(type == RPCSTRUCT){
        struct rpcstruct* ch = unpack_rpcstruct_type(&ptype);
        free(ptype.data);
        *(struct rpcstruct**)otype = ch;
        return 0;
    }
    return 1;
}

int rpcbuff_pushto(struct rpcbuff* rpcbuff, uint64_t* index, size_t index_len, void* untype,uint64_t type_len,enum rpctypes type){
    assert(rpcbuff);
    struct rpcbuff_el* got = rpcbuff_el_getlast_from(rpcbuff,index,index_len);
    assert(got);
    if(got->endpoint != (void*)0xCAFE) free(got->endpoint);
    got->type = type;
    struct rpctype ptype = {0};
    if(type == CHAR){
        char_to_type(*(char*)untype,&ptype);
    }
    if(type == UINT16){
        uint16_to_type(*(uint16_t*)untype,&ptype);
    }
    if(type == INT16){
        int16_to_type(*(int16_t*)untype,&ptype);
    }
    if(type == UINT32){
        uint32_to_type(*(uint32_t*)untype,&ptype);
    }
    if(type == INT32){
        int32_to_type(*(int32_t*)untype,&ptype);
    }
    if(type == UINT64){
        uint64_to_type(*(uint64_t*)untype,&ptype);
    }
    if(type == INT64){
        int64_to_type(*(int64_t*)untype,&ptype);
    }
    if(type == FLOAT){
        float_to_type(*(float*)untype,&ptype);
    }
    if(type == DOUBLE){
        double_to_type(*(double*)untype,&ptype);
    }
    if(type == STR){
        create_str_type(untype,1,&ptype);
    }
    if(type == SIZEDBUF){
        create_sizedbuf_type(untype,type_len,1,&ptype);
    }
    if(type == RPCBUFF){
        create_rpcbuff_type(untype,1,&ptype);
    }
    if(type == RPCSTRUCT){
        create_rpcstruct_type(untype,1,&ptype);
    }
    got->elen = type_buflen(&ptype);
    got->endpoint = malloc(got->elen);
    assert(got->endpoint);
    type_to_arr(got->endpoint,&ptype);
    free(ptype.data);
    return 0;
}
struct _prpcbuff_dim{
    enum rpctypes type;
    char* data;
    uint64_t elen;
    struct _prpcbuff_dim* next;
};
char* rpcbuff_to_arr(struct rpcbuff* rpcbuff,uint64_t* buflen){
    assert(rpcbuff);
    struct tqueque* tque = tqueque_create();
    struct tqueque* fque = tqueque_create();
    assert(tque != NULL && fque != NULL);
    tqueque_push(tque,rpcbuff->start,sizeof(struct rpcbuff_el*),NULL);
    if(rpcbuff->dimsizes_len == 0 || rpcbuff->dimsizes == NULL){
        tqueque_push(fque,rpcbuff->start,sizeof(struct rpcbuff_el*),NULL);
    }
    for(uint64_t i = 0; i < rpcbuff->dimsizes_len; i++){
        uint64_t iter = tqueque_get_tagamm(tque,NULL);
        for(uint64_t j = 0; j < iter; j++){
            struct rpcbuff_el* cur = tqueque_pop(tque,NULL,NULL);
            for(uint64_t k = 0; k < rpcbuff->dimsizes[i]; k++){
                if(cur->childs[k].endpoint != NULL) tqueque_push(fque,&cur->childs[k],sizeof(struct rpcbuff_el*),NULL);
                tqueque_push(tque,&cur->childs[k],sizeof(struct rpcbuff_el*),NULL);
            }
        }
    }
    struct rpcbuff_el* fcur = NULL;
    char* out = NULL;
    char* ret = NULL;
    struct _prpcbuff_dim* start = calloc(1,sizeof(*start));
    assert(start);
    struct _prpcbuff_dim* cur = start;
    if((fcur = tqueque_pop(fque,NULL,NULL)) != NULL){
            cur->data = fcur->endpoint;
            cur->type = fcur->type;
            cur->elen = fcur->elen;
        while((fcur = tqueque_pop(fque,NULL,NULL)) != NULL){
            cur->next = calloc(1,sizeof(*cur));
            assert(cur->next);
            cur = cur->next;
            cur->data = fcur->endpoint;
            cur->type = fcur->type;
            cur->elen = fcur->elen;
        }
        cur = start;
        uint64_t outbuflen = sizeof(uint64_t)+(sizeof(uint64_t) * (rpcbuff->dimsizes_len)) + 1;
        while(cur){
            if(cur->data == (char*)0xCAFE) outbuflen++;
            else outbuflen += (sizeof(uint64_t) + cur->elen + 2);
            cur = cur->next;
        }
        cur = start;
        out = calloc(outbuflen, sizeof(char));
        assert(out);
        ret = out;
        uint64_t dimsizeslen_be64 = cpu_to_be64(rpcbuff->dimsizes_len);
        memcpy(out, &dimsizeslen_be64, sizeof(uint64_t));
        out += sizeof(uint64_t);
        for(uint64_t i = 0; i < rpcbuff->dimsizes_len; i++){
            uint64_t be64 = cpu_to_be64(rpcbuff->dimsizes[i]);
            memcpy(out, &be64, sizeof(uint64_t));
            out += sizeof(uint64_t);
         }
        out++;
        while(cur){
            if(cur->data == (void*)0xCAFE) {*out = 'S';out++;}else{
                *out = 'N'; out++;
                *out = cur->type; out++;
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
    uint64_t dimdata[dimsizes_len];
    for(uint64_t i = 0; i < dimsizes_len; i++){
        uint64_t be64;memcpy(&be64, buf, sizeof(uint64_t));
        buf += sizeof(uint64_t);
        dimdata[i] = be64_to_cpu(be64);
    }
    buf++;
    struct rpcbuff* rpcbuff = rpcbuff_create(dimdata, dimsizes_len);
    struct tqueque* tque = tqueque_create();
    struct tqueque* fque = tqueque_create();
    assert(tque != NULL && fque != NULL);
    if(rpcbuff->dimsizes_len == 0 || rpcbuff->dimsizes == NULL){
        tqueque_push(fque,rpcbuff->start,sizeof(struct rpcbuff_el*),NULL);
    }
    tqueque_push(tque,rpcbuff->start,sizeof(struct rpcbuff_el*),NULL);
    for(uint64_t i = 0; i < rpcbuff->dimsizes_len; i++){
        uint64_t iter = tqueque_get_tagamm(tque,NULL);
        for(uint64_t j = 0; j < iter; j++){
            struct rpcbuff_el* cur = tqueque_pop(tque,NULL,NULL);
            for(uint64_t k = 0; k < rpcbuff->dimsizes[i]; k++){
                if(cur->childs[k].endpoint != NULL) tqueque_push(fque,&cur->childs[k],sizeof(struct rpcbuff_el*),NULL);
                tqueque_push(tque,&cur->childs[k],sizeof(struct rpcbuff_el*),NULL);
            }
        }
    }
    struct rpcbuff_el* fcur = NULL;
    while((fcur = tqueque_pop(fque,NULL,NULL)) != NULL){
        if(*buf == 'S') {buf++;continue;}
        if(*buf == 'N'){
            buf++;
            fcur->type = *buf;buf++;
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


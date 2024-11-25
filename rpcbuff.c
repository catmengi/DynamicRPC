#include <assert.h>
#include "rpcpack.h"
#include "rpccall.h"
#include <stdint.h>
#include "lb_endian.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "tqueque/tqueque.h"
#include <stdint.h>
#include <string.h>
#include "rpctypes.h"

struct rpcbuff_el{
    char is_packed;
    struct rpcbuff_el* childs;
    char type;
    uint64_t elen;
    void* endpoint;
};

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
        tqueque_push(que,md_array);
        for(uint64_t i = 0; i < dimsizes_len; i++){
            struct rpcbuff_el* cur = NULL;
            uint64_t iter = tqueque_get_len(que);
            for(uint64_t j = 0; j < iter; j++){
                cur = tqueque_pop(que);
                cur->childs = calloc(dimsizes[i],sizeof(struct rpcbuff_el));
                assert(cur->childs);
                for(uint64_t k = 0; k <dimsizes[i]; k++){
                    tqueque_push(que,&cur->childs[k]);
                }
            }
        }
        struct rpcbuff_el* cur = NULL;
        while((cur = tqueque_pop(que)) != NULL){
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
    struct tqueque* eque = tqueque_create();
    assert(tque != NULL && fque != NULL && eque != NULL);
    tqueque_push(tque,rpcbuff->start);
    for(uint64_t i = 0; i < rpcbuff->dimsizes_len; i++){
        uint64_t iter = tqueque_get_len(tque);
        for(uint64_t j = 0; j < iter; j++){
            struct rpcbuff_el* cur = tqueque_pop(tque);
            tqueque_push(fque,cur->childs);
            for(uint64_t k = 0; k < rpcbuff->dimsizes[i]; k++){
                if(cur->childs[k].endpoint != NULL && cur->childs[k].endpoint != (void*)0xCAFE)
                    tqueque_push(eque,&cur->childs[k]);
                tqueque_push(tque,&cur->childs[k]);
            }
        }
    }
    tqueque_free(tque);
    struct rpcbuff_el* el = NULL;
    while((el = tqueque_pop(eque)) != NULL){
        if(el->is_packed == 1)
            free(el->endpoint);
        else{
            switch(el->type){
                case RPCBUFF:
                    rpcbuff_free(el->endpoint);
                    break;
                case RPCSTRUCT:
                    rpcstruct_free(el->endpoint);
                    break;
                default:
                    free(el->endpoint);
                    break;
            }
        }
    }
    void* fcur = NULL;
    while((fcur = tqueque_pop(fque)) != NULL){
        free(fcur);
    }
    if(rpcbuff->dimsizes == NULL) if(rpcbuff->start->endpoint != (void*)0xCAFE) free(rpcbuff->start->endpoint);
    tqueque_free(fque);
    tqueque_free(eque);
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

int rpcbuff_getfrom(struct  rpcbuff* rpcbuff, uint64_t* index, size_t index_len,void* otype,uint64_t* otype_len,enum rpctypes type){
    assert(rpcbuff);
    struct rpcbuff_el* got = rpcbuff_el_getlast_from(rpcbuff,index,index_len);
    if(got == NULL) return MISS_NONE;
    if(got->endpoint == (void*)0xCAFE) return MISS_NONE;
    if(got->type != type) return MISS_TYPE;
    struct rpctype ptype = {0};
    if(got->is_packed){
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
    }
    if(otype_len != NULL)
        *otype_len = got->elen;
    *(void**)otype = got->endpoint;
    return 0;
}

int rpcbuff_pushto(struct rpcbuff* rpcbuff, uint64_t* index, size_t index_len, void* untype,uint64_t type_len,enum rpctypes type){
    assert(rpcbuff);
    if(untype == NULL) return 1;
    struct rpcbuff_el* got = rpcbuff_el_getlast_from(rpcbuff,index,index_len);
    if(got == NULL) return MISS_NONE;
    if(got->endpoint != (void*)0xCAFE){
        if(got->is_packed == 1)
            free(got->endpoint);
        else{
            switch(got->type){
                case RPCBUFF:
                    rpcbuff_free(got->endpoint);
                    break;
                case RPCSTRUCT:
                    rpcstruct_free(got->endpoint);
                    break;
                default:
                    free(got->endpoint);
                    break;
            }
        }
    }
    got->type = type;
    got->is_packed = 1;
    struct rpctype ptype = {0};
    switch(type){
        case CHAR:
            char_to_type(*(char*)untype,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case UINT16:
            uint16_to_type(*(uint16_t*)untype,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case INT16:
            int16_to_type(*(int16_t*)untype,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case UINT32:
            uint32_to_type(*(uint32_t*)untype,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case INT32:
            int32_to_type(*(int32_t*)untype,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case UINT64:
            uint64_to_type(*(uint64_t*)untype,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case INT64:
            int64_to_type(*(int64_t*)untype,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case FLOAT:
            float_to_type(*(float*)untype,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case DOUBLE:
            double_to_type(*(double*)untype,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case STR:
            got->is_packed = 0;
            got->endpoint = strdup(untype);
            break;
        case SIZEDBUF:
            got->is_packed = 0;
            got->elen = type_len;
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            memcpy(got->endpoint,untype,got->elen);
            break;
        default:
            got->endpoint = untype;
            got->elen = type_len;
            got->type = type;
            got->is_packed = 0;
            return 0;
    }
    return 0;
}

void rpcbuff_remove(struct rpcbuff* rpcbuff, uint64_t* index, size_t index_len){
    assert(rpcbuff);
    struct rpcbuff_el* got = rpcbuff_el_getlast_from(rpcbuff,index,index_len);
    if(!got) return;
    if(got->endpoint != (void*)0xCAFE) {
        if(got->is_packed == 1){
            free(got->endpoint);
            got->endpoint = (void*)0xCAFE;
        }else{
            switch(got->type){
                case RPCBUFF:
                    rpcbuff_free(got->endpoint);
                    got->endpoint = (void*)0xCAFE;
                    break;
                case RPCSTRUCT:
                    rpcstruct_free(got->endpoint);
                    got->endpoint = (void*)0xCAFE;
                    break;
                default:
                    free(got->endpoint);
                    got->endpoint = (void*)0xCAFE;
                    break;
            }
        }
    }
}

void rpcbuff_unlink(struct rpcbuff* rpcbuff, uint64_t* index, size_t index_len){
    assert(rpcbuff);
    struct rpcbuff_el* got = rpcbuff_el_getlast_from(rpcbuff,index,index_len);
    if(!got) return;
    if(got->is_packed == 0)
        got->endpoint = (void*)0xCAFE;
    return;
}

char* rpcbuff_to_buf(struct rpcbuff* rpcbuff,uint64_t* buflen){
    assert(rpcbuff);
    struct tqueque* tque = tqueque_create();
    struct tqueque* fque = tqueque_create();
    assert(tque != NULL && fque != NULL);
    tqueque_push(tque,rpcbuff->start);
    if(rpcbuff->dimsizes_len == 0 || rpcbuff->dimsizes == NULL){
        tqueque_push(fque,rpcbuff->start);
    }
    for(uint64_t i = 0; i < rpcbuff->dimsizes_len; i++){
        uint64_t iter = tqueque_get_len(tque);
        for(uint64_t j = 0; j < iter; j++){
            struct rpcbuff_el* cur = tqueque_pop(tque);
            for(uint64_t k = 0; k < rpcbuff->dimsizes[i]; k++){
                if(cur->childs[k].endpoint != NULL) tqueque_push(fque,&cur->childs[k]);
                tqueque_push(tque,&cur->childs[k]);
            }
        }
    }
    tqueque_free(tque);
    uint64_t outbuflen = sizeof(uint64_t) + (sizeof(uint64_t) * rpcbuff->dimsizes_len) + 1;
    uint64_t types_amm = tqueque_get_len(fque);
    struct rpctype * otypes = calloc(types_amm,sizeof(*otypes));
    for(uint64_t i = 0; i < types_amm; i++){
        struct rpcbuff_el* el = tqueque_pop(fque);
        memset(&otypes[i],0,sizeof(otypes[i]));
        if(el->endpoint != (void*)0xCAFE){
            if(el->is_packed){
                char* tmp = malloc(el->elen);
                assert(tmp);
                memcpy(tmp,el->endpoint,el->elen);
                otypes[i].data = tmp;
                otypes[i].datalen = cpu_to_be64(el->elen);
                printf("%lu, %lu\n",cpu_to_be64(otypes[i].datalen),el->elen);
                otypes[i].flag = 1;
                otypes[i].type = el->type;
            }else{
                switch(el->type){
                    case RPCBUFF:
                        create_rpcbuff_type(el->endpoint,1,&otypes[i]);
                        break;
                    case RPCSTRUCT:
                        create_rpcstruct_type(el->endpoint,1,&otypes[i]);
                        break;
                    case STR:
                        create_str_type(el->endpoint,1,&otypes[i]);
                        break;
                    case SIZEDBUF:
                        create_sizedbuf_type(el->endpoint,el->elen,1,&otypes[i]);
                        break;
                    default:
                        break;
                }
            }
        }
    }
    outbuflen += rpctypes_get_buflen(otypes,types_amm);
    *buflen = outbuflen;
    char* out = calloc(outbuflen,sizeof(char));
    char* ret = out;
    assert(out);
    uint64_t dimsizeslen_be64 = cpu_to_be64(rpcbuff->dimsizes_len);
    memcpy(out, &dimsizeslen_be64, sizeof(uint64_t));
    out += sizeof(uint64_t);
    for(uint64_t i = 0; i < rpcbuff->dimsizes_len; i++){
        uint64_t be64 = cpu_to_be64(rpcbuff->dimsizes[i]);
        memcpy(out, &be64, sizeof(uint64_t));
        out += sizeof(uint64_t);
    }
    out++;
    rpctypes_to_buf(otypes,types_amm,out);
    rpctypes_free(otypes,types_amm);
    tqueque_free(fque);
    return ret;
}
struct rpcbuff* buf_to_rpcbuff(char* buf){
    assert(buf);
    uint64_t dimsizeslen_be64;
    memcpy(&dimsizeslen_be64,buf,sizeof(uint64_t));
    buf += sizeof(uint64_t);
    uint64_t dimsizes_len = be64_to_cpu(dimsizeslen_be64);
    uint64_t* dimdata = calloc(dimsizes_len,sizeof(uint64_t));
    assert(dimdata);
    if(dimsizes_len > 0){
        for(uint64_t i = 0; i < dimsizes_len; i++){
            uint64_t be64;memcpy(&be64, buf, sizeof(uint64_t));
            buf += sizeof(uint64_t);
            dimdata[i] = be64_to_cpu(be64);
        }
    }
    buf++;
    struct rpcbuff* rpcbuff = rpcbuff_create(dimdata, dimsizes_len);
    free(dimdata);
    struct tqueque* tque = tqueque_create();
    struct tqueque* fque = tqueque_create();
    assert(tque != NULL && fque != NULL);
    if(rpcbuff->dimsizes_len == 0 || rpcbuff->dimsizes == NULL){
        tqueque_push(fque,rpcbuff->start);
    }
    tqueque_push(tque,rpcbuff->start);
    for(uint64_t i = 0; i < rpcbuff->dimsizes_len; i++){
        uint64_t iter = tqueque_get_len(tque);
        for(uint64_t j = 0; j < iter; j++){
            struct rpcbuff_el* cur = tqueque_pop(tque);
            for(uint64_t k = 0; k < rpcbuff->dimsizes[i]; k++){
                if(cur->childs[k].endpoint != NULL) tqueque_push(fque,&cur->childs[k]);
                tqueque_push(tque,&cur->childs[k]);
            }
        }
    }
    tqueque_free(tque);
    uint64_t types_amm = 0;
    struct rpctype* ptypes = buf_to_rpctypes(buf,&types_amm);
    assert(types_amm == tqueque_get_len(fque));
    for(uint64_t i = 0; i < types_amm; i++){
        struct rpcbuff_el* el = tqueque_pop(fque);
        el->type = ptypes[i].type;
        if(el->type == VOID) continue;
        if(el->type != RPCBUFF && el->type != RPCSTRUCT && el->type != STR && el->type != SIZEDBUF){
            el->is_packed = 1;
            uint64_t cpylen = be64_to_cpu(ptypes[i].datalen);
            el->endpoint = malloc(cpylen);
            el->elen = cpylen;
            assert(el->endpoint);
            memcpy(el->endpoint,ptypes[i].data,cpylen);
        }else{
            el->is_packed = 0;
            switch(el->type){
                case RPCBUFF:
                    el->endpoint = unpack_rpcbuff_type(&ptypes[i]);
                    break;
                case RPCSTRUCT:
                    el->endpoint = unpack_rpcstruct_type(&ptypes[i]);
                    break;
                case STR:
                    el->endpoint = unpack_str_type(&ptypes[i]);
                    ptypes[i].data = NULL;
                    break;
                case SIZEDBUF:
                    el->endpoint = unpack_sizedbuf_type(&ptypes[i],&el->elen);
                    ptypes[i].data = NULL;
                    break;
                default:
                    break;
            }
        }
    }
    rpctypes_free(ptypes,types_amm);
    tqueque_free(fque);
    return rpcbuff;
}

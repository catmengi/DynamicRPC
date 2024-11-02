#include "hashtable.c/hashtable.h"
#include "rpcpack.h"
#include "rpccall.h"
#include <assert.h>
#include "lb_endian.h"
#include "rpctypes.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int __rpcstruct_create(struct rpcstruct* rpcstruct){
    rpcstruct->count = 0;
    return hashtable_create(&rpcstruct->ht,4,4);
}
struct rpcstruct* rpcstruct_create(){
    struct rpcstruct* s = calloc(1,sizeof(*s));
    assert(s);
    if(__rpcstruct_create(s) != 0){
        free(s);
        return NULL;
    }
    return s;
}

struct rpcstruct_el{
    char is_packed;
    enum rpctypes type;
    uint64_t elen;
    void* endpoint;
};

int rpcstruct_set(struct rpcstruct* rpcstruct,char* key,void* arg,uint64_t typelen,enum rpctypes type){
    struct rpcstruct_el* got = NULL;
    int r = hashtable_get(rpcstruct->ht,key,strlen(key) + 1,(void**)&got);
    if(r == ENOTFOUND){
        got = calloc(1,sizeof(*got));
        assert(got);
    }else if(r == 0 && got != NULL){
        if(got->is_packed == 1){
            free(got->endpoint);
        }else{
            switch(got->type){
                case RPCSTRUCT:
                    rpcstruct_free(got->endpoint);
                    break;
                case RPCBUFF:
                    rpcbuff_free(got->endpoint);
                    break;
                default:
                    break;
            }
        }
    }
    got->type = type;
    got->is_packed = 1;
    struct rpctype ptype = {0};
    switch(type){
        case CHAR:
            char_to_type(*(char*)arg,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case UINT16:
            uint16_to_type(*(uint16_t*)arg,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case INT16:
            int16_to_type(*(int16_t*)arg,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case UINT32:
            uint32_to_type(*(uint32_t*)arg,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case INT32:
            int32_to_type(*(int32_t*)arg,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case UINT64:
            uint64_to_type(*(uint64_t*)arg,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case INT64:
            int64_to_type(*(int64_t*)arg,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case FLOAT:
            float_to_type(*(float*)arg,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case DOUBLE:
            double_to_type(*(double*)arg,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case STR:
            create_str_type(arg,1,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        case SIZEDBUF:
            create_sizedbuf_type(arg,typelen,1,&ptype);
            got->elen = type_buflen(&ptype);
            got->endpoint = malloc(got->elen);
            assert(got->endpoint);
            type_to_arr(got->endpoint,&ptype);
            free(ptype.data);
            break;
        default:
            got->endpoint = arg;
            got->type = type;
            got->is_packed = 0;
            break;
    }
    assert(hashtable_add(rpcstruct->ht,key,strlen(key) + 1,got,0) == 0);
    rpcstruct->count++;
    return 0;
}
int rpcstruct_get(struct rpcstruct* rpcstruct,char* key,void* otype,uint64_t* otype_len,enum rpctypes type){
    struct rpcstruct_el* got = NULL;
    if(hashtable_get(rpcstruct->ht,key,strlen(key) + 1,(void**)&got) != 0) return 1;
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
        if(type == STR){
            char* ch = unpack_str_type(&ptype);
            *(char**)otype = ch;
            return 0;
        }
        if(type == SIZEDBUF){
            uint64_t tmp = 0;
            uint64_t* len = NULL;
            if(otype_len != NULL) len = otype_len;
            else len = &tmp;
            void* ch = unpack_sizedbuf_type(&ptype,len);
            *(void**)otype = ch;
            return 0;
        }
    }
    if(got->type != type) return 1;
    if(otype_len != NULL)
        *otype_len = got->elen;
    *(void**)otype = got->endpoint;
    return 0;
}
void _rpcstruct_pack_callback(char* key,void* ptype,void* vtype,size_t iter){
    // struct rpctype* type = vtype;
    // char* packed = calloc(strlen(key) + 1 +  type_buflen(ptype),sizeof(char));
    // assert(packed);
    // char* org = packed;
    // strcpy(packed,key);
    // packed += strlen(key) + 1;
    // type_to_arr(packed,ptype);
    // struct rpctype* rtype = ptype;
    // type[iter].type = _RPCSTRUCTEL;
    // type[iter].flag = rtype->flag;
    // type[iter].data = org;
    // type[iter].datalen = cpu_to_be64(strlen(key) + 1 +  type_buflen(ptype));
    struct rpcstruct_el* el = ptype;
    struct rpctype* types = vtype;
    struct rpctype ready = {0};
    if(el->is_packed == 1){
        arr_to_type(el->endpoint,&ready);
    }else{
        switch(el->type){
            case RPCSTRUCT:
                create_rpcstruct_type(el->endpoint,1,&ready);
                break;
            case RPCBUFF:
                create_rpcbuff_type(el->endpoint,1,&ready);
                break;
            default:
                break;
        }
    }
    char* packed = calloc(strlen(key) + 1 + type_buflen(&ready),sizeof(char));
    assert(packed);
    char* org = packed;
    memcpy(packed,key,strlen(key) + 1);
    packed += strlen(key) + 1;
    type_to_arr(packed,&ready);
    types[iter].type = el->type;
    types[iter].flag = ready.flag;
    types[iter].data = org;
    types[iter].datalen = cpu_to_be64(strlen(key) + 1 + type_buflen(&ready));
    free(ready.data);
}
char* rpcstruct_to_buf(struct rpcstruct* rpcstruct, uint64_t* buflen){
    assert(buflen);
    struct rpctype* types = calloc(rpcstruct->count,sizeof(*types));
    assert(types);
    hashtable_iterate_wkey(rpcstruct->ht,(void*)types,_rpcstruct_pack_callback);
    uint64_t rpcbuflen = rpctypes_get_buflen(types,rpcstruct->count);
    char* ret = malloc(rpcbuflen); *buflen = rpcbuflen;
    assert(ret);
    rpctypes_to_buf(types,rpcstruct->count,ret);
    rpctypes_free(types,rpcstruct->count);
    return ret;
}
int buf_to_rpcstruct(char* arr, struct rpcstruct* rpcstruct){
    assert(rpcstruct);
    uint64_t count = 0;
    struct rpctype* types = buf_to_rpctypes(arr,&count);
    if(!types) return 1;
    rpcstruct->count = count;
    assert(hashtable_create(&rpcstruct->ht,4,4) == 0);
    for(uint64_t i = 0; i < count; i++){
        char* org = types[i].data;
        char* tmp = org + strlen(org) + 1;
        char* uptype = malloc(be64_to_cpu(types[i].datalen) - strlen(org) - 1);
        assert(uptype);
        memcpy(uptype,tmp,be64_to_cpu(types[i].datalen) - strlen(org) - 1);
        struct rpcstruct_el* el = calloc(1,sizeof(*el));
        assert(el);
        if(types[i].type != RPCBUFF && types[i].type != RPCSTRUCT){
            el->is_packed = 1;
            el->type = types[i].type;
            el->endpoint = uptype;
        }else{
            el->is_packed = 0;
            el->type = types[i].type;
            struct rpctype up_uptype = {0};
            arr_to_type(uptype,&up_uptype);
            el->elen = be64_to_cpu(up_uptype.datalen);
            switch(el->type){
                case RPCSTRUCT:
                    el->endpoint = unpack_rpcstruct_type(&up_uptype);
                    break;
                case RPCBUFF:
                    el->endpoint = unpack_rpcbuff_type(&up_uptype);
                    break;
                default:
                    break;
            }
            free(uptype);
            free(up_uptype.data);
        }
        hashtable_add(rpcstruct->ht,org,strlen(org) + 1,el,0);
    }
    rpctypes_free(types,count);
    return 0;
}
int rpcstruct_remove(struct rpcstruct* rpcstruct, char* key){
    struct rpcstruct_el* el = NULL;
    if(hashtable_get(rpcstruct->ht,key,strlen(key) + 1,(void**)&el) != 0) return 1;
    if(el->is_packed == 1)
        free(el->endpoint);
    else{
        switch(el->type){
            case RPCSTRUCT:
                rpcstruct_free(el->endpoint);
                break;
            case RPCBUFF:
                rpcbuff_free(el->endpoint);
                break;
            default:
                break;
        }
    }
    free(el);
    hashtable_remove_entry(rpcstruct->ht,key,strlen(key) + 1);
    rpcstruct->count--;
    return 0;
}
void __rpcstruct_get_fields_cb(char* key,void* vptr,void* usr,size_t iter){
    char** fields = usr;
    fields[iter] = key;
}
char** rpcstruct_get_fields(struct rpcstruct* rpcstruct, size_t* fields_len){
    assert(rpcstruct); assert(fields_len);
    if(rpcstruct->count == 0){*fields_len = 0;return NULL;}
    char** fields = calloc(rpcstruct->count, sizeof(char*));
    assert(fields);
    hashtable_iterate_wkey(rpcstruct->ht,fields,__rpcstruct_get_fields_cb);
    *fields_len = rpcstruct->count;
    return fields;
}
void __rpcstruct_free_cb(void* vptr){
    struct rpcstruct_el* el = vptr;
    if(el->is_packed == 1)
        free(el->endpoint);
    else{
        switch(el->type){
            case RPCSTRUCT:
                rpcstruct_free(el->endpoint);
                break;
            case RPCBUFF:
                rpcbuff_free(el->endpoint);
                break;
            default:
                break;
        }
    }
    free(el);
}
void rpcstruct_free_internals(struct rpcstruct* rpcstruct){
    if(rpcstruct->count > 0) hashtable_iterate(rpcstruct->ht,__rpcstruct_free_cb);
    hashtable_free(rpcstruct->ht);
    rpcstruct->ht = 0;
    rpcstruct->count = 0;
}
void rpcstruct_free(struct rpcstruct* rpcstruct){
    rpcstruct_free_internals(rpcstruct);
    free(rpcstruct);
}

#include "rpcpack.h"
#include "rpccall.h"
#include <assert.h>
#include "lb_endian.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int rpcstruct_create(struct rpcstruct* rpcstruct){
    rpcstruct->count = 0;
    return hashtable_create(&rpcstruct->ht,4,4);
}
int rpcstruct_set(struct rpcstruct* rpcstruct,char* key,enum rpctypes type, void* arg,size_t typelen){
    struct rpctype* check = NULL;
    struct rpctype* ptype = NULL;
    if(hashtable_get(rpcstruct->ht,key,strlen(key) + 1,(void**)&check) != 0){
        ptype = malloc(sizeof(*ptype)); assert(ptype);
        rpcstruct->count++;
    }else{ptype = check; free(ptype->data);}
    if(type == CHAR){
        char_to_type(*(char*)arg,ptype);
    }
    if(type == UINT16){
        uint16_to_type(*(uint16_t*)arg,ptype);
    }
    if(type == INT16){
        int16_to_type(*(int16_t*)arg,ptype);
    }
    if(type == UINT32){
        uint32_to_type(*(uint32_t*)arg,ptype);
    }
    if(type == INT32){
        int32_to_type(*(int32_t*)arg,ptype);
    }
    if(type == UINT64){
        uint64_to_type(*(uint64_t*)arg,ptype);
    }
    if(type == INT64){
        int64_to_type(*(int64_t*)arg,ptype);
    }
    if(type == FLOAT){
        float_to_type(*(float*)arg,ptype);
    }
    if(type == DOUBLE){
        double_to_type(*(double*)arg,ptype);
    }
    if(type == STR){
        create_str_type(arg,1,ptype);
    }
    if(type == SIZEDBUF){
        create_sizedbuf_type(arg,typelen,1,ptype);
    }
    if(type == RPCBUFF){
        create_rpcbuff_type(arg,1,ptype);
    }
    if(type == RPCSTRUCT){
        create_rpcstruct_type(arg,1,ptype);
    }
    ptype->flag = 1;
    assert(hashtable_add(rpcstruct->ht,key,strlen(key) + 1,ptype,0) == 0);
    return 0;
}
int rpcstruct_get(struct rpcstruct* rpcstruct,char* key,enum rpctypes type,void* out,uint64_t* obuflen){
    struct rpctype* ptype = NULL;
    if(hashtable_get(rpcstruct->ht,key,strlen(key) + 1,(void**)&ptype) != 0) return 1;
    if(ptype->type != type) return 2;
    if(type == CHAR){
        char ch = type_to_char(ptype);
        *(char*)out = ch;
        return 0;
    }
    if(type == UINT16){
        uint16_t ch = type_to_uint16(ptype);
        *(uint16_t*)out = ch;
        return 0;
    }
    if(type == INT16){
        int32_t ch = type_to_int32(ptype);
        *(int32_t*)out = ch;
        return 0;
    }
    if(type == UINT32){
        uint32_t ch = type_to_uint32(ptype);
        *(uint32_t*)out = ch;
        return 0;
    }
    if(type == INT32){
        int32_t ch = type_to_int32(ptype);
        *(int32_t*)out = ch;
        return 0;
    }
    if(type == UINT64){
        uint64_t ch = type_to_uint64(ptype);
        *(uint64_t*)out = ch;
        return 0;
    }
    if(type == INT64){
        int64_t ch = type_to_int64(ptype);
        *(int64_t*)out = ch;
        return 0;
    }
    if(type == FLOAT){
        float ch = type_to_float(ptype);
        *(float*)out = ch;
        return 0;
    }
    if(type == DOUBLE){
        double ch = type_to_double(ptype);
        *(double*)out = ch;
        return 0;
    }
    if(type == STR){
        char* ch = unpack_str_type(ptype);
        char* imm = malloc(strlen(ch) + 1);
        assert(imm);
        strcpy(imm,ch);
        *(char**)out = imm;
        return 0;
    }
    if(type == SIZEDBUF){
        uint64_t tmp = 0;
        uint64_t* len = NULL;
        if(obuflen != NULL) len = obuflen;
        else len = &tmp;
        void* ch = unpack_sizedbuf_type(ptype,len);
        void* imm = malloc(*len);
        assert(imm);
        memcpy(imm,ch,*len);
        *(void**)out = imm;
        return 0;
    }
    if(type == RPCBUFF){
        struct rpcbuff* ch = unpack_rpcbuff_type(ptype);
        *(struct rpcbuff**)out = ch;
        return 0;
    }
    if(type == RPCSTRUCT){
        struct rpcstruct* ch = unpack_rpcstruct_type(ptype);
        *(struct rpcstruct**)out = ch;
        return 0;
    }

    return 2;
}
void _rpcstruct_pack_callback(char* key,void* ptype,void* vtype,size_t iter){
    struct rpctype* type = vtype;
    char* packed = calloc(strlen(key) + 1 +  type_buflen(ptype),sizeof(char));
    assert(packed);
    char* org = packed;
    strcpy(packed,key);
    packed += strlen(key) + 1;
    type_to_arr(packed,ptype);
    struct rpctype* rtype = ptype;
    type[iter].type = _RPCSTRUCTEL;
    type[iter].flag = rtype->flag;
    type[iter].data = org;
    type[iter].datalen = cpu_to_be64(strlen(key) + 1 +  type_buflen(ptype));
}
char* rpcstruct_to_buf(struct rpcstruct* rpcstruct, uint64_t* buflen){
    assert(buflen);
    struct rpctype* types = calloc(rpcstruct->count,sizeof(*types));
    assert(types);
    hashtable_iterate_wkey(rpcstruct->ht,(void*)types,_rpcstruct_pack_callback);
    uint8_t newcount = 0;
    types = rpctypes_clean_nonres_args(types,rpcstruct->count,&newcount);
    rpcstruct->count = newcount;
    uint64_t rpcbuflen = rpctypes_get_buflen(types,rpcstruct->count);
    char* packed = calloc(rpcbuflen + sizeof(uint64_t) + 1,sizeof(char));
    assert(packed);
    uint64_t be64_rpcbuflen = cpu_to_be64(rpcbuflen);
    memcpy(packed,&be64_rpcbuflen,sizeof(uint64_t));
    packed += sizeof(uint64_t);
    assert(rpctypes_to_buf(types,rpcstruct->count,packed) == 0);
    rpctypes_free(types,rpcstruct->count);
    *buflen = rpcbuflen;
    return packed;
}
int buf_to_rpcstruct(char* arr, struct rpcstruct* rpcstruct){
    assert(rpcstruct);
    rpcstruct->count = 0;
    rpcstruct->ht = NULL;
    uint8_t count = 0;
    uint64_t checklen = 0;
    memcpy(&checklen,arr,sizeof(uint64_t));
    checklen = be64_to_cpu(checklen);
    struct rpctype* types = buf_to_rpctypes(arr,&count,checklen);
    if(!types) return 1;
    rpcstruct->count = count;
    assert(hashtable_create(&rpcstruct->ht,4,4) == 0);
    for(uint8_t i = 0; i < count; i++){
        char* org = types[i].data;
        char* type = org + strlen(org) + 1;
        struct rpctype* utype = malloc(sizeof(*utype));
        assert(utype);
        arr_to_type(type,utype);
        assert(hashtable_add(rpcstruct->ht,org,strlen(org) + 1,utype,0) == 0);
        free(org);
    }
    free(types);
    return 0;
}
int rpcstruct_set_flag(struct rpcstruct* rpcstruct, char* key, char flag){
    struct rpctype* chflag = NULL;
    if(hashtable_get(rpcstruct->ht,key,strlen(key) + 1,(void**)&chflag) != 0) return 2;
    chflag->flag = flag;
    return 0;
}
int rpcstruct_remove(struct rpcstruct* rpcstruct, char* key){
    struct rpctype* rm = NULL;
    if(hashtable_get(rpcstruct->ht,key,strlen(key) + 1,(void**)&rm) != 0) return 2;
    free(rm->data);
    free(rm);
    hashtable_remove_entry(rpcstruct->ht,key,strlen(key) + 1);
    rpcstruct->count--;
    return 0;
}
void __rpcstruct_get_fields_cb(char* key,void* vptr,void* usr,size_t iter){
    char** fields = usr;
    fields[iter] = key;
}
char** rpcstruct_get_fields(struct rpcstruct* rpcstruct, uint64_t* fields_len){
    assert(rpcstruct); assert(fields_len);
    if(rpcstruct->count == 0){*fields_len = 0;return NULL;}
    char** fields = calloc(rpcstruct->count, sizeof(char*));
    assert(fields);
    hashtable_iterate_wkey(rpcstruct->ht,fields,__rpcstruct_get_fields_cb);
    *fields_len = rpcstruct->count;
    return fields;
}
void __rpcstruct_free_cb(void* vptr){
    struct rpctype* type = vptr;
    free(type->data);
    free(type);
}
void rpcstruct_free(struct rpcstruct* rpcstruct){
    if(rpcstruct->count > 0) hashtable_iterate(rpcstruct->ht,__rpcstruct_free_cb);
    hashtable_free(rpcstruct->ht);
    rpcstruct->ht = 0;
    rpcstruct->count = 0;
}

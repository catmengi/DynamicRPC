#include "rpccall.h"
#include "rpcpack.h"
#include <stddef.h>
#include <stdio.h>
#include "rpctypes.h"
#include "tqueque/tqueque.h"
#include <string.h>
#include "lb_endian.h"
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>


int is_rpctypes_equal(enum rpctypes* serv, uint64_t servlen, enum rpctypes* client, uint64_t clientlen){
    if(!serv && !client) return 1;
    struct tqueque* check_que = tqueque_create();
    assert(check_que);
    size_t newservlen = 0;
    for(size_t i = 0; i < servlen;i++){
        if(serv[i] != INTERFUNC && serv[i] != PSTORAGE && serv[i] != FINGERPRINT){
            tqueque_push(check_que,&serv[i]);
            if(serv[i] == SIZEDBUF) i++;
            newservlen++;
        }
    }
    if((serv && !client) || (!serv && client)) {
        if(clientlen == 0 && newservlen == 0){
            tqueque_free(check_que);
            return 1;
        }
        tqueque_free(check_que);
        return 0;
    }
    if(newservlen != clientlen) {tqueque_free(check_que);return 0;}
    enum rpctypes* newserv = calloc(newservlen,sizeof(enum rpctypes));
    assert(newserv);
    for(size_t i = 0; i < newservlen; i++){
        newserv[i] = *(enum rpctypes*)tqueque_pop(check_que);
    }
    int ret = 1;
    for(size_t i = 0; i < clientlen; i++)
        if(newserv[i] != client[i]) {ret = 0;break;}
    tqueque_free(check_que);
    free(newserv);
    return ret;
}
uint64_t rpctypes_get_buflen(struct rpctype* rpctypes,uint64_t rpctypes_len){
    size_t len = sizeof(uint64_t) + 10; //ummm i dont know but it fixes the issue
    if(rpctypes == NULL) return len;
    for(uint64_t i = 0; i < rpctypes_len; i++){
        len += type_buflen(&rpctypes[i]);
    }
    return len;
}
void rpctypes_to_buf(struct rpctype* rpctypes,uint64_t rpctypes_amm, char* out){
    assert(out);
    memset(out,0,rpctypes_get_buflen(rpctypes,rpctypes_amm));
    uint64_t be64_rpctypes_amm = cpu_to_be64(rpctypes_amm);
    memcpy(out,&be64_rpctypes_amm,sizeof(uint64_t));
    out += sizeof(uint64_t) ;
    for(uint64_t i = 0; i < rpctypes_amm; i++){
        out += type_to_arr(out,&rpctypes[i]);
    }
    return;
}
struct rpctype* buf_to_rpctypes(char* in,uint64_t* rpctypes_amm){
    assert(in);
    uint64_t be64_rpctypes_amm = 0;
    memcpy(&be64_rpctypes_amm,in,sizeof(uint64_t)); in += sizeof(uint64_t);
    *rpctypes_amm = be64_to_cpu(be64_rpctypes_amm);
    struct rpctype* types = calloc(*rpctypes_amm,sizeof(struct rpctype));
    for(uint64_t i = 0; i < *rpctypes_amm; i++){
        arr_to_type(in,&types[i]);
        in += type_buflen(&types[i]);
    }
    return types;
}
enum rpctypes* rpctypes_get_types(struct rpctype* rpctypes,uint64_t rpctypes_amm){
    if(rpctypes == NULL) return NULL;
    enum rpctypes* erpctypes = malloc(rpctypes_amm * sizeof(enum rpctypes));
    for(uint64_t i = 0; i < rpctypes_amm; i++){
        erpctypes[i] = rpctypes[i].type;
    }
    return erpctypes;
}
void rpctypes_free(struct rpctype* rpctypes, uint64_t rpctypes_amm){
    if(rpctypes == NULL) return;
    for(uint64_t i = 0; i <rpctypes_amm; i++) free(rpctypes[i].data);
    free(rpctypes);
}

char* rpccall_to_buf(struct rpccall* rpccall, uint64_t* buflen){
    struct rpcstruct* call_struct = rpcstruct_create();

    assert(rpcstruct_set(call_struct,"fn",rpccall->fn_name,0,STR) == 0);

    uint64_t args_buflen = rpctypes_get_buflen(rpccall->args,rpccall->args_amm);
    char* args_buf = malloc(args_buflen); assert(args_buf);
    rpctypes_to_buf(rpccall->args,rpccall->args_amm,args_buf);

    assert(rpcstruct_set(call_struct,"args",args_buf,args_buflen,SIZEDBUF) == 0);
    free(args_buf);

    char* ret = rpcstruct_to_buf(call_struct,buflen);
    rpcstruct_free(call_struct);

    return ret;
}
void buf_to_rpccall(struct rpccall* rpccall,char* in){
    struct rpcstruct call_structC = {};
    struct rpcstruct* call_struct = &call_structC;
    buf_to_rpcstruct(in,call_struct);

    assert(rpcstruct_get(call_struct,"fn",&rpccall->fn_name,NULL,STR) == 0);
    rpcstruct_unlink(call_struct,"fn");

    char* args_buf = NULL;
    uint64_t args_buflen = 0;
    assert(rpcstruct_get(call_struct,"args",&args_buf,&args_buflen,SIZEDBUF) == 0);

    rpccall->args = buf_to_rpctypes(args_buf,&rpccall->args_amm);

    rpcstruct_free_internals(call_struct);

    return;
}
struct rpctype* rpctypes_clean_nonres_args(struct rpctype* rpctypes, uint64_t rpctypes_len,uint64_t* retargsamm_out){
    struct rpctype* resargs = NULL;
    uint64_t resargs_len = 0;
    for(uint64_t i = 0; i < rpctypes_len; i++){
        if(rpctypes[i].flag == 1) resargs_len++;
    }
    if(resargs_len > 0){
        resargs = malloc(resargs_len * sizeof(*resargs));
        assert(resargs);
    }
    *retargsamm_out = resargs_len;
    struct tqueque* tque = tqueque_create();
    assert(tque);
    uint64_t res_i = 0;
    for(uint64_t i = 0; i < rpctypes_len; i++){
        if(res_i < resargs_len) {
            if(rpctypes[i].flag == 1){
                resargs[res_i] = rpctypes[i];
                res_i++;
            }else tqueque_push(tque,&rpctypes[i]);
        }else tqueque_push(tque,&rpctypes[i]);
    }
    struct rpctype* nonres = NULL;
    while((nonres = tqueque_pop(tque)) != NULL){
        free(nonres->data);
    }
    tqueque_free(tque);
    free(rpctypes);
    return resargs;
}
char* rpcret_to_buf(struct rpcret* rpcret, uint64_t* buflen){
    struct rpcstruct* ret_struct = rpcstruct_create();
    uint64_t ret_buflen = type_buflen(&rpcret->ret);
    char* ret = malloc(type_buflen(&rpcret->ret)); assert(ret);
    type_to_arr(ret,&rpcret->ret);

    uint64_t resargs_buflen = rpctypes_get_buflen(rpcret->resargs,rpcret->resargs_amm);
    char* resargs = malloc(resargs_buflen); assert(resargs);
    rpctypes_to_buf(rpcret->resargs,rpcret->resargs_amm,resargs);

    assert(rpcstruct_set(ret_struct,"ret",ret,ret_buflen,SIZEDBUF) == 0);
    assert(rpcstruct_set(ret_struct,"resargs",resargs,resargs_buflen,SIZEDBUF) == 0);

    char* outbuf = rpcstruct_to_buf(ret_struct,buflen);
    free(ret);
    free(resargs);
    rpcstruct_free(ret_struct);
    return outbuf;
}
void buf_to_rpcret(struct rpcret* ret,char* in){
    char *ret_buf,*resargs = NULL;

    struct rpcstruct ret_structC = {0};
    buf_to_rpcstruct(in,&ret_structC);
    struct rpcstruct* ret_struct = &ret_structC;


    assert(rpcstruct_get(ret_struct,"ret",&ret_buf,NULL,SIZEDBUF) == 0);
    assert(rpcstruct_get(ret_struct,"resargs",&resargs,NULL,SIZEDBUF) == 0);

    arr_to_type(ret_buf,&ret->ret);
    ret->resargs = buf_to_rpctypes(resargs,&ret->resargs_amm);

    rpcstruct_free_internals(ret_struct);
    return;

}

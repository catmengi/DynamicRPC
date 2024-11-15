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
            assert(tqueque_push(check_que,&serv[i],sizeof(enum rpctypes),NULL) == 0);
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
        newserv[i] = *(enum rpctypes*)tqueque_pop(check_que,NULL,NULL);
    }
    int ret = 1;
    for(size_t i = 0; i < clientlen; i++)
        if(newserv[i] != client[i]) {ret = 0;break;}
    tqueque_free(check_que);
    free(newserv);
    return ret;
}
uint64_t rpctypes_get_buflen(struct rpctype* rpctypes,uint64_t rpctypes_len){
    size_t len = sizeof(uint64_t);
    if(rpctypes == NULL) return len;
    for(uint64_t i = 0; i <rpctypes_len; i++){
        len += type_buflen(&rpctypes[i]);
    }
    return len;
}
int rpctypes_to_buf(struct rpctype* rpctypes,uint64_t rpctypes_amm, char* out){
    assert(out);
    uint64_t be64_rpctypes_amm = cpu_to_be64(rpctypes_amm);
    memcpy(out,&be64_rpctypes_amm,sizeof(uint64_t));
    out += sizeof(uint64_t);
    for(uint64_t i = 0; i < rpctypes_amm; i++){
        out += type_to_arr(out,&rpctypes[i]);
    }
    return 0;
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
    *buflen = strlen(rpccall->fn_name) + 1 + rpctypes_get_buflen(rpccall->args,rpccall->args_amm);
    char* buf = calloc(*buflen,sizeof(char));
    assert(buf);
    char* ret = buf;
    memcpy(buf,rpccall->fn_name,strlen(rpccall->fn_name) + 1);
    buf += strlen(rpccall->fn_name) + 1;
    rpctypes_to_buf(rpccall->args,rpccall->args_amm,buf);
    return ret;
}
int buf_to_rpccall(struct rpccall* rpccall,char* in){
    unsigned long fn_name_len = strlen(in);
    rpccall->fn_name = malloc(fn_name_len + 1);
    assert(rpccall->fn_name);
    memcpy(rpccall->fn_name,in,fn_name_len + 1);
    in += fn_name_len + 1;
    rpccall->args = buf_to_rpctypes(in,&rpccall->args_amm);
    return 0;
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
            }else tqueque_push(tque,&rpctypes[i],1,NULL);
        }else tqueque_push(tque,&rpctypes[i],1,NULL);
    }
    struct rpctype* nonres = NULL;
    while((nonres = tqueque_pop(tque,NULL,NULL)) != NULL){
        free(nonres->data);
    }
    tqueque_free(tque);
    free(rpctypes);
    return resargs;
}
char* rpcret_to_buf(struct rpcret* rpcret, uint64_t* buflen){
    *buflen = rpctypes_get_buflen(rpcret->resargs,rpcret->resargs_amm) + type_buflen(&rpcret->ret) + 2;
    char* buf = malloc(*buflen);
    char* ret = buf;
    if(rpcret->ret.type != VOID) *buf = 1;
    else                         *buf = 0;

    buf++;

    if(rpcret->resargs != NULL)  *buf = 1;
    else                         *buf = 0;

    buf++;

    if(rpcret->ret.type != VOID){
        type_to_arr(buf,&rpcret->ret);
        buf += type_buflen(&rpcret->ret);
    }
    if(rpcret->resargs != NULL){
        rpctypes_to_buf(rpcret->resargs,rpcret->resargs_amm,buf);
    }
    return ret;
}
int buf_to_rpcret(struct rpcret* ret,char* in){
    char rflag = *in; in++;
    char aflag = *in; in++;
    if(rflag == 1){
        arr_to_type(in,&ret->ret);
        in += type_buflen(&ret->ret);
    }
    if(aflag == 1) ret->resargs = buf_to_rpctypes(in,&ret->resargs_amm);
    return 0;
}

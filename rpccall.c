#include "rpccall.h"
#include "rpcpack.h"
#include <stddef.h>
#include <stdio.h>
#include "tqueque/tqueque.h"
#include <string.h>
#include <libubox/utils.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

int is_rpctypes_equal(enum rpctypes* serv, size_t servlen, enum rpctypes* client, uint8_t clientlen){
    if((serv && !client) || (!serv && client)) return 0;
    if(!serv && !client) return 1;
    struct tqueque* check_que = tqueque_create();
    assert(check_que);
    size_t newservlen = 0;
    for(size_t i = 0; i < servlen;i++){
        assert(tqueque_push(check_que,&serv[i],sizeof(enum rpctypes),NULL) == 0);
        if(serv[i] == SIZEDBUF) i++;
        newservlen++;
    }
    if(newservlen != clientlen) return 0;
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
size_t rpctypes_get_buflen(struct rpctype* rpctypes,uint8_t rpctypes_len){
    size_t len = sizeof(uint16_t);
    if(rpctypes == NULL) return len;
    for(uint8_t i = 0; i <rpctypes_len; i++){
        len += type_buflen(&rpctypes[i]);
    }
    return len;
}
int rpctypes_to_buf(struct rpctype* rpctypes,uint8_t rpctypes_amm, char* out){
    assert(out);
    char* wr = out + 1;
    for(uint8_t i = 0; i < rpctypes_amm; i++){
        wr += type_to_arr(wr,&rpctypes[i]);
    }
    *out = rpctypes_amm;
    return 0;
}
struct rpctype* buf_to_rpctypes(char* in,uint8_t* rpctypes_amm){
    assert(in);
    uint8_t amm = *in;
    in++;
    if(amm == 0) return NULL;
    struct rpctype* types = malloc(amm * sizeof(*types));
    assert(types);
    *rpctypes_amm = amm;
    for(uint8_t i = 0; i < amm; i++){
        arr_to_type(in,&types[i]);
        in += type_buflen(&types[i]);
    }
    return types;
}
enum rpctypes* rpctypes_get_types(struct rpctype* rpctypes,uint8_t rpctypes_amm){
    if(rpctypes == NULL) return NULL;
    enum rpctypes* erpctypes = malloc(rpctypes_amm * sizeof(enum rpctypes));
    for(uint8_t i = 0; i < rpctypes_amm; i++){
        erpctypes[i] = rpctypes[i].type;
    }
    return erpctypes;
}
void rpctypes_free(struct rpctype* rpctypes, uint8_t rpctypes_amm){
    if(rpctypes == NULL) return;

    for(uint8_t i = 0; i <rpctypes_amm; i++) free(rpctypes[i].data);

    free(rpctypes);
}

char* rpccall_to_buf(struct rpccall* rpccall, uint64_t* buflen){
    assert(rpccall);
    assert(rpccall->fn_name);
    assert(buflen);
    uint64_t argsbuflen = rpctypes_get_buflen(rpccall->args,rpccall->args_amm);
    *buflen = (strlen(rpccall->fn_name) + 1) + argsbuflen + sizeof(uint64_t);
    char* buf = malloc(*buflen);
    assert(buf);
    char* write = buf;
    uint64_t be64_fulstrlen = cpu_to_be64(strlen(rpccall->fn_name) + 1);
    memcpy(write, &be64_fulstrlen, sizeof(uint64_t));
    write += sizeof(uint64_t);
    memcpy(write,rpccall->fn_name,strlen(rpccall->fn_name) + 1);
    write += strlen(rpccall->fn_name) + 1;
    if(rpccall->args == NULL) *write = 0; else *write = 1;
    write++;
    rpctypes_to_buf(rpccall->args,rpccall->args_amm,write);
    return buf;
}
int buf_to_rpccall(struct rpccall* rpccall,char* in){
    if(in == NULL) return 1;
    uint64_t be64_fulstrlen = 0;
    memcpy(&be64_fulstrlen,in,sizeof(uint64_t));
    rpccall->fn_name = malloc(be64_to_cpu(be64_fulstrlen));
    assert(rpccall->fn_name);
    in += sizeof(uint64_t);
    memcpy(rpccall->fn_name,in,be64_to_cpu(be64_fulstrlen));
    in += be64_to_cpu(be64_fulstrlen);
    if(*in == 1) {in++; rpccall->args = buf_to_rpctypes(in,&rpccall->args_amm);}
    return 0;
}
struct rpctype* rpctypes_clean_nonres_args(struct rpctype* rpctypes, uint8_t rpctypes_len,uint8_t* retargsamm_out){
    // struct tqueque* tque = tqueque_create();
    // while(ll){
    //     assert(!(ll->type.flag > 2));
    //     if(ll->type.flag == 0) tqueque_push(tque,ll,sizeof(struct rpctypes_llist*),"non");
    //     if(ll->type.flag > 0) tqueque_push(tque,ll,sizeof(struct rpctypes_llist*),"res");
    //     struct rpctypes_llist* nnull = ll;            //Nulling nexts, maybe some next will point to freed memory on rebuild iteration
    //     ll = ll->next;
    //     nnull->next = NULL;
    // }
    // struct rpctypes_llist* res = tqueque_pop(tque,NULL,"res");
    // struct rpctypes_llist* tres = res;
    // while(tres){
    //     tres->next = tqueque_pop(tque,NULL,"res");
    //     tres = tres->next;
    // }
    // struct rpctypes_llist* freell = NULL;
    // while((freell = tqueque_pop(tque,NULL,"non")) != NULL){
    //     if(freell->type.type != _GENERICPTR){
    //         if(freell->type.data) free(freell->type.data);
    //     }
    //     free(freell);
    // }
    // tqueque_free(tque);
    // return res;
    struct rpctype* resargs = NULL;
    uint8_t resargs_len = 0;
    for(uint8_t i = 0; i < rpctypes_len; i++){
        if(rpctypes[i].flag == 1) resargs_len++;
    }
    if(resargs_len > 0){
        resargs = malloc(resargs_len * sizeof(*resargs));
        assert(resargs);
    }
    *retargsamm_out = resargs_len;
    struct tqueque* tque = tqueque_create();
    assert(tque);
    uint8_t res_i = 0;
    for(uint8_t i = 0; i < rpctypes_len; i++){
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
    assert(rpcret);
    assert(buflen);
    *buflen = rpctypes_get_buflen(rpcret->resargs,rpcret->resargs_amm) + type_buflen(&rpcret->ret) + 5;
    char* buf = calloc(*buflen,sizeof(char));
    char* ret = buf;
    assert(buf);
    if(rpcret->ret.type != VOID){*buf = 1; buf++; buf += type_to_arr(buf,&rpcret->ret);}
    buf++;
    if(rpcret->resargs){*buf = 1; buf++; rpctypes_to_buf(rpcret->resargs,rpcret->resargs_amm,buf);}
    return ret;
}
void buf_to_rpcret(struct rpcret* ret,char* in){
    ret->ret.data = NULL; ret->ret.datalen = 0;
    ret->ret.flag = 0; ret->ret.type = VOID;
    if(*in == 1) {in++;arr_to_type(in,&ret->ret);in += type_buflen(&ret->ret);}
    in++;
    if(*in == 1) {in++; ret->resargs = buf_to_rpctypes(in,&ret->resargs_amm);in++;}
}

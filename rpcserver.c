#include "rpcserver.h"
#include "hashtable.c/hashtable.h"
#include "rpccall.h"
#include "rpcpack.h"
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <ffi.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "rpcmsg.h"
#include "rpctypes.h"
#include "tqueque/tqueque.h"
#include "lb_endian.h"
#include <unistd.h>
#define DEFAULT_CLIENT_TIMEOUT 1
#define DEFAULT_MAXIXIMUM_CLIENT 512
#define _GNU_SOURCE
#define RPCSERVER
ffi_type** rpctypes_to_ffi_types(enum rpctypes* rpctypes,size_t rpctypes_amm){
    if(!rpctypes) return NULL;
    ffi_type** out = calloc(rpctypes_amm, sizeof(ffi_type*));
    assert(out);
    for(size_t i = 0; i < rpctypes_amm; i++){
        assert(rpctypes[i] != VOID);
        if(rpctypes[i] == CHAR){out[i] = &ffi_type_sint8; continue;}
        if(rpctypes[i] == UINT16){out[i] = &ffi_type_uint16; continue;}
        if(rpctypes[i] == INT16){out[i] = &ffi_type_sint16; continue;}
        if(rpctypes[i] == UINT32){out[i] = &ffi_type_uint32; continue;}
        if(rpctypes[i] == INT32){out[i] = &ffi_type_sint32; continue;}
        if(rpctypes[i] == UINT64){out[i] = &ffi_type_uint64; continue;}
        if(rpctypes[i] == INT64){out[i] = &ffi_type_sint64; continue;}
        if(rpctypes[i] == RPCBUFF){out[i] = &ffi_type_pointer; continue;}
        if(rpctypes[i] == STR){out[i] = &ffi_type_pointer; continue;}
        if(rpctypes[i] == UNIQSTR){out[i] = &ffi_type_pointer; continue;}
        if(rpctypes[i] == PSTORAGE){ out[i] = &ffi_type_pointer; continue;}
        if(rpctypes[i] == SIZEDBUF){
            if(rpctypes[i+1] != UINT64) goto exit;
            out[i] = &ffi_type_pointer;
            continue;
        }
        if(rpctypes[i] == INTERFUNC){out[i] = &ffi_type_pointer; continue;}
        if(rpctypes[i] == RPCSTRUCT){ out[i] = &ffi_type_pointer; continue;}
    }
    return out;
exit:
    free(out);
    return NULL;
}
ffi_type* rpctype_to_ffi_type(enum rpctypes rpctype){
        if(rpctype == VOID){return &ffi_type_void;}
        if(rpctype == CHAR){return &ffi_type_schar;}
        if(rpctype == UINT16){return &ffi_type_uint16;}
        if(rpctype == INT16){return &ffi_type_sint16;}
        if(rpctype == UINT32){return &ffi_type_uint32;}
        if(rpctype == INT32){return &ffi_type_sint32;}
        if(rpctype == UINT64){return &ffi_type_uint64;}
        if(rpctype == INT64){return &ffi_type_sint64;}
        if(rpctype == RPCSTRUCT){return &ffi_type_pointer;}
        if(rpctype == RPCBUFF){return &ffi_type_pointer;}
        if(rpctype == STR){return &ffi_type_pointer;}
        return NULL;
}
void* rpcserver_dispatcher(void* vserv);

struct rpcserver* rpcserver_create(uint16_t port){
    struct rpcserver* rpcserver = calloc(1,sizeof(*rpcserver));
    assert(rpcserver);
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET,
                SO_REUSEADDR, &opt,
                sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Forcefully attaching socket to the port
    if (bind(server_fd, (struct sockaddr*)&address,
        sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    listen(server_fd,DEFAULT_MAXIXIMUM_CLIENT);
    rpcserver->sfd = server_fd;
    rpcserver->stop = 0;
    pthread_mutex_init(&rpcserver->edit,NULL);
    assert(hashtable_create(&rpcserver->fn_ht,32,4) == 0);
    assert(hashtable_create(&rpcserver->users,32,4) == 0);
    return rpcserver;
}
void rpcserver_start(struct rpcserver* rpcserver){
    rpcserver->stop = 0;
    assert(pthread_create(&rpcserver->accept_thread, NULL,rpcserver_dispatcher,rpcserver) == 0);
}
void __rpcserver_fn_free_callback(void* cfn){
    struct fn* fn = (struct fn*)cfn;
    free(fn->ffi_type_free);
    free(fn->argtypes);
    free(fn->fn_name);
    free(fn);
}
void rpcserver_free(struct rpcserver* serv){
    if(!serv) return;
    serv->stop = 1;
    while(serv->clientcount > 0) {printf("%s:clients last:%llu\n",__PRETTY_FUNCTION__,serv->clientcount); sleep(1);}
    pthread_mutex_lock(&serv->edit);
    hashtable_iterate(serv->fn_ht,__rpcserver_fn_free_callback);
    hashtable_iterate(serv->users,free);
    hashtable_free(serv->fn_ht);
    hashtable_free(serv->users);
    serv->fn_ht = NULL;
    serv->interfunc = NULL;
    pthread_mutex_unlock(&serv->edit);
    shutdown(serv->sfd,SHUT_RD);
    close(serv->sfd);
    pthread_join(serv->accept_thread,NULL);
    free(serv);
}
void rpcserver_stop(struct rpcserver* serv){
    if(!serv) return;
    serv->stop = 1;
    while(serv->clientcount > 0) {printf("%s:clients last:%llu\n",__PRETTY_FUNCTION__,serv->clientcount); sleep(1);}
    pthread_mutex_lock(&serv->edit);
    pthread_mutex_unlock(&serv->edit);
    shutdown(serv->sfd,SHUT_RD);
    close(serv->sfd);
    pthread_join(serv->accept_thread,NULL);
}
int rpcserver_register_fn(struct rpcserver* serv, void* fn, char* fn_name,
                          enum rpctypes rtype, enum rpctypes* argstype,
                          uint8_t argsamm, void* pstorage,int perm){
    pthread_mutex_lock(&serv->edit);
    assert(serv && fn && fn_name);
    struct fn* fnr = malloc(sizeof(*fnr));
    assert(fnr);
    size_t eargsamm = argsamm;
    if(argsamm > 0){
        fnr->argtypes = malloc(sizeof(enum rpctypes) * eargsamm);
        assert(fnr->argtypes);
        memcpy(fnr->argtypes, argstype,sizeof(enum rpctypes) * argsamm);
    }else fnr->argtypes = NULL;
    argsamm = eargsamm;
    fnr->rtype = rtype;
    fnr->nargs = argsamm;
    fnr->fn = fn;
    fnr->fn_name = malloc(strlen(fn_name) + 1);
    assert(fnr->fn_name);
    memcpy(fnr->fn_name, fn_name, strlen(fn_name)+1);
    fnr->personal = pstorage;
    fnr->perm = perm;
    ffi_type** argstype_ffi = rpctypes_to_ffi_types(argstype,argsamm);
    if(argstype_ffi == NULL && argstype != NULL) return 2;
    fnr->ffi_type_free = argstype_ffi;
    ffi_type* rtype_ffi = rpctype_to_ffi_type(rtype);
    if(ffi_prep_cif(&fnr->cif,FFI_DEFAULT_ABI,fnr->nargs,rtype_ffi,argstype_ffi) != FFI_OK) return 1;
    if(hashtable_add(serv->fn_ht,fn_name,strlen(fn_name) + 1,fnr,0) != 0) return 1;
    pthread_mutex_unlock(&serv->edit);
    return 0;
}
void rpcserver_unregister_fn(struct rpcserver* serv, char* fn_name){
    assert(serv);
    pthread_mutex_lock(&serv->edit);
    struct fn* cfn = NULL;
    if(hashtable_get(serv->fn_ht,fn_name,strlen(fn_name) + 1,(void**)&cfn) != 0) {pthread_mutex_unlock(&serv->edit);return;}
    free(cfn->argtypes);
    free(cfn->ffi_type_free);
    free(cfn);
    hashtable_remove_entry(serv->fn_ht,fn_name,strlen(fn_name) + 1);
    pthread_mutex_unlock(&serv->edit);
}
int __rpcserver_call_fn(struct rpcret* ret,struct rpcserver* serv,struct rpccall* call,struct fn* cfn, int* err_code, char* uniq){
    void** callargs = NULL;
    if(cfn->nargs > 0){
        callargs = calloc(cfn->nargs, sizeof(void*));
        assert(callargs);
    }
    uint8_t j = 0;
    struct tqueque* rpcbuff_upd = tqueque_create();
    struct tqueque* rpcbuff_free = tqueque_create();
    struct tqueque* rpcstruct_upd = tqueque_create();
    struct tqueque* _rpcstruct_free = tqueque_create();
    struct tqueque* strret_free = tqueque_create(); //queue to track do i need to free return if it STR type
    assert(rpcbuff_upd);
    assert(rpcbuff_free);
    assert(rpcstruct_upd);
    assert(_rpcstruct_free);
    assert(strret_free);
    enum rpctypes* check = rpctypes_get_types(call->args,call->args_amm);
    if(!is_rpctypes_equal(cfn->argtypes,cfn->nargs,check,call->args_amm)){
        *err_code = 7;
        free(check);
        goto exit;
    }
    free(check);
    assert((callargs != NULL && call->args_amm != 0) || call->args_amm == 0);
    for(uint8_t i = 0; i < cfn->nargs; i++){
        if(cfn->argtypes[i] == PSTORAGE){
            callargs[i] = calloc(1,sizeof(void*));
            assert(callargs[i]);
            *(void**)callargs[i] = cfn->personal;
            continue;
        }
        if(cfn->argtypes[i] == INTERFUNC){
            callargs[i] = calloc(1,sizeof(void*));
            assert(callargs[i]);
            *(void**)callargs[i] = serv->interfunc;
            continue;
        }
        if(cfn->argtypes[i] == UNIQSTR){
            callargs[i] = calloc(1,sizeof(void*));
            assert(callargs[i]);
            *(void**)callargs[i] = uniq;
            if(cfn->rtype == STR) tqueque_push(strret_free,*(void**)callargs[i],1,NULL);
            continue;
        }
        if(j < call->args_amm){
            if(call->args[j].type == RPCBUFF){
                callargs[i] = calloc(1,sizeof(void*));
                assert(callargs[i]);
                *(void**)callargs[i] = unpack_rpcbuff_type(&call->args[j]);
                if(call->args[j].flag == 1)
                    tqueque_push(rpcbuff_upd,*(void**)callargs[i],1,NULL);
                else
                    tqueque_push(rpcbuff_free,*(void**)callargs[i],1,NULL);
                free(call->args[j].data);
                call->args[j].data = NULL;
                j++;
                continue;
            }
            if(call->args[j].type == RPCSTRUCT){
                callargs[i] = calloc(1,sizeof(void*));
                assert(callargs[i]);
                *(void**)callargs[i] = unpack_rpcstruct_type(&call->args[j]);
                if(call->args[j].flag == 1)
                    tqueque_push(rpcstruct_upd,*(void**)callargs[i],1,NULL);
                else
                    tqueque_push(_rpcstruct_free,*(void**)callargs[i],1,NULL);
                free(call->args[j].data);
                call->args[j].data = NULL;
                j++;
                continue;
            }
            if(call->args[j].type == SIZEDBUF){
                callargs[i] = calloc(1,sizeof(void*));
                assert(callargs[i]);
                uint64_t buflen = 0;
                *(void**)callargs[i] = unpack_sizedbuf_type(&call->args[j],&buflen);
                callargs[++i] = calloc(1,sizeof(void*));
                assert(callargs[i]);
                *(uint64_t*)callargs[i] = buflen;
                j++;
                continue;
            }
            if(call->args[j].type == STR){
                callargs[i] = calloc(1,sizeof(char*));
                assert(callargs[i]);
                *(void**)callargs[i] =  unpack_str_type(&call->args[j]);
                if(cfn->rtype == STR) tqueque_push(strret_free,*(void**)callargs[i],1,NULL);
                j++;
                continue;
            }
            if(call->args[j].type == CHAR){
                callargs[i] = calloc(1,sizeof(char));
                assert(callargs[i]);
                *(char*)callargs[i] =  type_to_char(&call->args[j]);
                j++;
                continue;
            }
            if(call->args[j].type == INT16){
                callargs[i] = calloc(1,sizeof(int16_t));
                assert(callargs[i]);
                *(int16_t*)callargs[i] =  type_to_int16(&call->args[j]);
                j++;
                continue;
            }
            if(call->args[j].type == UINT16){
                callargs[i] = calloc(1,sizeof(uint16_t));
                assert(callargs[i]);
                *(uint16_t*)callargs[i] =  type_to_uint16(&call->args[j]);
                j++;
                continue;
            }
            if(call->args[j].type == INT32){
                callargs[i] = calloc(1,sizeof(int32_t));
                assert(callargs[i]);
                *(int32_t*)callargs[i] =  type_to_int32(&call->args[j]);
                j++;
                continue;
            }
            if(call->args[j].type == UINT32){
                callargs[i] = calloc(1,sizeof(uint32_t));
                assert(callargs[i]);
                *(uint32_t*)callargs[i] =  type_to_uint32(&call->args[j]);
                j++;
                continue;
            }
            if(call->args[j].type == INT64){
                callargs[i] = calloc(1,sizeof(int64_t));
                assert(callargs[i]);
                *(int64_t*)callargs[i] =  type_to_int64(&call->args[j]);
                j++;
                continue;
            }
            if(call->args[j].type == UINT64){
                callargs[i] = calloc(1,sizeof(uint64_t));
                assert(callargs[i]);
                *(uint64_t*)callargs[i] =  type_to_uint64(&call->args[j]);
                j++;
                continue;
            }
            if(call->args[j].type == FLOAT){
                callargs[i] = calloc(1,sizeof(float));
                assert(callargs[i]);
                *(float*)callargs[i] =  type_to_float(&call->args[j]);
                j++;
                continue;
            }
            if(call->args[j].type == DOUBLE){
                callargs[i] = calloc(1,sizeof(double));
                assert(callargs[i]);
                *(double*)callargs[i] =  type_to_double(&call->args[j]);
                j++;
                continue;
            }
        } else {*err_code = 7; goto exit;}
    }
    void* fnret = NULL;
    if(cfn->rtype != VOID){
        fnret = calloc(1,sizeof(uint64_t));
        assert(fnret);
    }
    ffi_call(&cfn->cif,cfn->fn,fnret,callargs);
    enum rpctypes rtype = cfn->rtype;
    ret->ret.type = VOID;
    if(rtype != VOID){
        if(rtype == CHAR)    char_to_type(*(char*)fnret,&ret->ret);
        if(rtype == INT16)   int16_to_type(*(int16_t*)fnret,&ret->ret);
        if(rtype == UINT16)  uint16_to_type(*(uint16_t*)fnret,&ret->ret);
        if(rtype == INT32)   int32_to_type(*(int32_t*)fnret,&ret->ret);
        if(rtype == UINT32)  uint32_to_type(*(uint32_t*)fnret,&ret->ret);
        if(rtype == INT64)   int64_to_type(*(int64_t*)fnret,&ret->ret);
        if(rtype == UINT64)  uint64_to_type(*(uint64_t*)fnret,&ret->ret);
        if(rtype == FLOAT)   float_to_type(*(float*)fnret,&ret->ret);
        if(rtype == DOUBLE)  double_to_type(*(double*)fnret,&ret->ret);
        
        if(rtype == STR && *(void**)fnret != NULL){
            create_str_type(*(char**)fnret,0,&ret->ret);
            int needfree = 1;
            size_t el = tqueque_get_tagamm(strret_free,NULL);
            for(size_t i = 0; i < el; i++){
                if(!needfree) break;
                void* ptr = tqueque_pop(strret_free,NULL,NULL);
                if(ptr == *(char**)fnret) needfree = 0;
                assert(tqueque_push(strret_free,ptr,sizeof(struct rpcstruct),NULL) == 0);
            }
            if(needfree) free(*(char**)fnret);
        }else if(rtype == STR && *(void**)fnret == NULL){
            ret->ret.type = VOID;
        }
        if(rtype == RPCBUFF && *(void**)fnret != NULL){
            int needfree = 1;
            size_t el = tqueque_get_tagamm(rpcbuff_upd,NULL);
            for(size_t i = 0; i < el; i++){
                if(!needfree) break;
                void* ptr = tqueque_pop(rpcbuff_upd,NULL,NULL);
                if(ptr == *(struct rpcbuff**)fnret) needfree = 0;

                assert(tqueque_push(rpcbuff_upd,ptr,sizeof(struct rpcbuff),NULL) == 0);
            }
            el = 0;
            el = tqueque_get_tagamm(rpcbuff_free,NULL);
            for(size_t i = 0; i < el; i++){
                if(!needfree) break;
                void* ptr = tqueque_pop(rpcbuff_free,NULL,NULL);
                if(ptr == *(struct rpcbuff**)fnret) needfree = 0;

                assert(tqueque_push(rpcbuff_free,ptr,sizeof(struct rpcbuff),NULL) == 0);
            }
            create_rpcbuff_type(*(struct rpcbuff**)fnret,0,&ret->ret);
            if(needfree) _rpcbuff_free(*(struct rpcbuff**)fnret);
        }
        else if(rtype == RPCBUFF && *(void**)fnret == NULL)
            ret->ret.type = VOID;
        if(rtype == RPCSTRUCT && *(void**)fnret != NULL){
            int needfree = 1;
            size_t el = tqueque_get_tagamm(rpcstruct_upd,NULL);
            for(size_t i = 0; i < el; i++){
                if(!needfree) break;
                void* ptr = tqueque_pop(rpcstruct_upd,NULL,NULL);
                if(ptr == *(struct rpcstruct**)fnret) needfree = 0;

                assert(tqueque_push(rpcstruct_upd,ptr,sizeof(struct rpcstruct),NULL) == 0);
            }
            el = 0;
            el = tqueque_get_tagamm(_rpcstruct_free,NULL);
            for(size_t i = 0; i < el; i++){
                if(!needfree) break;
                void* ptr = tqueque_pop(_rpcstruct_free,NULL,NULL);
                if(ptr == *(struct rpcstruct**)fnret) needfree = 0;

                assert(tqueque_push(_rpcstruct_free,ptr,sizeof(struct rpcstruct),NULL) == 0);
            }
            create_rpcstruct_type(*(struct rpcstruct**)fnret,0,&ret->ret);
            if(needfree) {rpcstruct_free(*(struct rpcstruct**)fnret);}
        }
        else if(rtype == RPCSTRUCT && *(void**)fnret == NULL)
            ret->ret.type = VOID;
    }
    tqueque_free(strret_free);
    ret->resargs = rpctypes_clean_nonres_args(call->args,call->args_amm,&ret->resargs_amm);
    for(uint8_t i = 0; i < ret->resargs_amm; i++){
        if(ret->resargs[i].type == RPCBUFF){
            struct rpcbuff* buf = tqueque_pop(rpcbuff_upd,NULL,NULL);
            if(!buf) break;
            create_rpcbuff_type(buf,ret->resargs[i].flag,&ret->resargs[i]);
            _rpcbuff_free(buf);
        }
        if(ret->resargs[i].type == RPCSTRUCT){
            struct rpcstruct* buf = tqueque_pop(rpcstruct_upd,NULL,NULL);
            if(!buf) break;
            create_rpcstruct_type(buf,ret->resargs[i].flag,&ret->resargs[i]);
            rpcstruct_free(buf);
        }
    }
    tqueque_free(rpcbuff_upd);
    tqueque_free(rpcstruct_upd);
    void* buf = NULL;
    while((buf = tqueque_pop(rpcbuff_free,NULL,NULL)) != NULL) _rpcbuff_free(buf);
    while((buf = tqueque_pop(_rpcstruct_free,NULL,NULL)) != NULL) {rpcstruct_free(buf);}
    tqueque_free(rpcbuff_free);
    tqueque_free(_rpcstruct_free);
    free(fnret);
    for(uint8_t i = 0; i < cfn->nargs; i++){
        free(callargs[i]);
    }
    free(callargs);
    return 0;
exit:
    while((buf = tqueque_pop(rpcbuff_free,NULL,NULL)) != NULL) _rpcbuff_free(buf);
    while((buf = tqueque_pop(rpcbuff_upd,NULL,NULL)) != NULL) _rpcbuff_free(buf);
    while((buf = tqueque_pop(rpcstruct_upd,NULL,NULL)) != NULL) {rpcstruct_free(buf);}
    while((buf = tqueque_pop(_rpcstruct_free,NULL,NULL)) != NULL) {rpcstruct_free(buf);}
    tqueque_free(rpcbuff_free);
    tqueque_free(rpcbuff_upd);
    tqueque_free(rpcstruct_upd);
    tqueque_free(_rpcstruct_free);
    tqueque_free(strret_free);
    for(uint8_t i = 0; i < cfn->nargs; i++){
        free(callargs[i]);
    }
    free(callargs);
    return 1;
}
void __rpcserver_lsfn_create_callback(char* key, void* fn, void* Pusr,size_t unused){

    void** usr = Pusr;
    int32_t perm = ((struct fn*)fn)->perm;
    if(*(int*)usr[0] > perm || *(int*)usr[0] == -1){
        if(perm == -1 && *(int*)usr[0] != -1) return;
        struct tqueque* check_que = tqueque_create();
        assert(check_que);
        size_t newservlen = 0;
        for(size_t i = 0; i < ((struct fn*)fn)->nargs;i++){
            assert(tqueque_push(check_que,&((struct fn*)fn)->argtypes[i],sizeof(enum rpctypes),NULL) == 0);
            if(((struct fn*)fn)->argtypes[i] == SIZEDBUF) i++;
            newservlen++;
        }
        char* newserv = calloc(newservlen,sizeof(char));
        assert(newserv);
        for(size_t i = 0; i < newservlen; i++){
            newserv[i] = *(enum rpctypes*)tqueque_pop(check_que,NULL,NULL);
        }
        
        assert(rpcstruct_set(usr[1],key,SIZEDBUF,newserv,newservlen) == 0);
        tqueque_free(check_que);
        free(newserv);
    }
}
char* __rpcserver_lsfn(struct rpcserver* serv,uint64_t* outlen,int user_perm){
    struct rpcstruct lsfn;
    assert(__rpcstruct_create(&lsfn) == 0);
    void** callback_data[2] = {(void*)&user_perm,(void*)&lsfn};
    hashtable_iterate_wkey(serv->fn_ht,callback_data,__rpcserver_lsfn_create_callback);
    struct rpctype otype;
    create_rpcstruct_type(&lsfn,0,&otype);
    __rpcstruct_free(&lsfn);
    *outlen = type_buflen(&otype);
    char* out = malloc(*outlen);
    assert(out);
    type_to_arr(out,&otype);
    free(otype.data);
    return out;
}
void __get_uniq(char* uniq,int L){
    uniq[L - 1] = '\0';
    char charset[] = "qwertyuiop[]asdfghjkl;'zxcvbnm,./QWERTYUIOP{}ASDFGHJKL:ZXCVBNM<>?1234567890-=!@#$%^&*()_+";
    srand((unsigned int)time(NULL));
    for(int i = 0; i < L - 1; i++){
        uniq[i] = charset[(rand() % (sizeof(charset) - 1))];
    }
}
void* rpcserver_client_thread(void* arg){
    struct client_thread* thrd = (struct client_thread*)arg;
    struct rpcmsg gotmsg = {0},repl = {0};
    thrd->serv->clientcount++;
    int user_perm = 0;
    int is_authed = 0;
    if(get_rpcmsg_from_fd(&gotmsg,thrd->client_fd) == 0 && gotmsg.msg_type == AUTH && gotmsg.payload != NULL){
        uint64_t credlen = 0;
        struct rpctype type;
        uint64_t hash = type_to_uint64(arr_to_type(gotmsg.payload,&type));
        free(gotmsg.payload);
        if(hash != 0){
            int* gotusr = NULL;
            if(hashtable_get_by_hash(thrd->serv->users,hash,(void**)&gotusr) == 0){
                assert(gotusr);
                is_authed = 1;
                user_perm = *gotusr;
            }
        }
        free(type.data);
        if(is_authed){
            repl.msg_type = OK;
            __get_uniq(thrd->client_uniq,sizeof(thrd->client_uniq));
            struct rpctype perm = {0};
            create_str_type(thrd->client_uniq,0,&perm);
            repl.payload = malloc((repl.payload_len = type_buflen(&perm)));
            assert(repl.payload);
            type_to_arr(repl.payload,&perm);
            free(perm.data);
            int iserror = 0;
            if(rpcmsg_write_to_fd(&repl,thrd->client_fd) != 0) iserror = 1; 
            repl.msg_type = 0;
            free(repl.payload);
            repl.payload = NULL;
            repl.payload_len = 0;
            if(iserror) goto exit;
            printf("%s: auth ok, OK is replied to client\n",__PRETTY_FUNCTION__);
            if(thrd->serv->newclient_cb != NULL)
                thrd->serv->newclient_cb(thrd->serv->newclient_cb_data,thrd->addr);
            while(thrd->serv->stop == 0){
                repl.msg_type = 0; repl.payload = NULL; repl.payload_len = 0;
                gotmsg.msg_type = 0; gotmsg.payload = NULL; gotmsg.payload_len = 0;
                struct rpccall call = {0}; struct rpcret ret = {0};
                if(get_rpcmsg_from_fd(&gotmsg,thrd->client_fd) != 0) {printf("%s: client disconected badly\n",__PRETTY_FUNCTION__);goto exit;}
                switch(gotmsg.msg_type){
                    case LSFN:
                                    printf("%s: client requested list of registred functions!\n",__PRETTY_FUNCTION__);
                                    repl.payload = __rpcserver_lsfn(thrd->serv,&repl.payload_len,user_perm);
                                    repl.msg_type = LSFN;
                                    if(rpcmsg_write_to_fd(&repl,thrd->client_fd) != 0) {free(repl.payload);goto exit;}
                                    free(repl.payload);
                                    break;
                    case PING:
                                    free(gotmsg.payload);
                                    break;
                    case DISCON:
                                    free(gotmsg.payload);
                                    printf("%s: client disconnected normaly\n",__PRETTY_FUNCTION__);
                                    goto exit;
                    case CALL:
                                    if(buf_to_rpccall(&call,gotmsg.payload,gotmsg.payload_len) != 0 ){
                                        printf("%s: bad 'CALL' msg! \n",__PRETTY_FUNCTION__);
                                        free(gotmsg.payload);
                                        free(call.fn_name);
                                        goto exit;
                                    }
                                    free(gotmsg.payload);
                                    pthread_mutex_lock(&thrd->serv->edit); pthread_mutex_unlock(&thrd->serv->edit);
                                    struct fn* cfn = NULL;hashtable_get(thrd->serv->fn_ht,call.fn_name,strlen(call.fn_name) + 1,(void**)&cfn);
                                    if(cfn == NULL){
                                        repl.msg_type = NOFN;
                                        printf("%s: '%s' no such function\n",__PRETTY_FUNCTION__,call.fn_name);
                                        free(call.fn_name); rpctypes_free(call.args,call.args_amm);
                                        if(rpcmsg_write_to_fd(&repl,thrd->client_fd) != 0) goto exit;
                                        break;
                                    }else if((cfn->perm > user_perm || cfn->perm == -1) && user_perm != -1){
                                        repl.msg_type = LPERM;
                                        printf("%s: low permissions, need %d, have %d\n",__PRETTY_FUNCTION__,cfn->perm,(int)user_perm);
                                        free(call.fn_name);
                                        rpctypes_free(call.args,call.args_amm);
                                        if(rpcmsg_write_to_fd(&repl,thrd->client_fd) != 0) goto exit;
                                        break;
                                    }
                                    repl.msg_type = RET;
                                    int err = 0;
                                    int callret = 0;
                                    if((callret = __rpcserver_call_fn(&ret,thrd->serv,&call,cfn,&err,thrd->client_uniq)) != 0 && err == 0){
                                        free(call.fn_name);
                                        rpctypes_free(call.args,call.args_amm);
                                        printf("%s: internal server error\n",__PRETTY_FUNCTION__);
                                        goto exit;
                                    }else if(callret != 0 && err != 0){
                                        repl.msg_type = BAD;
                                        printf("%s: client provided wrong arguments\n",__PRETTY_FUNCTION__);
                                        free(call.fn_name);
                                        rpctypes_free(call.args,call.args_amm);
                                        if(rpcmsg_write_to_fd(&repl,thrd->client_fd) != 0) goto exit;
                                        break;
                                    }
                                    free(call.fn_name);
                                    repl.payload = rpcret_to_buf(&ret,&repl.payload_len);
                                    rpctypes_free(ret.resargs,ret.resargs_amm);
                                    int iserror = 0;
                                    if(rpcmsg_write_to_fd(&repl,thrd->client_fd) != 0) iserror = 1;
                                    free(repl.payload);
                                    if(ret.ret.data) free(ret.ret.data);
                                    if(iserror) goto exit;
                                    break;

                    default:
                            free(gotmsg.payload);
                            free(call.fn_name);
                            rpctypes_free(call.args,call.args_amm);
                            printf("%s: client sent non call or disconected badly(%d), exiting\n",__PRETTY_FUNCTION__,gotmsg.msg_type);
                            goto exit;
                }
            }
        }else {repl.msg_type = BAD;rpcmsg_write_to_fd(&repl,thrd->client_fd);printf("%s: client not passed auth\n",__PRETTY_FUNCTION__);}
    } else printf("%s: no auth provided\n",__PRETTY_FUNCTION__);

exit:
    if(thrd->serv->stop == 1) printf("%s: server stopping, exiting\n",__PRETTY_FUNCTION__);
    struct rpcmsg lreply = {DISCON,0,NULL};
    rpcmsg_write_to_fd(&lreply,thrd->client_fd);
    shutdown(thrd->client_fd, SHUT_RD);
    close(thrd->client_fd);
    thrd->serv->clientcount--;
    if(thrd->serv->clientdiscon_cb != NULL)
        thrd->serv->clientdiscon_cb(thrd->serv->clientdiscon_cb_data, thrd->addr);
    free(thrd);
    pthread_detach(pthread_self());
    pthread_exit(NULL);
}

/*this function must run in another thread*/
void* rpcserver_dispatcher(void* vserv){
    struct rpcserver* serv = (struct rpcserver*)vserv;
    assert(serv);
    int fd = 0;
    printf("%s: dispatcher started\n",__PRETTY_FUNCTION__);
    while(serv->stop == 0){
        if(serv->clientcount < DEFAULT_MAXIXIMUM_CLIENT){
            struct sockaddr_in addr = {0};
            unsigned int addrlen = sizeof(addr);
            fd = accept(serv->sfd, (struct sockaddr*)&addr,&addrlen);
                if(fd < 0) break;
                printf("%s: got client from %s\n",__PRETTY_FUNCTION__,inet_ntoa(addr.sin_addr));
                struct timeval time;
                time.tv_sec = 5;
                time.tv_usec = 0;
                assert(setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&time,sizeof(time)) == 0);
                assert(setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&time,sizeof(time)) == 0);
                struct rpcmsg msg = {0};
                if(get_rpcmsg_from_fd(&msg,fd) == 0){
                    printf("%s: got msg from client\n",__PRETTY_FUNCTION__);
                    if(msg.msg_type == CON){
                        printf("%s: connection ok,passing to client_thread to auth\n",__PRETTY_FUNCTION__);
                        struct client_thread* thrd = malloc(sizeof(*thrd));
                        assert(thrd);
                        thrd->client_fd = fd;
                        thrd->serv = serv;
                        thrd->addr = addr;
                        pthread_t client_thread = 0;
                        assert(pthread_create(&client_thread,NULL,rpcserver_client_thread,thrd) == 0);
                    }else {printf("%s: client not connected\n",__PRETTY_FUNCTION__);shutdown(fd, SHUT_RD);close(fd);memset(&addr,0,sizeof(addr));}
                }else{printf("%s: wrong info from client\n",__PRETTY_FUNCTION__);shutdown(fd, SHUT_RD);close(fd);memset(&addr,0,sizeof(addr));}
                if(msg.payload) free(msg.payload);
        }else{printf("%s:server overloaded\n",__PRETTY_FUNCTION__); sleep(1);}
    }
    printf("%s: dispatcher stopped\n",__PRETTY_FUNCTION__);
    pthread_exit(NULL);
}
void rpcserver_register_newclient_cb(struct rpcserver* serv,rpcserver_newclient_cb callback, void* user){
    assert(serv);
    serv->newclient_cb = callback;
    serv->newclient_cb_data = user;
}
void rpcserver_register_clientdiscon_cb(struct rpcserver* serv,rpcserver_clientdiscon_cb callback, void* user){
    assert(serv);
    serv->clientdiscon_cb = callback;
    serv->clientdiscon_cb_data = user;
}
void rpcserver_load_keys(struct rpcserver* serv, char* filename){
    FILE* keys = fopen(filename,"r");
    if(!keys){
        perror("rpcserver_load_keys");
        exit(1);
    }
    char buf[512] = {0};
    while(fgets(buf,sizeof(buf),keys)){
        /*yet another shit text parser*/
        size_t i = 0;
        for(i = 0; i < sizeof(buf) && buf[i] != '"'; i++);
        if(i == sizeof(buf) - 1) continue;

        char* key_start = &buf[++i];
        char* perm_start = strchr(&buf[i],'"');
        if(!perm_start) continue;
        *perm_start = '\0'; perm_start++;
        int perm = atoi(perm_start);
        if(perm == 0) continue;
        int* lperm = malloc(sizeof(perm)); assert(lperm);
        *lperm = perm;
        hashtable_add(serv->users,key_start,strlen(key_start) + 1,lperm,0);
    }
    fclose(keys);
}

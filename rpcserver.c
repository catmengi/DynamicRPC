#include "rpcserver.h"
#include "rpcpack.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <ffi.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "rpcmsg.h"
#include <libubox/utils.h>
#include <time.h>
#include <unistd.h>
#define DEFAULT_CLIENT_TIMEOUT 1
#define DEFAULT_MAXIXIMUM_CLIENT 512
#define _GNU_SOURCE

enum rpctypes* ffi_types_to_rpctypes(ffi_type** ffi_types, size_t ffi_types_amm){
    if(!ffi_types) return NULL;
    enum rpctypes* out = malloc(ffi_types_amm * sizeof(enum rpctypes));
    assert(out);
    for(size_t i = 0; i <ffi_types_amm; i++){
        if(ffi_types[i] == &ffi_type_void){out[i] = VOID; continue;}
        if(ffi_types[i] == &ffi_type_sint8){out[i] = CHAR; continue;}
        if(ffi_types[i] == &ffi_type_uint16){out[i] = UINT16; continue;}
        if(ffi_types[i] == &ffi_type_sint16){out[i] = INT16; continue;}
        if(ffi_types[i] == &ffi_type_uint32){out[i] = UINT32; continue;}
        if(ffi_types[i] == &ffi_type_sint32){out[i] = INT32; continue;}
        if(ffi_types[i] == &ffi_type_uint64){out[i] = UINT64; continue;}
        if(ffi_types[i] == &ffi_type_sint64){out[i] = INT64; continue;}
        if(ffi_types[i] == &ffi_type_float){out[i] = FLOAT; continue;}
        if(ffi_types[i] == &ffi_type_double){out[i] = DOUBLE; continue;}
        if(ffi_types[i] == &ffi_type_pointer){out[i] = RPCBUFF; continue;}
    }
    return out;
}
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
        if(rpctypes[i] == PSTORAGE){ out[i] = &ffi_type_pointer; continue;}
        if(rpctypes[i] == SIZEDBUF){
            if(rpctypes[i+1] != UINT64) goto exit;
            out[i] = &ffi_type_pointer;
            continue;
        }
        if(rpctypes[i] == INTERFUNC){out[i] = &ffi_type_pointer; continue;}
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
        if(rpctype == RPCBUFF){return &ffi_type_pointer;}
        if(rpctype == STR){return &ffi_type_pointer;}
        return NULL;
}
enum rpctypes ffi_type_to_rpctype(ffi_type* ffi_type){
        assert(ffi_type);
        if(ffi_type == &ffi_type_void){return VOID;}
        if(ffi_type == &ffi_type_sint8){return CHAR;}
        if(ffi_type == &ffi_type_uint16){return UINT16;}
        if(ffi_type == &ffi_type_sint16){return INT16;}
        if(ffi_type == &ffi_type_uint32){return UINT32;}
        if(ffi_type == &ffi_type_sint32){return INT32;}
        if(ffi_type == &ffi_type_uint64){return UINT64;}
        if(ffi_type == &ffi_type_sint64){return INT64;}
        if(ffi_type == &ffi_type_float){return FLOAT;}
        if(ffi_type == &ffi_type_double){return DOUBLE;}
        if(ffi_type == &ffi_type_pointer){return RPCBUFF;}
        return VOID;
}

void* rpcserver_dispatcher_reliver(void* args);
void* rpcserver_dispatcher(void* vserv);

struct rpcserver* rpcserver_create(uint16_t port){
    struct rpcserver* rpcserver = calloc(1,sizeof(*rpcserver));
    assert(rpcserver);
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET,
                SO_REUSEADDR | SO_REUSEPORT, &opt,
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
    assert(hashtable_create(&rpcserver->fn_ht,32,4) == 0);
    assert(hashtable_create(&rpcserver->users,32,4) == 0);
    return rpcserver;
}
void rpcserver_start(struct rpcserver* rpcserver){
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    rpcserver->reliverargs = malloc(sizeof(void*) * 2);
    assert(rpcserver->reliverargs);
    rpcserver->reliverargs[0] = rpcserver;
    if(pthread_create(&rpcserver->accept_thread, &attr,rpcserver_dispatcher,rpcserver)) exit(-1);
    if(pthread_create(&rpcserver->reliver, &attr,rpcserver_dispatcher_reliver,rpcserver->reliverargs)) exit(-1);
    pthread_attr_destroy(&attr);
}
void __rpcserver_fn_free_callback(void* cfn){
    struct fn* fn = (struct fn*)cfn;
    free(fn->ffi_type_free);
    free(fn->argtypes);
    free(fn->fn_name);
    if(fn->personal) free(fn->personal);
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
    free(serv->reliverargs);
    pthread_mutex_unlock(&serv->edit);
    close(serv->sfd);
    pthread_join(serv->accept_thread,NULL);
    pthread_join(serv->reliver,NULL);
    free(serv);
}
int rpcserver_register_fn(struct rpcserver* serv, void* fn, char* fn_name,
                          enum rpctypes rtype, enum rpctypes* argstype,
                          uint8_t argsamm, void* pstorage,int perm){
    pthread_mutex_lock(&serv->edit);
    assert(serv && fn && fn_name);
    struct fn* fnr = malloc(sizeof(*fnr));
    assert(fnr);
    size_t eargsamm = argsamm;
    fnr->argtypes = malloc(sizeof(enum rpctypes) * eargsamm);
    assert(fnr->argtypes);
    memcpy(fnr->argtypes, argstype,sizeof(enum rpctypes) * argsamm);
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
    if(argstype_ffi == NULL) return 2;
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
    if(hashtable_get(serv->fn_ht,fn_name,strlen(fn_name) + 1,(void**)&cfn) != 0) return;
    free(cfn->argtypes);
    free(cfn->ffi_type_free);
    free(cfn->personal);
    free(cfn);
    hashtable_remove_entry(serv->fn_ht,fn_name,strlen(fn_name) + 1);
    pthread_mutex_unlock(&serv->edit);
}
int __rpcserver_call_fn(struct rpcret* ret,struct rpcserver* serv,struct rpccall* call,struct fn* cfn, int* err_code){
    void** callargs = calloc(cfn->nargs, sizeof(void*));
    struct tqueque* rpcbuff_upd = tqueque_create();
    struct tqueque* rpcbuff_free = tqueque_create();
    assert(rpcbuff_upd);
    assert(rpcbuff_free);
    enum rpctypes* check = rpctypes_get_types(call->args,call->args_amm);
    if(!is_rpctypes_equal(cfn->argtypes,cfn->nargs,check,call->args_amm)){
        *err_code = 7;
        free(check);
        goto exit;
    }
    free(check);
    assert(callargs != NULL && call->args_amm != 0);
    uint8_t j = 0;
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
    ret->resargs = rpctypes_clean_nonres_args(call->args,call->args_amm,&ret->resargs_amm);
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
        if(rtype == DOUBLE)  double_to_type(*(double*)fnret,&ret->ret);\

        if(rtype == STR && *(void**)fnret != NULL){
            create_str_type(*(char**)fnret,0,&ret->ret);
            free(*(char**)fnret);
        }else if(rtype == STR && *(void**)fnret == NULL){
            ret->ret.type = VOID;
        }

        if(rtype == RPCBUFF && *(void**)fnret != NULL){
            create_rpcbuff_type(*(struct rpcbuff**)fnret,0,&ret->ret);
        }
        else if(rtype == RPCBUFF && *(void**)fnret == NULL)
            ret->ret.type = VOID;

    }
    for(uint8_t i = 0; i < ret->resargs_amm; i++){
        if(ret->resargs[i].flag == RPCBUFF){
            struct rpcbuff* buf = tqueque_pop(rpcbuff_upd,NULL,NULL);
            if(!buf) break;
            create_rpcbuff_type(buf,ret->resargs[i].flag,&ret->resargs[i]);
            _rpcbuff_free(buf);
        }
    }
    tqueque_free(rpcbuff_upd);
    struct rpcbuff* buf = NULL;
    while((buf = tqueque_pop(rpcbuff_free,NULL,NULL)) != NULL) _rpcbuff_free(buf);
    tqueque_free(rpcbuff_free);
    free(fnret);
    for(uint8_t i = 0; i < cfn->nargs; i++){
        free(callargs[i]);
    }
    free(callargs);
    return 0;
exit:
    while((buf = tqueque_pop(rpcbuff_free,NULL,NULL)) != NULL) _rpcbuff_free(buf);
    while((buf = tqueque_pop(rpcbuff_upd,NULL,NULL)) != NULL) _rpcbuff_free(buf);
    tqueque_free(rpcbuff_free);
    tqueque_free(rpcbuff_upd);
    for(uint8_t i = 0; i < cfn->nargs; i++){
        free(callargs[i]);
    }
    free(callargs);
    return 1;
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
        char* credbuf = unpack_sizedbuf_type(arr_to_type(gotmsg.payload,&type),&credlen);
        free(gotmsg.payload);
        if(credbuf != NULL){
            int* gotusr = NULL;
            if(hashtable_get(thrd->serv->users,credbuf,credlen,(void**)&gotusr) == 0){
                assert(gotusr);
                is_authed = 1;
                user_perm = *gotusr;
            }
        }
        free(credbuf);
        if(is_authed){
            repl.msg_type = OK;
            rpcmsg_write_to_fd(&repl,thrd->client_fd); repl.msg_type = 0;
            printf("%s: auth ok, OK is replied to client\n",__PRETTY_FUNCTION__);
            while(thrd->serv->stop == 0){
                repl.msg_type = 0; repl.payload = NULL; repl.payload_len = 0;
                gotmsg.msg_type = 0; gotmsg.payload = NULL; gotmsg.payload_len = 0;
                struct rpccall call = {0}; struct rpcret ret = {0};
                if(get_rpcmsg_from_fd(&gotmsg,thrd->client_fd) < 0) {printf("%s: client disconected badly\n",__PRETTY_FUNCTION__);goto exit;}
                switch(gotmsg.msg_type){
                    case DISCON: {free(gotmsg.payload);printf("%s: client disconnected normaly\n",__PRETTY_FUNCTION__); goto exit;}
                    case CALL:
                                    if(buf_to_rpccall(&call,gotmsg.payload) != 0 ){
                                        printf("%s: internal server error or server closing\n",__PRETTY_FUNCTION__);
                                        rpctypes_free(call.args,call.args_amm);
                                        free(call.fn_name);
                                        free(gotmsg.payload);
                                        goto exit;
                                    }
                                    free(gotmsg.payload);
                                    pthread_mutex_lock(&thrd->serv->edit); pthread_mutex_unlock(&thrd->serv->edit);
                                    struct fn* cfn = NULL;hashtable_get(thrd->serv->fn_ht,call.fn_name,strlen(call.fn_name) + 1,(void**)&cfn);
                                    if(cfn == NULL){
                                        repl.msg_type = NOFN;
                                        printf("%s: '%s' no such function\n",__PRETTY_FUNCTION__,call.fn_name);
                                        rpcmsg_write_to_fd(&repl,thrd->client_fd);
                                        free(call.fn_name); rpctypes_free(call.args,call.args_amm);
                                        break;
                                    }else if(cfn->perm > user_perm && user_perm != -1){
                                        repl.msg_type = LPERM;
                                        printf("%s: low permissions, need %d, have %d\n",__PRETTY_FUNCTION__,cfn->perm,(int)user_perm);
                                        rpcmsg_write_to_fd(&repl,thrd->client_fd);
                                        free(call.fn_name);
                                        rpctypes_free(call.args,call.args_amm);
                                        break;
                                    }else repl.msg_type = OK;

                                    if(rpcmsg_write_to_fd(&repl,thrd->client_fd) < 0){
                                        free(call.fn_name);
                                        rpctypes_free(call.args,call.args_amm);
                                        printf("%s: client connection closed\n",__PRETTY_FUNCTION__);
                                        goto exit;
                                    }
                                    if(get_rpcmsg_from_fd(&gotmsg,thrd->client_fd) < 0) {
                                        free(gotmsg.payload);
                                        free(call.fn_name);
                                        rpctypes_free(call.args,call.args_amm);
                                        printf("%s: client disconected badly\n",__PRETTY_FUNCTION__);
                                        goto exit;
                                    }
                                    free(gotmsg.payload);
                                    if(gotmsg.msg_type == NONREADY){printf("%s: client nonready\n",__PRETTY_FUNCTION__);free(call.fn_name); rpctypes_free(call.args,call.args_amm); break;}
                                    if(gotmsg.msg_type == READY){
                                        repl.msg_type = RET;
                                        int err = 0;
                                        int callret = 0;
                                        if((callret = __rpcserver_call_fn(&ret,thrd->serv,&call,cfn,&err)) != 0 && err == 0){
                                            free(call.fn_name);
                                            rpctypes_free(call.args,call.args_amm);
                                            printf("%s: internal server error\n",__PRETTY_FUNCTION__);
                                            goto exit;
                                        }else if(callret != 0 && err != 0){
                                            repl.msg_type = BAD;
                                            printf("%s: client provided wrong arguments\n",__PRETTY_FUNCTION__);
                                            rpcmsg_write_to_fd(&repl,thrd->client_fd);
                                            free(call.fn_name);
                                            rpctypes_free(call.args,call.args_amm);
                                            break;
                                        }
                                        free(call.fn_name);
                                        repl.payload = rpcret_to_buf(&ret,&repl.payload_len);
                                        if(repl.payload == NULL){
                                            if(ret.resargs == NULL) rpctypes_free(call.args,call.args_amm);
                                            else rpctypes_free(ret.resargs,ret.resargs_amm);
                                            if(ret.ret.data) free(ret.ret.data);
                                            printf("%s: internal server error\n",__PRETTY_FUNCTION__);
                                            goto exit;
                                        }
                                        rpcmsg_write_to_fd(&repl,thrd->client_fd);
                                        if(ret.resargs == NULL)
                                            rpctypes_free(call.args,call.args_amm);
                                        else
                                            rpctypes_free(ret.resargs,ret.resargs_amm);
                                        free(repl.payload);
                                        if(ret.ret.data) free(ret.ret.data);
                                        break;
                                    }
                                    free(gotmsg.payload);
                                    free(call.fn_name);
                                    rpctypes_free(call.args,call.args_amm);
                                    printf("%s: client bad reply\n",__PRETTY_FUNCTION__);
                                    goto exit;
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
    //hashtable_remove_entry(thrd->serv->threads,thrd->key,strlen(thrd->key) + 1);
    struct rpcmsg lreply = {DISCON,0,NULL,0};
    rpcmsg_write_to_fd(&lreply,thrd->client_fd);
    close(thrd->client_fd);
    thrd->serv->clientcount--;
    free(thrd);
    pthread_detach(pthread_self());
    pthread_exit(NULL);
}

/*this function must run in another thread*/
void* rpcserver_dispatcher_reliver(void* args){
    void** rargs = args;
    struct rpcserver* serv = (struct rpcserver*)rargs[0];
    int sleep_iter = 0;
    while(1 && serv->stop == 0){
        if(serv->is_incon != 0){
            int* fd = serv->reliverargs[1];
            printf("%s: server picked up client, now waiting\n",__PRETTY_FUNCTION__);
            if (sleep_iter >= DEFAULT_CLIENT_TIMEOUT){
                printf("%s: dispatcher died, starting a new one\n",__PRETTY_FUNCTION__);
                pthread_cancel(serv->accept_thread);
                serv->accept_thread = 0;
                close(*fd);
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                if(pthread_create(&serv->accept_thread, &attr,rpcserver_dispatcher,serv)) exit(-1);
                pthread_attr_destroy(&attr);
                sleep_iter = 0;
            }
            sleep_iter++;
        }
        sleep(1);
    }
    printf("%s: server stopped, exiting\n",__PRETTY_FUNCTION__);
    pthread_exit(NULL);
}
void* rpcserver_dispatcher(void* vserv){
    struct rpcserver* serv = (struct rpcserver*)vserv;
    assert(serv);
    serv->is_incon = 0;
    struct sockaddr_in addr = {0};
    int fd = 0;
    serv->reliverargs[1] = &fd;
    unsigned int addrlen = 0;
    printf("%s: dispatcher started\n",__PRETTY_FUNCTION__);
    while(serv->stop == 0){
        if(serv->clientcount < DEFAULT_MAXIXIMUM_CLIENT){
            fd = accept(serv->sfd, (struct sockaddr*)&addr,&addrlen);
                if(fd < 0) {usleep(10000);continue;}
                printf("%s: got client from %s\n",__PRETTY_FUNCTION__,inet_ntoa(addr.sin_addr));
                serv->is_incon = 1;
                struct rpcmsg msg = {0};
                if(get_rpcmsg_from_fd(&msg,fd) == 0){
                    serv->is_incon = 0;
                    printf("%s: got msg from client\n",__PRETTY_FUNCTION__);
                    if(msg.msg_type == CON){
                        printf("%s: connection ok,passing to client_thread to auth\n",__PRETTY_FUNCTION__);
                        struct client_thread* thrd = malloc(sizeof(*thrd));
                        assert(thrd);
                        thrd->client_fd = fd;
                        thrd->serv = serv;
                        pthread_t client_thread = 0;
                        pthread_attr_t attr; pthread_attr_init(&attr);
                        pthread_create(&client_thread,&attr,rpcserver_client_thread,thrd);
                        pthread_attr_destroy(&attr);
                    }else {printf("%s: client not connected\n",__PRETTY_FUNCTION__);close(fd);memset(&addr,0,sizeof(addr));}
                }else{printf("%s: wrong info from client\n",__PRETTY_FUNCTION__); close(fd);memset(&addr,0,sizeof(addr));}
                if(msg.payload) free(msg.payload);
                serv->is_incon = 0;

        }else{printf("%s:server overloaded\n",__PRETTY_FUNCTION__); sleep(1);}
    }
    printf("%s: dispatcher stopped\n",__PRETTY_FUNCTION__);
    pthread_exit(NULL);
}
void rpcserver_load_keys(struct rpcserver* serv, char* filename){
    FILE* keys = fopen(filename,"r");
    if(!keys){
        perror("rpcserver_load_keys");
        exit(1);
    }
    char buf[512] = {0};
    while(fgets(buf,sizeof(buf),keys)){
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
struct rpcbuff* rpcbuff_test_fn(char* p,struct rpcbuff* buf,char* sz, uint64_t szl, uint64_t i, char* s){
    size_t index[1] = {0};
    printf("%s: %p %s %s %lu %lu %s\n",__PRETTY_FUNCTION__,buf,p,sz,szl,i,s);
    for(; index[0] < buf->dimsizes[0]; index[0]++){
        rpcbuff_pushto(buf,index,sizeof(index) / sizeof(index[0]),p,strlen(p) + 1);
        printf("%s:%s(%lu)\n",__PRETTY_FUNCTION__, rpcbuff_getlast_from(buf,index,sizeof(index) / sizeof(index[0]),NULL),index[0]);
    }
    return buf;
}
int main(){
    struct rpcserver* serv = rpcserver_create(2077);
    serv->interfunc = (void*)0x1488;
    assert(serv);
    enum rpctypes fntype[] = {PSTORAGE,RPCBUFF,SIZEDBUF,UINT64,UINT64,STR};
    puts("crpc server by catmengi\n");
   // hashtable_add(serv->users,"hello da fuck",strlen("hello da fuck") + 1, perm,0);
    rpcserver_register_fn(serv,rpcbuff_test_fn,"buft",RPCBUFF,fntype,sizeof(fntype) / sizeof(fntype[0]),strdup("fuza world"),999);
    rpcserver_load_keys(serv,"keys.txt");
    rpcserver_start(serv);
    printf("server started\n");
    sleep(2);
    getchar();
    rpcserver_free(serv);
}

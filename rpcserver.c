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
#include <unistd.h>
#define DEFAULT_MAXIXIMUM_CLIENT 512
#define _GNU_SOURCE

/*converts rpctypes into suitable for libffi type system*/
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
        if(rpctypes[i] == FINGERPRINT){out[i] = &ffi_type_pointer; continue;}
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
/* same as rpctypes_to_ffi_types but doing it for 1 type, not array of types*/
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
    rpcserver->sfd = server_fd;       //preparing rpcserver structure with basic data
    rpcserver->stop = 0;
    pthread_mutex_init(&rpcserver->edit,NULL);  //creates mutex for editing (register/unregister functions)
    assert(hashtable_create(&rpcserver->fn_ht,32,4) == 0);  //create hashtables for functions and user keys
    assert(hashtable_create(&rpcserver->users,32,4) == 0);
    return rpcserver;
}
void rpcserver_start(struct rpcserver* rpcserver){
    rpcserver->stop = 0;
    assert(pthread_create(&rpcserver->accept_thread, NULL,rpcserver_dispatcher,rpcserver) == 0);   //simply starts main thread, that will pick up any rpc client
}

/*callback for rpcserver_free (made as callback because of ht realiszation)*/
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
    while(serv->clientcount > 0) {printf("%s:clients last:%llu\n",__PRETTY_FUNCTION__,serv->clientcount); sleep(1);}     //wait until all client_threads exit
    pthread_mutex_lock(&serv->edit);                                                                                     //no one can edit server now
    hashtable_iterate(serv->fn_ht,__rpcserver_fn_free_callback);                                                         //iterate hashtable via callback
    hashtable_iterate(serv->users,free);
    hashtable_free(serv->fn_ht);
    hashtable_free(serv->users);
    serv->fn_ht = NULL;
    serv->interfunc = NULL;
    pthread_mutex_unlock(&serv->edit);                                                                                  //unlocks mutex for not potential leaving bunch of mutexes
    shutdown(serv->sfd,SHUT_RD);
    close(serv->sfd);
    pthread_join(serv->accept_thread,NULL);                                                                             //wait until main thread exit
    free(serv);
}

/*cutdown version rpcserver_free*/
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
                          enum rpctypes rtype, enum rpctypes* fn_arguments,
                          uint8_t fn_arguments_len, void* pstorage,int perm){
    pthread_mutex_lock(&serv->edit);
    assert(serv && fn && fn_name);
    struct fn* fnr = malloc(sizeof(*fnr));
    assert(fnr);

    if(fn_arguments_len > 0){
        /*allocating a copying copy of fn prototype*/
        fnr->argtypes = malloc(sizeof(enum rpctypes) * fn_arguments_len);
        assert(fnr->argtypes);
        memcpy(fnr->argtypes, fn_arguments,sizeof(enum rpctypes) * fn_arguments_len);
    }else fnr->argtypes = NULL;

    /*preparing struct fn*/
    fnr->rtype = rtype;
    fnr->nargs = fn_arguments_len;
    fnr->fn = fn;
    fnr->fn_name = malloc(strlen(fn_name) + 1);
    assert(fnr->fn_name);
    memcpy(fnr->fn_name, fn_name, strlen(fn_name)+1);
    fnr->personal = pstorage;
    fnr->perm = perm;
    /*==================*/

    /*converting rpctypes to libffi types*/
    ffi_type** argstype_ffi = rpctypes_to_ffi_types(fn_arguments,fn_arguments_len);
    if(argstype_ffi == NULL && fn_arguments != NULL) return 2;
    fnr->ffi_type_free = argstype_ffi;
    ffi_type* rtype_ffi = rpctype_to_ffi_type(rtype);
    /*==================*/

    /*Finally preparing cif and adding struct fn to hashtable*/
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
static int __rpcserver_call_fn(struct rpcret* ret,struct rpcserver* serv,struct rpccall* call,struct fn* cfn,char* uniq){

    void** callargs = NULL;
    if(cfn->nargs > 0){
        callargs = calloc(cfn->nargs, sizeof(void*));
        assert(callargs);
    }

    struct tqueque* rpcbuff_upd = tqueque_create();     //queque to track struct rpcbuff to be updated (RE-serialize them to apply changes)
    struct tqueque* rpcbuff_freeQ = tqueque_create();   //queque to track struct rpcbuff that need free
    struct tqueque* rpcstruct_upd = tqueque_create();   //queque to track struct rpcstruct to be updated (RE-serialize them to apply changes)
    struct tqueque* rpcstruct_freeQ = tqueque_create(); //queque to track struct rpcstruct that need free
    struct tqueque* strret_free = tqueque_create();     //queue to track do i need to free return if it STR type
    assert(rpcbuff_upd);
    assert(rpcbuff_freeQ);
    assert(rpcstruct_upd);
    assert(rpcstruct_freeQ);
    assert(strret_free);

    enum rpctypes* check = rpctypes_get_types(call->args,call->args_amm);  //checking that arguments supplyed by client match arguments of requested function
    if(!is_rpctypes_equal(cfn->argtypes,cfn->nargs,check,call->args_amm)){
        //if it fails we are freeing types from client, further frees will be in "exit:" or rpcserver_client_thread
        free(check);
        goto exit;
    }
    free(check);
    assert((callargs != NULL && call->args_amm != 0) || call->args_amm == 0);
    uint64_t j = 0;

    /*This is type unpacking monstrosity*/
    for(uint64_t i = 0; i < cfn->nargs; i++){
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
        if(cfn->argtypes[i] == FINGERPRINT){
            callargs[i] = calloc(1,sizeof(void*));
            assert(callargs[i]);
            *(void**)callargs[i] = uniq;
            if(cfn->rtype == STR) tqueque_push(strret_free,*(void**)callargs[i],1,NULL);
            continue;
        }
        // if(j < call->args_amm){
            if(call->args[j].type == RPCBUFF){
                callargs[i] = calloc(1,sizeof(void*));
                assert(callargs[i]);
                *(void**)callargs[i] = unpack_rpcbuff_type(&call->args[j]);
                if(call->args[j].flag == 1)                                    //this flag means "Do i need to resend modified version of this argument to client?"
                    tqueque_push(rpcbuff_upd,*(void**)callargs[i],1,NULL);
                else
                    tqueque_push(rpcbuff_freeQ,*(void**)callargs[i],1,NULL);
                free(call->args[j].data);
                call->args[j].data = NULL;
                j++;
                continue;
            }
            if(call->args[j].type == RPCSTRUCT){
                callargs[i] = calloc(1,sizeof(void*));
                assert(callargs[i]);
                *(void**)callargs[i] = unpack_rpcstruct_type(&call->args[j]);
                if(call->args[j].flag == 1)                                   //this flag means "Do i need to resend modified version of this argument to client?"
                    tqueque_push(rpcstruct_upd,*(void**)callargs[i],1,NULL);
                else
                    tqueque_push(rpcstruct_freeQ,*(void**)callargs[i],1,NULL);
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
                callargs[++i] = calloc(1,sizeof(void*));    //Doing ++i because we need to SKIP next UINT64 type(we are putting len of sizedbuf there)
                assert(callargs[i]);
                *(uint64_t*)callargs[i] = buflen;
                j++;
                continue;
            }
            if(call->args[j].type == STR){
                callargs[i] = calloc(1,sizeof(char*));
                assert(callargs[i]);
                *(void**)callargs[i] =  unpack_str_type(&call->args[j]);
                if(cfn->rtype == STR) tqueque_push(strret_free,*(void**)callargs[i],1,NULL);  //If return type is str we are pushing this string to que, because we need
                j++;                                                                          // to check to free this str or leave it for return
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
        // } else goto exit;
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
        /*Packing return into RPC type*/
        if(rtype == CHAR)    char_to_type(*(char*)fnret,&ret->ret);
        if(rtype == INT16)   int16_to_type(*(int16_t*)fnret,&ret->ret);
        if(rtype == UINT16)  uint16_to_type(*(uint16_t*)fnret,&ret->ret);
        if(rtype == INT32)   int32_to_type(*(int32_t*)fnret,&ret->ret);
        if(rtype == UINT32)  uint32_to_type(*(uint32_t*)fnret,&ret->ret);
        if(rtype == INT64)   int64_to_type(*(int64_t*)fnret,&ret->ret);
        if(rtype == UINT64)  uint64_to_type(*(uint64_t*)fnret,&ret->ret);
        if(rtype == FLOAT)   float_to_type(*(float*)fnret,&ret->ret);
        if(rtype == DOUBLE)  double_to_type(*(double*)fnret,&ret->ret);
        /*===========================*/

        if(rtype == STR && *(void**)fnret != NULL){
            /*This copy-pasted shit used to check do we need that pointer in future (like resending it)*/
            create_str_type(*(char**)fnret,0,&ret->ret);
            int needfree = 1;
            size_t el = tqueque_get_tagamm(strret_free,NULL);   //gets ammount of pointers in que
            for(size_t i = 0; i < el; i++){
                if(!needfree) break;                            //this is just breaked, it exit loop if needfree is set to 0
                void* ptr = tqueque_pop(strret_free,NULL,NULL);
                if(ptr == *(char**)fnret) needfree = 0;         //If this is true it means this pointer was in arguments, and it will be freed in code that used to free arguments

                assert(tqueque_push(strret_free,ptr,sizeof(struct rpcstruct),NULL) == 0);
            }
            if(needfree) free(*(char**)fnret);                  //free it if it wasnt used in arguments
        }else if(rtype == STR && *(void**)fnret == NULL){
            ret->ret.type = VOID;
        }
        if(rtype == RPCBUFF && *(void**)fnret != NULL){
            /*ABSOLUTLY the same code as STR but it checks for rpc*_upd que*/
            int needfree = 1;
            size_t el = tqueque_get_tagamm(rpcbuff_upd,NULL);
            for(size_t i = 0; i < el; i++){
                if(!needfree) break;
                void* ptr = tqueque_pop(rpcbuff_upd,NULL,NULL);
                if(ptr == *(struct rpcbuff**)fnret) needfree = 0;

                assert(tqueque_push(rpcbuff_upd,ptr,sizeof(struct rpcbuff),NULL) == 0);
            }
            el = 0;
            el = tqueque_get_tagamm(rpcbuff_freeQ,NULL);
            for(size_t i = 0; i < el; i++){
                if(!needfree) break;
                void* ptr = tqueque_pop(rpcbuff_freeQ,NULL,NULL);
                if(ptr == *(struct rpcbuff**)fnret) needfree = 0;

                assert(tqueque_push(rpcbuff_freeQ,ptr,sizeof(struct rpcbuff),NULL) == 0);
            }
            create_rpcbuff_type(*(struct rpcbuff**)fnret,0,&ret->ret);
            if(needfree) rpcbuff_free(*(struct rpcbuff**)fnret);
        }
        else if(rtype == RPCBUFF && *(void**)fnret == NULL)
            ret->ret.type = VOID;
        if(rtype == RPCSTRUCT && *(void**)fnret != NULL){
            /*ABSOLUTLY the same code as RPCBUFF but it checks for rpc*_upd que*/
            int needfree = 1;
            size_t el = tqueque_get_tagamm(rpcstruct_upd,NULL);
            for(size_t i = 0; i < el; i++){
                if(!needfree) break;
                void* ptr = tqueque_pop(rpcstruct_upd,NULL,NULL);
                if(ptr == *(struct rpcstruct**)fnret) needfree = 0;

                assert(tqueque_push(rpcstruct_upd,ptr,sizeof(struct rpcstruct),NULL) == 0);
            }
            el = 0;
            el = tqueque_get_tagamm(rpcstruct_freeQ,NULL);
            for(size_t i = 0; i < el; i++){
                if(!needfree) break;
                void* ptr = tqueque_pop(rpcstruct_freeQ,NULL,NULL);
                if(ptr == *(struct rpcstruct**)fnret) needfree = 0;

                assert(tqueque_push(rpcstruct_freeQ,ptr,sizeof(struct rpcstruct),NULL) == 0);
            }
            create_rpcstruct_type(*(struct rpcstruct**)fnret,0,&ret->ret);
            if(needfree) {rpcstruct_free(*(struct rpcstruct**)fnret);}
        }
        else if(rtype == RPCSTRUCT && *(void**)fnret == NULL)
            ret->ret.type = VOID;
    }
    tqueque_free(strret_free);
    ret->resargs = rpctypes_clean_nonres_args(call->args,call->args_amm,&ret->resargs_amm);    //removing ALL not resendable arguments from call->args
    for(uint64_t i = 0; i < ret->resargs_amm; i++){                                            //then we check which types we need to update via rpc*_upd ques
        if(ret->resargs[i].type == RPCBUFF){
            struct rpcbuff* buf = tqueque_pop(rpcbuff_upd,NULL,NULL);
            assert(buf);                                                                       //Asserting because we cant trust in rpcserver anymore.....
            create_rpcbuff_type(buf,ret->resargs[i].flag,&ret->resargs[i]);
            rpcbuff_free(buf);
        }
        if(ret->resargs[i].type == RPCSTRUCT){
            struct rpcstruct* buf = tqueque_pop(rpcstruct_upd,NULL,NULL);
            assert(buf);                                                                       //Asserting because we cant trust in rpcserver anymore.....
            create_rpcstruct_type(buf,ret->resargs[i].flag,&ret->resargs[i]);
            rpcstruct_free(buf);
        }
    }
    tqueque_free(rpcbuff_upd);
    tqueque_free(rpcstruct_upd);
    void* buf = NULL;
    while((buf = tqueque_pop(rpcbuff_freeQ,NULL,NULL)) != NULL) rpcbuff_free(buf);
    while((buf = tqueque_pop(rpcstruct_freeQ,NULL,NULL)) != NULL) {rpcstruct_free(buf);}
    tqueque_free(rpcbuff_freeQ);
    tqueque_free(rpcstruct_freeQ);
    free(fnret);
    for(uint64_t i = 0; i < cfn->nargs; i++){
        free(callargs[i]);
    }
    free(callargs);
    return 0;
exit:
    while((buf = tqueque_pop(rpcbuff_freeQ,NULL,NULL)) != NULL) rpcbuff_free(buf);
    while((buf = tqueque_pop(rpcbuff_upd,NULL,NULL)) != NULL) rpcbuff_free(buf);
    while((buf = tqueque_pop(rpcstruct_upd,NULL,NULL)) != NULL) {rpcstruct_free(buf);}
    while((buf = tqueque_pop(rpcstruct_freeQ,NULL,NULL)) != NULL) {rpcstruct_free(buf);}
    tqueque_free(rpcbuff_freeQ);
    tqueque_free(rpcbuff_upd);
    tqueque_free(rpcstruct_upd);
    tqueque_free(rpcstruct_freeQ);
    tqueque_free(strret_free);
    for(uint64_t i = 0; i < cfn->nargs; i++){
        free(callargs[i]);
    }
    free(callargs);
    return 1;
}
static void __rpcserver_lsfn_create_callback(char* key, void* fn, void* Pusr,size_t unused){

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
        
        assert(rpcstruct_set(usr[1],key,newserv,newservlen,SIZEDBUF) == 0);
        tqueque_free(check_que);
        free(newserv);
    }
}
static char* __rpcserver_lsfn(struct rpcserver* serv,uint64_t* outlen,int user_perm){
    struct rpcstruct* lsfn = rpcstruct_create();
    assert(lsfn);
    void** callback_data[2] = {(void*)&user_perm,(void*)lsfn};
    hashtable_iterate_wkey(serv->fn_ht,callback_data,__rpcserver_lsfn_create_callback);
    struct rpctype otype;
    create_rpcstruct_type(lsfn,0,&otype);
    rpcstruct_free(lsfn);
    *outlen = type_buflen(&otype);
    char* out = malloc(*outlen);
    assert(out);
    type_to_arr(out,&otype);
    free(otype.data);
    return out;
}
static void __get_uniq(char* uniq,int L){
    uniq[L - 1] = '\0';
    char charset[] = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890!@#$%&*";
    srand((unsigned int)time(NULL));
    for(int i = 0; i < L - 1; i++){
        uniq[i] = charset[(rand() % (sizeof(charset) - 1))];
    }
}
void* rpcserver_client_thread(void* arg){
    struct client_thread* thrd = (struct client_thread*)arg;
    struct rpcmsg gotmsg = {0},reply = {0};
    thrd->serv->clientcount++;
    int user_perm = 0;
    int is_authed = 0;
    char cipher[16] = {0};
    if(get_rpcmsg(&gotmsg,thrd->client_fd,NULL) == 0 && gotmsg.msg_type == AUTH && gotmsg.payload != NULL){
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
                char* got_user_key = NULL;
                if(hashtable_get_key_by_hash(thrd->serv->users,hash,&got_user_key) != 0) is_authed = 1;
                memcpy(cipher,got_user_key,(strlen(got_user_key) > 16 ? 16 : strlen(got_user_key)));
            }
        }
        free(type.data);
        if(is_authed){
            reply.msg_type = OK;
            __get_uniq(thrd->client_uniq,sizeof(thrd->client_uniq));
            struct rpctype uniq = {0};
            create_str_type(thrd->client_uniq,0,&uniq);
            reply.payload = malloc((reply.payload_len = type_buflen(&uniq)));
            assert(reply.payload);
            type_to_arr(reply.payload,&uniq);
            free(uniq.data);
            if(send_rpcmsg(&reply,thrd->client_fd,NULL) != 0) goto exit;
            printf("%s: auth ok, OK is replyied to client (%s)\n",__PRETTY_FUNCTION__,thrd->client_uniq);
            if(thrd->serv->newclient_cb != NULL)
                thrd->serv->newclient_cb(thrd->serv->newclient_cb_data,thrd->client_uniq,thrd->addr);


            while(thrd->serv->stop == 0){
                memset(&reply,0,sizeof(reply));
                struct rpccall call = {0}; struct rpcret ret = {0};
                if(get_rpcmsg(&gotmsg,thrd->client_fd,(uint8_t*)cipher) != 0){
                    printf("%s: disconected: %s(%s)\n",__PRETTY_FUNCTION__,inet_ntoa(thrd->addr.sin_addr),thrd->client_uniq);
                    goto exit;
                }
                switch(gotmsg.msg_type){
                    case LSFN:
                                    printf("%s: client requested list of registred functions!\n",__PRETTY_FUNCTION__);
                                    reply.payload = __rpcserver_lsfn(thrd->serv,&reply.payload_len,user_perm);
                                    reply.msg_type = LSFN;
                                    if(send_rpcmsg(&reply,thrd->client_fd,(uint8_t*)cipher) != 0) goto exit;
                                    break;
                    case PING:
                                    reply.msg_type = PING;
                                    if(send_rpcmsg(&reply,thrd->client_fd,(uint8_t*)cipher) != 0) goto exit;
                                    break;
                    case DISCON:
                                    free(gotmsg.payload);
                                    printf("%s: client (%s) disconnected normaly\n",__PRETTY_FUNCTION__,thrd->client_uniq);
                                    goto exit;
                    case CALL:
                                    if(buf_to_rpccall(&call,gotmsg.payload) != 0 ){
                                        printf("%s: bad 'CALL' msg! \n",__PRETTY_FUNCTION__);
                                        free(gotmsg.payload);
                                        free(call.fn_name);
                                        goto exit;
                                    }
                                    free(gotmsg.payload);
                                    printf("%s: client (%s) called function: '%s'\n",__PRETTY_FUNCTION__,thrd->client_uniq,call.fn_name);
                                    pthread_mutex_lock(&thrd->serv->edit); pthread_mutex_unlock(&thrd->serv->edit);
                                    struct fn* cfn = NULL;hashtable_get(thrd->serv->fn_ht,call.fn_name,strlen(call.fn_name) + 1,(void**)&cfn);
                                    if(cfn == NULL){
                                        reply.msg_type = NOFN;
                                        printf("%s: '%s' no such function\n",__PRETTY_FUNCTION__,call.fn_name);
                                        free(call.fn_name); rpctypes_free(call.args,call.args_amm);
                                        if(send_rpcmsg(&reply,thrd->client_fd,(uint8_t*)cipher) != 0) goto exit;
                                        break;
                                    }else if((cfn->perm > user_perm || cfn->perm == -1) && user_perm != -1){
                                        reply.msg_type = LPERM;
                                        printf("%s: low permissions, need %d, have %d\n",__PRETTY_FUNCTION__,cfn->perm,(int)user_perm);
                                        free(call.fn_name);
                                        rpctypes_free(call.args,call.args_amm);
                                        if(send_rpcmsg(&reply,thrd->client_fd,(uint8_t*)cipher) != 0) goto exit;
                                        break;
                                    }
                                    reply.msg_type = RET;
                                    int callret = 0;
                                    if((callret = __rpcserver_call_fn(&ret,thrd->serv,&call,cfn,thrd->client_uniq)) != 0){
                                        reply.msg_type = BAD;
                                        printf("%s: client provided wrong arguments\n",__PRETTY_FUNCTION__);
                                        free(call.fn_name);
                                        rpctypes_free(call.args,call.args_amm);
                                        if(send_rpcmsg(&reply,thrd->client_fd,(uint8_t*)cipher) != 0) goto exit;
                                        break;
                                    }
                                    printf("%s: client (%s) call of '%s' succeded\n",__PRETTY_FUNCTION__,thrd->client_uniq,call.fn_name);
                                    free(call.fn_name);
                                    reply.payload = rpcret_to_buf(&ret,&reply.payload_len);
                                    rpctypes_free(ret.resargs,ret.resargs_amm);
                                    int iserror = 0;
                                    if(send_rpcmsg(&reply,thrd->client_fd,(uint8_t*)cipher) != 0) iserror = 1;
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
        }else {reply.msg_type = BAD;send_rpcmsg(&reply,thrd->client_fd,(uint8_t*)cipher);printf("%s: client not passed auth\n",__PRETTY_FUNCTION__);}
    } else printf("%s: no auth provided\n",__PRETTY_FUNCTION__);

exit:
    if(thrd->serv->stop == 1)
        printf("%s: disconecting: %s(%s)\n",__PRETTY_FUNCTION__,inet_ntoa(thrd->addr.sin_addr),thrd->client_uniq);
    shutdown(thrd->client_fd, SHUT_RD);
    close(thrd->client_fd);
    thrd->serv->clientcount--;
    if(thrd->serv->clientdiscon_cb != NULL)
        thrd->serv->clientdiscon_cb(thrd->serv->clientdiscon_cb_data,thrd->client_uniq,thrd->addr);
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
                printf("\n%s: got client from %s\n",__PRETTY_FUNCTION__,inet_ntoa(addr.sin_addr));
                struct timeval time;
                time.tv_sec = 5;
                time.tv_usec = 0;
                assert(setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&time,sizeof(time)) == 0);
                assert(setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&time,sizeof(time)) == 0);
                struct rpcmsg msg = {0};
                if(get_rpcmsg(&msg,fd,NULL) == 0){
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
        }else{printf("%s:server overloaded\n",__PRETTY_FUNCTION__); sleep(1);}
    }
    printf("%s: dispatcher stopped\n",__PRETTY_FUNCTION__);
    pthread_exit(NULL);
}
void rpcserver_register_newclient_cb(struct rpcserver* serv,rpcserver_client_cb callback, void* user){
    assert(serv);
    serv->newclient_cb = callback;
    serv->newclient_cb_data = user;
}
void rpcserver_register_clientdiscon_cb(struct rpcserver* serv,rpcserver_client_cb callback, void* user){
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
        if(hashtable_add(serv->users,key_start,strlen(key_start) + 1,lperm,0) != 0) break;
    }
    fclose(keys);
}
void rpcserver_add_key(struct rpcserver* serv, char* key, int perm){
    if(key == NULL || serv == NULL) return;
    int* lperm = malloc(sizeof(perm));
    assert(lperm);
    memcpy(lperm,&perm,sizeof(perm));
    hashtable_add(serv->users,key,strlen(key) + 1,lperm,0);
}
void rpcserver_set_interfunc(struct rpcserver* self, void* interfunc){
    assert(self);
    self->interfunc = interfunc;
}

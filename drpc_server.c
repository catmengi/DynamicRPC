#include "drpc_struct.h"
#include "drpc_types.h"
#include "drpc_protocol.h"
#include "drpc_server.h"
#include "drpc_que.h"
#include "drpc_queue.h"
#include "hashtable.c/hashtable.h"


#include <assert.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <ffi.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>


#define MAX_LISTEN 512

void* drpc_server_dispatcher(void* drpc_server_P);

struct drpc_server* new_drpc_server(uint16_t port){
    struct drpc_server* drpc_serv = calloc(1,sizeof(*drpc_serv)); assert(drpc_serv);

    hashtable_create(&drpc_serv->functions,8,2);
    hashtable_create(&drpc_serv->users,8,2);

    drpc_serv->port = port;

    return drpc_serv;
}

void drpc_server_start(struct drpc_server* server){
    struct sockaddr_in addr = {
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(server->port),
        .sin_family = AF_INET,
    };
    server->server_fd = socket(AF_INET,SOCK_STREAM,0);
    assert(server->server_fd > 0);

    int opt = 1;
    assert(setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,sizeof(opt)) == 0);
    assert(bind(server->server_fd,(struct sockaddr*)&addr,sizeof(addr)) == 0);
    assert(listen(server->server_fd,MAX_LISTEN) == 0);

    assert(pthread_create(&server->dispatcher,NULL,drpc_server_dispatcher,server) == 0);
}

void drpc_fn_info_free_CB(void* fn_info_P){
    struct drpc_function* fn_info = fn_info_P;

    free(fn_info->cif);
    free(fn_info->fn_name);
    free(fn_info->ffi_prototype);
    free(fn_info->prototype);

    drpc_que_free(fn_info->delayed_massage_que);

    free(fn_info);
}

void drpc_server_free(struct drpc_server* server){
    server->should_stop = 1;
    while(server->client_ammount != 0) sleep(1);

    shutdown(server->server_fd, SHUT_RD);
    close(server->server_fd);
    pthread_join(server->dispatcher,NULL);

    hashtable_iterate(server->functions,drpc_fn_info_free_CB);
    hashtable_free(server->functions);

    hashtable_iterate(server->users, free);
    hashtable_free(server->users);

    free(server);
}

void drpc_server_register_fn(struct drpc_server* server,char* fn_name, void* fn,
                             enum drpc_types return_type, enum drpc_types* prototype,
                             size_t prototype_len, void* pstorage, int perm){
    assert(return_type != d_sizedbuf   || return_type != d_fn_pstorage
        || return_type != d_clientinfo || return_type != d_interfunc
        || return_type != d_delayed_massage_queue);
    struct drpc_function* fn_info = calloc(1,sizeof(*fn_info)); assert(fn_info);

    fn_info->delayed_massage_que = NULL;
    fn_info->fn_name = strdup(fn_name);
    fn_info->fn = fn;
    fn_info->minimal_permission_level = perm;
    fn_info->return_type = return_type;
    fn_info->personal = pstorage;
    if(prototype != NULL){
        fn_info->prototype_len = prototype_len;
        fn_info->prototype = calloc(fn_info->prototype_len, sizeof(enum drpc_types));
        memcpy(fn_info->prototype,prototype, sizeof(enum drpc_types) * prototype_len);
    }
    assert(hashtable_add(server->functions,fn_name, strlen(fn_name) + 1,fn_info,0) == 0);

}

enum drpc_types* drpc_types_extract_prototype(struct drpc_type* drpc_types,size_t drpc_types_len){
    if(drpc_types == NULL) return NULL;
    enum drpc_types* ret = malloc(drpc_types_len * sizeof(enum drpc_types));
    for(uint64_t i = 0; i < drpc_types_len; i++){
        ret[i] = drpc_types[i].type;
    }
    return ret;
}

int is_arguments_equal_prototype(enum drpc_types* serv, size_t servlen, enum drpc_types* client, size_t clientlen){
    if(!serv && !client) return 0;
    struct drpc_que* check_que = drpc_que_create();
    assert(check_que);
    size_t newservlen = 0;
    for(size_t i = 0; i < servlen;i++){
        if(serv[i] != d_interfunc && serv[i] != d_fn_pstorage && serv[i] != d_clientinfo && serv[i] != d_delayed_massage_queue){
            drpc_que_push(check_que,&serv[i]);
            newservlen++;
        }
    }
    if((serv && !client) || (!serv && client)) {
        if(clientlen == 0 && newservlen == 0){
            drpc_que_free(check_que);
            return 0;
        }
        drpc_que_free(check_que);
        return 1;
    }
    if(newservlen != clientlen) {drpc_que_free(check_que);return 1;}
    enum drpc_types* newserv = calloc(newservlen,sizeof(enum drpc_types));
    assert(newserv);
    for(size_t i = 0; i < newservlen; i++){
        newserv[i] = *(enum drpc_types*)drpc_que_pop(check_que);
    }
    int ret = 0;
    for(size_t i = 0; i < clientlen; i++)
        if(newserv[i] != client[i]) {ret = 1;break;}
    drpc_que_free(check_que);
    free(newserv);
    return ret;
}


void** ffi_from_drpc(struct drpc_type* arguments,enum drpc_types* prototype,size_t prototype_len,size_t* ffi_len,struct drpc_que* to_repack, struct drpc_que* fill_later){
    size_t adjusted_len = drpc_proto_to_ffi_len_adjust(prototype,prototype_len);
    void** ffi_arguments = calloc(adjusted_len, sizeof(void*)); assert(ffi_arguments);

    *ffi_len = adjusted_len;
    size_t j = 0; size_t k = 0;
    for(size_t i = 0; i < prototype_len; i++){
        if(prototype[i] == d_fn_pstorage){
            ffi_arguments[k] = calloc(1,sizeof(void*));
            assert(ffi_arguments[k]);
            struct drpc_type_update* fill_later_info = calloc(1,sizeof(*fill_later_info)); assert(fill_later_info);
            fill_later_info->type = d_fn_pstorage;
            fill_later_info->ptr = &ffi_arguments[k];
            drpc_que_push(fill_later,fill_later_info);
            k++;
            continue;
        }
        if(prototype[i] == d_clientinfo){
            ffi_arguments[k] = calloc(1,sizeof(void*));
            assert(ffi_arguments[k]);
            struct drpc_type_update* fill_later_info = calloc(1,sizeof(*fill_later_info)); assert(fill_later_info);
            fill_later_info->type = d_clientinfo;
            fill_later_info->ptr = &ffi_arguments[k];
            drpc_que_push(fill_later,fill_later_info);
            k++;
            continue;
        }
        if(prototype[i] == d_interfunc){
            ffi_arguments[k] = calloc(1,sizeof(void*));
            assert(ffi_arguments[k]);
            struct drpc_type_update* fill_later_info = calloc(1,sizeof(*fill_later_info)); assert(fill_later_info);
            fill_later_info->type = d_interfunc;
            fill_later_info->ptr = &ffi_arguments[k];
            drpc_que_push(fill_later,fill_later_info);
            k++;
            continue;
        }
        if(prototype[i] == d_delayed_massage_queue){
            ffi_arguments[k] = calloc(1,sizeof(void*));
            assert(ffi_arguments[k]);
            struct drpc_type_update* fill_later_info = calloc(1,sizeof(*fill_later_info)); assert(fill_later_info);
            fill_later_info->type = d_delayed_massage_queue;
            fill_later_info->ptr = &ffi_arguments[k];
            drpc_que_push(fill_later,fill_later_info);
            k++;
            continue;
        }
            if(arguments[j].type == d_array){
                ffi_arguments[k] = calloc(1,sizeof(void*));
                assert(ffi_arguments[k]);
                *(void**)ffi_arguments[k] = drpc_to_d_array(&arguments[j]);

                struct drpc_type_update* update = calloc(1,sizeof(*update));
                update->type = d_array;
                update->ptr = *(void**)ffi_arguments[k];
                drpc_que_push(to_repack,update);

                j++; k++;
                continue;
            }
            if(arguments[j].type == d_struct){
                ffi_arguments[k] = calloc(1,sizeof(void*));
                assert(ffi_arguments[k]);
                *(void**)ffi_arguments[k] = drpc_to_d_struct(&arguments[j]);

                struct drpc_type_update* update = calloc(1,sizeof(*update));
                update->type = d_struct;
                update->ptr = *(void**)ffi_arguments[k];
                drpc_que_push(to_repack,update);

                j++;k++;
                continue;
            }
            if(arguments[j].type == d_queue){
                ffi_arguments[k] = calloc(1,sizeof(void*));
                assert(ffi_arguments[k]);
                *(void**)ffi_arguments[k] = drpc_to_d_queue(&arguments[j]);

                struct drpc_type_update* update = calloc(1,sizeof(*update));
                update->type = d_queue;
                update->ptr = *(void**)ffi_arguments[k];
                drpc_que_push(to_repack,update);

                j++;k++;
                continue;
            }
            if(arguments[j].type == d_str){
                ffi_arguments[k] = calloc(1,sizeof(void*));
                assert(ffi_arguments[k]);
                *(void**)ffi_arguments[k] = drpc_to_str(&arguments[j]);

                struct drpc_type_update* update = calloc(1,sizeof(*update));
                update->type = d_str;
                update->ptr = *(void**)ffi_arguments[k];
                drpc_que_push(to_repack,update);

                j++;k++;
                continue;
            }
            if(arguments[j].type == d_sizedbuf){
                ffi_arguments[k] = calloc(1,sizeof(void*));
                assert(ffi_arguments[k]);

                size_t sizedbuf_len = 0;
                *(void**)ffi_arguments[k] = drpc_to_sizedbuf(&arguments[j],&sizedbuf_len);

                struct drpc_type_update* update = calloc(1,sizeof(*update));
                update->type = d_sizedbuf;
                update->ptr = *(void**)ffi_arguments[k];
                update->len = sizedbuf_len;
                k++;
                ffi_arguments[k] = calloc(1,sizeof(size_t));
                assert(ffi_arguments[k]);
                *(size_t*)ffi_arguments[k] = sizedbuf_len;

                drpc_que_push(to_repack,update);

                j++;k++;
                continue;
            }
            if(arguments[j].type == d_int8){
                ffi_arguments[k] = calloc(1,sizeof(int8_t*));
                assert(ffi_arguments[k]);
                *(int8_t*)ffi_arguments[k] = drpc_to_int8(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_uint8){
                ffi_arguments[k] = calloc(1,sizeof(uint8_t*));
                assert(ffi_arguments[k]);
                *(uint8_t*)ffi_arguments[k] = drpc_to_uint8(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_int16){
                ffi_arguments[k] = calloc(1,sizeof(int16_t*));
                assert(ffi_arguments[k]);
                *(int16_t*)ffi_arguments[k] = drpc_to_int16(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_uint16){
                ffi_arguments[k] = calloc(1,sizeof(uint16_t*));
                assert(ffi_arguments[k]);
                *(uint16_t*)ffi_arguments[k] = drpc_to_uint16(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_int32){
                ffi_arguments[k] = calloc(1,sizeof(int32_t*));
                assert(ffi_arguments[k]);
                *(int32_t*)ffi_arguments[k] = drpc_to_int32(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_uint32){
                ffi_arguments[k] = calloc(1,sizeof(uint32_t*));
                assert(ffi_arguments[k]);
                *(uint32_t*)ffi_arguments[k] = drpc_to_uint32(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_int64){
                ffi_arguments[k] = calloc(1,sizeof(int64_t*));
                assert(ffi_arguments[k]);
                *(int64_t*)ffi_arguments[k] = drpc_to_int64(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_uint64){
                ffi_arguments[k] = calloc(1,sizeof(uint32_t*));
                assert(ffi_arguments[k]);
                *(uint64_t*)ffi_arguments[k] = drpc_to_uint64(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_float){
                ffi_arguments[k] = calloc(1,sizeof(float*));
                assert(ffi_arguments[k]);
                *(float*)ffi_arguments[k] = drpc_to_float(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_double){
                ffi_arguments[k] = calloc(1,sizeof(double*));
                assert(ffi_arguments[k]);
                *(double*)ffi_arguments[k] = drpc_to_double(&arguments[j]);
                j++;k++;
                continue;
            }
    }
    return ffi_arguments;
}

int drpc_server_call_fn(struct drpc_type* arguments,uint8_t arguments_len, struct drpc_function* fn_info, struct drpc_connection* client_info, struct drpc_return* returned){
    enum drpc_types* extracted_prototype = drpc_types_extract_prototype(arguments,arguments_len);
    if(is_arguments_equal_prototype(fn_info->prototype,fn_info->prototype_len,extracted_prototype,arguments_len)){
        free(extracted_prototype);
        return 1;
    }
    struct drpc_que* to_repack = drpc_que_create();
    struct drpc_que* to_fill = drpc_que_create();

    size_t ffi_len = 0;
    ffi_arg native_return = 0;
    int8_t return_is = -1;

    void** ffi_arguments = ffi_from_drpc(arguments,fn_info->prototype,fn_info->prototype_len,&ffi_len,to_repack,to_fill);
    if(fn_info->cif == NULL){
        fn_info->cif = calloc(1,sizeof(*fn_info->cif)); assert(fn_info->cif);
        assert(ffi_prep_cif(fn_info->cif,FFI_DEFAULT_ABI,ffi_len,(ffi_type*)drpc_ffi_convert_table[fn_info->return_type],
                    (fn_info->ffi_prototype = drpc_proto_to_ffi(fn_info->prototype, fn_info->prototype_len))) == FFI_OK);
    }
    free(extracted_prototype);

    size_t to_fill_len = drpc_que_get_len(to_fill);
    for(size_t i = 0; i <to_fill_len; i++){
        struct drpc_type_update* to_update = drpc_que_pop(to_fill);
        switch(to_update->type){
            case d_fn_pstorage:
                **(void***)to_update->ptr = fn_info->personal;
                break;
            case d_interfunc:
                **(void***)to_update->ptr = client_info->drpc_server->interfunc;
                break;
            case d_clientinfo:
                **(void***)to_update->ptr = client_info;
                break;
            case d_delayed_massage_queue:
                **(void***)to_update->ptr = fn_info->delayed_massage_que;
            default:
                break;
        }
        free(to_update);
    }
    // if(arguments_len == 0) free(arguments);
    drpc_types_free(arguments,arguments_len);
    ffi_call(fn_info->cif,FFI_FN(fn_info->fn),&native_return,ffi_arguments);

    size_t repack_len = drpc_que_get_len(to_repack);

    if(repack_len > 0){
        returned->updated_arguments = calloc(repack_len,sizeof(*returned->updated_arguments));
        assert(returned->updated_arguments);
    }else returned->updated_arguments = NULL;
    returned->updated_arguments_len = repack_len;

    for(size_t i = 0; i < repack_len; i++){
        struct drpc_type_update* repack = drpc_que_pop(to_repack);
        if(repack->ptr == (void*)native_return) {
            assert(repack->type == fn_info->return_type);    //dumb protection, you SHOULDNT return argument that is different type than return_type
            return_is = i;
        }

        switch(repack->type){
            case d_str:
                str_to_drpc(&returned->updated_arguments[i],repack->ptr);
                free(repack->ptr);
                break;
            case d_sizedbuf:
                sizedbuf_to_drpc(&returned->updated_arguments[i],repack->ptr,repack->len);
                free(repack->ptr);
                break;
            case d_struct:
                d_struct_to_drpc(&returned->updated_arguments[i],repack->ptr);
                d_struct_free(repack->ptr);
                break;
            case d_queue:
                d_queue_to_drpc(&returned->updated_arguments[i],repack->ptr);
                d_queue_free(repack->ptr);
                break;
            case d_array:
                d_array_to_drpc(&returned->updated_arguments[i],repack->ptr);
                //TODO: d_array_free(repack->ptr);
                break;
            default: break;
        }
        free(repack);

    }

    for(size_t i = 0; i <ffi_len; i++){
        free(ffi_arguments[i]);
    }
    free(ffi_arguments);

    if(return_is == -1){
        switch(fn_info->return_type){
            case d_void:
                void_to_drpc(&returned->returned);
                break;
            case d_int8:
                int8_to_drpc(&returned->returned,(int8_t)native_return);
                break;
            case d_uint8:
                uint8_to_drpc(&returned->returned,(uint8_t)native_return);
                break;
            case d_int16:
                int16_to_drpc(&returned->returned,(int16_t)native_return);
                break;
            case d_uint16:
                uint16_to_drpc(&returned->returned,(uint16_t)native_return);
                break;
            case d_int32:
                int32_to_drpc(&returned->returned,(int32_t)native_return);
                break;
            case d_uint32:
                uint32_to_drpc(&returned->returned,(uint32_t)native_return);
                break;
            case d_int64:
                int64_to_drpc(&returned->returned,(int64_t)native_return);
                break;
            case d_uint64:
                uint64_to_drpc(&returned->returned,(uint64_t)native_return);
                break;

            case d_float:
                float_to_drpc(&returned->returned,(float)native_return);
                break;
            case d_double:
                double_to_drpc(&returned->returned,(double)native_return);
                break;

            case d_str:
                if((char*)native_return == NULL) void_to_drpc(&returned->returned);
                else                             str_to_drpc(&returned->returned,(char*)native_return);
                free((char*)native_return);
                break;

            case d_array:
                if((char*)native_return == NULL) void_to_drpc(&returned->returned);
                else                             d_array_to_drpc(&returned->returned,(void*)native_return);
                //d_array_free((void*)native_return);
                break;
            case d_struct:
                if((char*)native_return == NULL) void_to_drpc(&returned->returned);
                else                             d_struct_to_drpc(&returned->returned,(void*)native_return);
                d_struct_free((void*)native_return);
                break;
            case d_queue:
                if((char*)native_return == NULL) void_to_drpc(&returned->returned);
                else                             d_queue_to_drpc(&returned->returned,(void*)native_return);
                d_queue_free((void*)native_return);
            break;
            default: break;
        }
    }else{
        return_is_to_drpc(&returned->returned,return_is);
    }
    drpc_que_free(to_repack);
    drpc_que_free(to_fill);
    return 0;
}

void drpc_handle_client(struct drpc_connection* client, int client_perm){
    struct drpc_massage recv;
    struct drpc_massage send;

    while(client->drpc_server->should_stop == 0){
        if(drpc_recv_massage(&recv,client->fd) != 0) {
            puts("");
            printf("%s: no massage provided,exiting\n",__PRETTY_FUNCTION__);
            return;
        }

        switch(recv.massage_type){
            case drpc_ping:
                send.massage = NULL;
                send.massage_type = drpc_ping;
                if(drpc_send_massage(&send,client->fd) != 0) return;
                break;
            case drpc_disconnect:
                puts("");
                printf("%s: client disconnected\n",__PRETTY_FUNCTION__);
                return;
            case drpc_send_delayed:
                struct d_struct* massage = NULL;
                char* fn_name = NULL;
                puts("");
                printf("%s: delayed massage sent!\n",__PRETTY_FUNCTION__);

                assert(d_struct_get(recv.massage,"payload",&massage,d_struct) == 0 && d_struct_get(recv.massage,"fn_name",&fn_name,d_str) == 0);

                struct drpc_function* fn_info = NULL;
                if(hashtable_get(client->drpc_server->functions,fn_name,strlen(fn_name) + 1,(void**)&fn_info) != 0){
                    printf("%s: no such function for drpc_send_delayed(%s)\n",__PRETTY_FUNCTION__,fn_name);
                    send.massage = NULL;
                    send.massage_type = drpc_bad;
                    drpc_send_massage(&send,client->fd);
                    d_struct_free(recv.massage);
                    break;
                }
                if(fn_info->delayed_massage_que == NULL){
                    fn_info->delayed_massage_que = drpc_que_create();
                }

                d_struct_unlink(recv.massage,"payload",d_struct);
                d_struct_free(recv.massage);

                struct drpc_delayed_massage* to_push = malloc(sizeof(*to_push)); assert(to_push);

                to_push->client = strdup(client->username);
                to_push->massage = massage;

                drpc_que_push(fn_info->delayed_massage_que, to_push);

                send.massage = NULL;
                send.massage_type = drpc_ok;
                drpc_send_massage(&send,client->fd);
                break;
            case drpc_call:
                puts("");
                struct drpc_call* call = massage_to_drpc_call(recv.massage);
                assert(call != NULL);
                d_struct_free(recv.massage);

                printf("%s: requested to call %s\n",__PRETTY_FUNCTION__,call->fn_name);

                struct drpc_function* call_fn = NULL;
                if(hashtable_get(client->drpc_server->functions,call->fn_name,strlen(call->fn_name) + 1,(void**)&call_fn) != 0){
                    printf("%s: no such function!\n",__PRETTY_FUNCTION__);
                    drpc_call_free(call);
                    free(call);
                    send.massage = NULL;
                    send.massage_type = drpc_nofn;
                    drpc_send_massage(&send,client->fd);
                    break;
                }

                struct drpc_return ret;
                if(client_perm > call_fn->minimal_permission_level || client_perm == -1){
                    if(drpc_server_call_fn(call->arguments,call->arguments_len,call_fn,client,&ret) != 0){
                        printf("%s: bad function call! \n",__PRETTY_FUNCTION__);
                        drpc_call_free(call);
                        free(call);
                        send.massage = NULL;
                        send.massage_type = drpc_bad;
                        drpc_send_massage(&send,client->fd);
                        break;
                    }
                    printf("%s: call of %s succesfull \n",__PRETTY_FUNCTION__,call->fn_name);
                    free(call->fn_name);
                    free(call);


                    send.massage_type = drpc_return;
                    send.massage = drpc_return_to_massage(&ret);
                    drpc_return_free(&ret);
                    if(drpc_send_massage(&send,client->fd) != 0){
                        printf("%s: unable to send return!\n",__PRETTY_FUNCTION__);
                        d_struct_free(send.massage);
                        return;
                    }
                    d_struct_free(send.massage);
                }else{
                    printf("%s: user permission is too low for %s (have: %d, require %d)!\n",__PRETTY_FUNCTION__,fn_info->fn_name, client_perm,fn_info->minimal_permission_level);
                    drpc_call_free(call);
                    free(call);
                    send.massage = NULL;
                    send.massage_type = drpc_eperm;
                    drpc_send_massage(&send,client->fd);
                }
                break;

            default:
                printf("%s: client provided bad request\n",__PRETTY_FUNCTION__);
                d_struct_free(recv.massage);
                return;
        }
    }
}

void* drpc_server_client_auth(void* drpc_connection_P){
   struct drpc_connection* client = drpc_connection_P;
   pthread_detach(pthread_self());

   struct drpc_massage recv;
   struct drpc_massage send;
   int perm = 0;
   puts("");
   if(drpc_recv_massage(&recv,client->fd) != 0){
       printf("%s: no auth request!\n",__PRETTY_FUNCTION__);
       goto exit;
   }
   if(recv.massage_type != drpc_auth || recv.massage == NULL){
       send.massage_type = drpc_bad;
       send.massage = NULL;
       drpc_send_massage(&send,client->fd);
       printf("%s: request is not auth or malformed!\n",__PRETTY_FUNCTION__);
       goto exit;
   }else{
       char* username;
       uint64_t hash;
       struct drpc_user* user;
       if(d_struct_get(recv.massage,"username",&username,d_str) != 0){
           printf("%s: auth malformed\n",__PRETTY_FUNCTION__);
           d_struct_free(recv.massage);
           send.massage_type = drpc_bad;
           send.massage = NULL;
           drpc_send_massage(&send,client->fd);
           goto exit;
       }
       if(d_struct_get(recv.massage,"passwd_hash",&hash,d_uint64) != 0){
           printf("%s: auth malformed\n",__PRETTY_FUNCTION__);
           d_struct_free(recv.massage);
           send.massage_type = drpc_bad;
           send.massage = NULL;
           drpc_send_massage(&send,client->fd);
           goto exit;
       }
       d_struct_unlink(recv.massage,"username",d_str);
       d_struct_free(recv.massage);
       if(hashtable_get(client->drpc_server->users,username,strlen(username) + 1,(void**)&user) != 0){
           printf("%s: no such username : %s\n",__PRETTY_FUNCTION__,username);
           send.massage_type = drpc_bad;
           send.massage = NULL;
           free(username);
           drpc_send_massage(&send,client->fd);
           goto exit;
       }
       if(user->hash != hash){
           printf("%s: wrong password for : %s\n",__PRETTY_FUNCTION__,username);
           send.massage_type = drpc_bad;
           send.massage = NULL;
           free(username);
           drpc_send_massage(&send,client->fd);
           goto exit;
       }
       perm = user->perm;
       client->username = username;

       send.massage = NULL;
       send.massage_type = drpc_ok;
       drpc_send_massage(&send,client->fd);
       printf("%s: client '%s' authenticated succesfully\n",__PRETTY_FUNCTION__,client->username);
   }
   client->drpc_server->client_ammount++;
   drpc_handle_client(client,perm);
   client->drpc_server->client_ammount--;
exit:
   shutdown(client->fd,SHUT_RD);
   close(client->fd);
   free(client->username);
   free(client);
   return NULL;
}



void* drpc_server_dispatcher(void* drpc_server_P){
    struct drpc_server* server = drpc_server_P;
    printf("%s: started\n",__PRETTY_FUNCTION__);
    while(server->should_stop == 0){
        socklen_t client_addr_len = sizeof(struct sockaddr_in);
        struct sockaddr_in client_addr;
        int client_fd = accept(server->server_fd,(struct sockaddr*)&client_addr,&client_addr_len);
        if(client_fd > 0){

            printf("%s: picked up client: %s\n",__PRETTY_FUNCTION__,inet_ntoa(client_addr.sin_addr));

            struct drpc_connection* client = calloc(1,sizeof(*client)); assert(client);
            client->client_addr = client_addr;
            client->drpc_server = server;
            client->fd = client_fd;

            struct timeval time;
            time.tv_sec = 5;
            time.tv_usec = 0;
            assert(setsockopt(client_fd,SOL_SOCKET,SO_RCVTIMEO,&time,sizeof(time)) == 0);
            assert(setsockopt(client_fd,SOL_SOCKET,SO_SNDTIMEO,&time,sizeof(time)) == 0);

            pthread_t client_auth;
            assert(pthread_create(&client_auth,NULL,drpc_server_client_auth,client) == 0);
        }else{
            break;
        }
    }
    pthread_detach(pthread_self());
    return NULL;
}
void drpc_server_add_user(struct drpc_server* serv, char* username,char* passwd, int perm){
    struct drpc_user* user = malloc(sizeof(*user)); assert(user);

    user->hash = _hash_fnc(username,strlen(passwd));
    user->perm = perm;

    hashtable_add(serv->users,username,strlen(username) + 1, user,0);
}

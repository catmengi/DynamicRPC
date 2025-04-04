#include "drpc_struct.h"
#include "drpc_types.h"
#include "drpc_protocol.h"
#include "drpc_server.h"
#include "queue.h"
#include "drpc_queue.h"
#include "drpc_array.h"
#include "hashtable.c/hashtable.h"


#include <assert.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <ffi.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>


#define MAX_LISTEN 512



/*=== server only libffi stuff ===*/

static ffi_type* drpc_ffi_convert_table[d_return_is] =
{
    &ffi_type_void,     &ffi_type_sint8,
    &ffi_type_uint8,    &ffi_type_sint16,
    &ffi_type_uint16,   &ffi_type_sint32,
    &ffi_type_uint32,   &ffi_type_sint64,
    &ffi_type_uint64,   &ffi_type_float,
    &ffi_type_double,   &ffi_type_pointer,
    &ffi_type_pointer,  &ffi_type_pointer,
    &ffi_type_pointer,  &ffi_type_pointer,
    &ffi_type_pointer,  &ffi_type_pointer,
    &ffi_type_pointer,  &ffi_type_pointer

};

size_t drpc_proto_to_ffi_len_adjust(enum drpc_types* prototype, size_t prototype_len){
    size_t adjusted_len = prototype_len;
    for(size_t i = 0; i < prototype_len; i++){
        if(prototype[i] == d_sizedbuf) adjusted_len++;
    }
    return adjusted_len;
}

ffi_type** drpc_proto_to_ffi(enum drpc_types* prototype, size_t prototype_len){
    if(prototype == NULL) return NULL;

    size_t adjusted_len = drpc_proto_to_ffi_len_adjust(prototype,prototype_len);
    ffi_type** ffi_proto = calloc(adjusted_len + 1,sizeof(ffi_type*)); assert(ffi_proto);
    size_t j = 0;

    for(size_t i = 0; i < prototype_len; i++,j++){
        if(prototype[i] >= d_return_is) prototype[i] = d_void;

        ffi_proto[j] = drpc_ffi_convert_table[prototype[i]];
        if(prototype[i] == d_sizedbuf){
            j++;
            ffi_proto[j] = &ffi_type_ulong;
        }
    }

    return ffi_proto;
}


/*=============================*/

void random_str(char* dest, size_t len){
    char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    arc4random_buf(dest,len - 1);
    for(size_t i = 0; i < len - 1; i++){
        dest[i] = charset[dest[i] % (sizeof(charset) - 1)];
    }
}

void* drpc_server_dispatcher(void* drpc_server_P);

struct drpc_server* new_drpc_server(uint16_t port){
    struct drpc_server* drpc_serv = calloc(1,sizeof(*drpc_serv)); assert(drpc_serv);

    drpc_serv->functions = hashtable_create();
    drpc_serv->users = hashtable_create();
    drpc_serv->client_threads = hashtable_create();

    drpc_serv->recv_mailboxes = new_d_struct();
    drpc_serv->send_mailboxes = new_d_struct();

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
    assert(setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt,sizeof(opt)) == 0);
    assert(bind(server->server_fd,(struct sockaddr*)&addr,sizeof(addr)) == 0);
    assert(listen(server->server_fd,MAX_LISTEN) == 0);

    assert(pthread_create(&server->dispatcher,NULL,drpc_server_dispatcher,server) == 0);
}

void drpc_fn_info_free_CB(void* fn_info_P){
    if(fn_info_P == NULL) return;
    struct drpc_function* fn_info = fn_info_P;
    if(fn_info->fnstorage != NULL && fn_info->fnstorage_free_cb != NULL)
        fn_info->fnstorage_free_cb(fn_info->fnstorage,fn_info->fnstorage_free_cb_userdata,fn_info);

    free(fn_info->cif);
    free(fn_info->fn_name);
    free(fn_info->ffi_prototype);
    free(fn_info->prototype);
    free(fn_info);
}

void drpc_server_free(struct drpc_server* server){
    server->should_stop = 1;  //this variable will stop while loops in dispatcher and client_handle
    for(size_t i = 0; i < server->client_threads->capacity; i++){
        if(server->client_threads->body[i].value != NULL && server->client_threads->body[i].key != NULL && server->client_threads->body[i].key != (char*)0xDEAD){
            pthread_join(*(pthread_t*)(server->client_threads->body[i].value),NULL);
            free(server->client_threads->body[i].value);
        }
    }
    hashtable_destroy(server->client_threads);

    shutdown(server->server_fd, SHUT_RD);
    close(server->server_fd);
    pthread_join(server->dispatcher,NULL); // waiting for dispatcher

    for(size_t i = 0; i < server->functions->capacity; i++){
        if(server->functions->body[i].value != NULL && server->functions->body[i].key != NULL && server->functions->body[i].key != (char*)0xDEAD)
            drpc_fn_info_free_CB(server->functions->body[i].value);
    }

    for(size_t i = 0; i < server->users->capacity; i++){
        if(server->users->body[i].value != NULL && server->users->body[i].key != NULL && server->users->body[i].key != (char*)0xDEAD)
            free(server->users->body[i].value);
    }

    d_struct_free(server->recv_mailboxes);
    d_struct_free(server->send_mailboxes);

    hashtable_destroy(server->users);
    hashtable_destroy(server->functions);

    free(server->name);
    free(server);
}

void drpc_server_register_fn(struct drpc_server* server,char* fn_name, void* fn,
                             enum drpc_types return_type, enum drpc_types* prototype,
                             size_t prototype_len, void* fnstorage, int perm){
    assert(return_type != d_sizedbuf   || return_type != d_fnstorage
        || return_type != d_clientinfo || return_type != d_interfunc);  //you cannot return thoose types!
    struct drpc_function* fn_info = calloc(1,sizeof(*fn_info)); assert(fn_info);

    fn_info->fn_name = strdup(fn_name);
    fn_info->fn = fn;
    fn_info->minimal_permission_level = perm;
    fn_info->return_type = return_type;
    fn_info->fnstorage = fnstorage;
    if(prototype != NULL){
        /*copying prototype*/
        fn_info->prototype_len = prototype_len;
        fn_info->prototype = calloc(fn_info->prototype_len, sizeof(enum drpc_types));
        memcpy(fn_info->prototype,prototype, sizeof(enum drpc_types) * prototype_len);
    }
    hashtable_set(server->functions,fn_name,fn_info);

}

enum drpc_types* drpc_types_extract_prototype(struct drpc_type* drpc_types,size_t drpc_types_len){
    if(drpc_types == NULL) return NULL;
    enum drpc_types* ret = malloc(drpc_types_len * sizeof(enum drpc_types));
    for(size_t i = 0; i < drpc_types_len; i++){
        ret[i] = drpc_types[i].type;
    }
    return ret;
}
enum drpc_types* drpc_create_client_header(enum drpc_types* serv, size_t servlen, size_t* client_len_output){
    struct queue* client_prototype_parts = queue_create();
    assert(client_prototype_parts);
    //creating server prototypes without server-only types
    for(size_t i = 0; i < servlen;i++){
        if(serv[i] != d_interfunc && serv[i] != d_fnstorage && serv[i] != d_clientinfo && serv[i] != d_fninfo){
            queue_push(client_prototype_parts,&serv[i]);
            (*client_len_output)++;
        }
    }

    enum drpc_types* client_prototype = calloc(*client_len_output,sizeof(enum drpc_types));
    assert(client_prototype);

    //recreating new server prototype from que
    for(size_t i = 0; i < *client_len_output; i++){
        client_prototype[i] = *(enum drpc_types*)queue_pop(client_prototype_parts);
    }
    queue_free(client_prototype_parts);
    return client_prototype;
}


int is_arguments_equal_prototype(enum drpc_types* serv, size_t servlen, enum drpc_types* client, size_t clientlen){
    if(!serv && !client) return 0;
    size_t newservlen = 0;

    //creating server prototypes without server-only types
    enum drpc_types* newserv = drpc_create_client_header(serv,servlen,&newservlen);

    if((serv && !client) || (!serv && client)) {
        if(clientlen == 0 && newservlen == 0){
            return 0;
        }
        return 1;
    }

    //if new len is different they are different
    if(newservlen != clientlen) return 1;

    for(size_t i = 0; i < clientlen; i++)
        //if they are different breaking the loop
        if(newserv[i] != client[i]) return 1;


    free(newserv);
    return 0;
}


void** ffi_from_drpc(struct drpc_type* arguments,enum drpc_types* prototype,size_t prototype_len,size_t* ffi_len,struct queue* to_repack, struct queue* fill_later){
    size_t adjusted_len = drpc_proto_to_ffi_len_adjust(prototype,prototype_len);
    void** ffi_arguments = calloc(adjusted_len, sizeof(void*)); assert(ffi_arguments);

    *ffi_len = adjusted_len;
    size_t j = 0; size_t k = 0;
    for(size_t i = 0; i < prototype_len; i++){
        /*those types does not exist on the client side, so extracting them from prototype, and then via que providing
          to the next layer
        */
        if(prototype[i] == d_fnstorage || prototype[i] == d_clientinfo || prototype[i] == d_interfunc || prototype[i] == d_fninfo){
            ffi_arguments[k] = calloc(1,sizeof(void*));
            assert(ffi_arguments[k]);
            struct drpc_type_update* fill_later_info = calloc(1,sizeof(*fill_later_info)); assert(fill_later_info);
            fill_later_info->type = prototype[i];
            fill_later_info->ptr = &ffi_arguments[k];
            queue_push(fill_later,fill_later_info);
            k++;
            continue;
        }
        /*////////////////////////////////////////////////////////////////*/

        /*Those types exist in arguments so unpacking them, some maybe pushed to the 'to_repack' and be provided to the
         next layer*/
            if(arguments[j].type == d_array || arguments[j].type == d_struct || arguments[j].type == d_queue || arguments[j].type == d_str){
                ffi_arguments[k] = calloc(1,sizeof(void*));
                assert(ffi_arguments[k]);
                switch(arguments[j].type){
                    case d_array:
                        *(void**)ffi_arguments[k] = drpc_to_d_array(&arguments[j]);
                        break;
                    case d_struct:
                        *(void**)ffi_arguments[k] = drpc_to_d_struct(&arguments[j]);
                        break;
                    case d_queue:
                        *(void**)ffi_arguments[k] = drpc_to_d_queue(&arguments[j]);
                        break;
                    case d_str:
                        *(void**)ffi_arguments[k] = drpc_to_str(&arguments[j]);
                        break;
                }
                struct drpc_type_update* update = calloc(1,sizeof(*update));
                update->type = arguments[j].type;
                update->ptr = *(void**)ffi_arguments[k];
                queue_push(to_repack,update);

                j++; k++;
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

                queue_push(to_repack,update);

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
            /*//////////////////////////////////////////////////*/
    }
    return ffi_arguments;
}

int drpc_server_call_fn(struct drpc_type* arguments,uint8_t arguments_len, struct drpc_function* fn_info, struct drpc_connection* client_info, struct drpc_return* returned){
    enum drpc_types* extracted_prototype = drpc_types_extract_prototype(arguments,arguments_len);
    if(is_arguments_equal_prototype(fn_info->prototype,fn_info->prototype_len,extracted_prototype,arguments_len)){
        free(extracted_prototype);
        return 1;
    }
    struct queue* to_repack = queue_create(); //this queue will be used for repackable arguments
    struct queue* to_fill = queue_create();   //this queue will be used for server-only arguments

    size_t ffi_len = 0;
    ffi_arg native_return = 0;
    int8_t return_is = -1;

    //generating arguments for ffi_call
    void** ffi_arguments = ffi_from_drpc(arguments,fn_info->prototype,fn_info->prototype_len,&ffi_len,to_repack,to_fill);
    if(fn_info->cif == NULL){
        //allocating CIF if it wasnt allocated already
        fn_info->cif = calloc(1,sizeof(*fn_info->cif)); assert(fn_info->cif);
        assert(ffi_prep_cif(fn_info->cif,FFI_DEFAULT_ABI,ffi_len,(ffi_type*)drpc_ffi_convert_table[fn_info->return_type],
                    (fn_info->ffi_prototype = drpc_proto_to_ffi(fn_info->prototype, fn_info->prototype_len))) == FFI_OK);
    }
    free(extracted_prototype);


    //filling in server-only arguments
    size_t to_fill_len = queue_get_len(to_fill);
    for(size_t i = 0; i <to_fill_len; i++){
        struct drpc_type_update* to_update = queue_pop(to_fill);
        switch(to_update->type){
            case d_fnstorage:
                **(void***)to_update->ptr = fn_info->fnstorage;
                break;
            case d_interfunc:
                **(void***)to_update->ptr = client_info->drpc_server->interfunc;
                break;
            case d_clientinfo:
                **(void***)to_update->ptr = client_info;
                break;
            case d_fninfo:
                **(void***)to_update->ptr = fn_info; //should only be used in proxy implementation, not in user code please it is a huge security issue
                break;
            default:
                break;
        }
        free(to_update);
    }
    drpc_types_free(arguments,arguments_len);
    ffi_call(fn_info->cif,FFI_FN(fn_info->fn),&native_return,ffi_arguments);

    size_t repack_len = queue_get_len(to_repack);

    if(repack_len > 0){
        returned->updated_arguments = calloc(repack_len,sizeof(*returned->updated_arguments));
        assert(returned->updated_arguments);
    }else returned->updated_arguments = NULL;
    returned->updated_arguments_len = repack_len;


    //this types will be in ret->updated_arguments and be used on client side to "emulate" pointers
    //so we are getting pointer of raw arguments from to_repack que, then packing to drpc_type then free
    for(size_t i = 0; i < repack_len; i++){
        struct drpc_type_update* repack = queue_pop(to_repack);

        //if pointer is the same as native_return then we setting return_is variable to i,native_return will
        //not be packed, and on client native_return will be same as the same as returned argument pointer
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
                d_array_free(repack->ptr);
                break;
            default: break;
        }
        free(repack);

    }

    //free arguments
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
                if(fn_info->fnstorage != (void*)native_return && client_info->userdata != (void*)native_return) free((char*)native_return);
                break;
            case d_array:
                if((char*)native_return == NULL) void_to_drpc(&returned->returned);
                else                             d_array_to_drpc(&returned->returned,(void*)native_return);
                if(fn_info->fnstorage != (void*)native_return && client_info->userdata != (void*)native_return) d_array_free((void*)native_return);
                break;
            case d_struct:
                if((char*)native_return == NULL) void_to_drpc(&returned->returned);
                else                             d_struct_to_drpc(&returned->returned,(void*)native_return);
                if(fn_info->fnstorage != (void*)native_return && client_info->userdata != (void*)native_return) d_struct_free((void*)native_return);
                break;
            case d_queue:
                if((char*)native_return == NULL) void_to_drpc(&returned->returned);
                else                             d_queue_to_drpc(&returned->returned,(void*)native_return);
                if(fn_info->fnstorage != (void*)native_return && client_info->userdata != (void*)native_return) d_queue_free((void*)native_return);
            break;
            default: break;
        }
    }else{
        return_is_to_drpc(&returned->returned,return_is);
    }
    queue_free(to_repack);
    queue_free(to_fill);
    return 0;
}
int drpc_handle_call(struct d_struct* received_message, struct drpc_connection* client, int client_perm){
    printf("\n%s: client '%s': requested function call\n",__PRETTY_FUNCTION__,client->username);
    struct drpc_message send = {
        .message = NULL,
        .message_type = drpc_bad,
    };
    int handle_ret = 0;

    struct drpc_call* call = message_to_drpc_call(received_message);

    if(call == NULL){
        printf("%s: malformed call message\n",__PRETTY_FUNCTION__);
        send.message_type = drpc_bad;
        handle_ret = 1; goto exit;
    }
    struct drpc_function* call_fn = NULL;
    if((call_fn = hashtable_get(client->drpc_server->functions,call->fn_name)) == NULL){
        printf("%s: no such function %s!\n",__PRETTY_FUNCTION__,call->fn_name);
        drpc_call_free(call);
        free(call);
        send.message_type = drpc_notfound;
        handle_ret = 1; goto exit;
    }

    struct drpc_return ret;
    if((client_perm > call_fn->minimal_permission_level && call_fn->minimal_permission_level != -1) || client_perm == -1){
        if(drpc_server_call_fn(call->arguments,call->arguments_len,call_fn,client,&ret) != 0){
            printf("%s: bad arguments for function '%s'! \n",__PRETTY_FUNCTION__,call_fn->fn_name);
            drpc_call_free(call);
            free(call);
            send.message_type = drpc_bad;
            handle_ret = 1; goto exit;
        }
        printf("%s: call of '%s' succesfull \n",__PRETTY_FUNCTION__,call->fn_name);
        free(call->fn_name);
        free(call);


        send.message_type = drpc_return;
        send.message = drpc_return_to_message(&ret);
        drpc_return_free(&ret);
        handle_ret = 0; goto exit;
    }

    printf("%s: user permission is too low for %s (have: %d, require %d)!\n",__PRETTY_FUNCTION__,call_fn->fn_name, client_perm,call_fn->minimal_permission_level);

    drpc_call_free(call);
    free(call);
    send.message_type = drpc_eperm;
exit:
    drpc_send_message(client->io, &send);
    return handle_ret;
}

void drpc_handle_mailbox_recv(struct d_struct* received_message,struct drpc_connection* client){
    struct drpc_message send = {
        .message = NULL,
        .message_type = drpc_notfound,
    };
    if(received_message == NULL) {send.message_type = drpc_bad; goto exit;}

    char* mailbox_name; struct d_queue* messages;
    if(d_struct_get(received_message,"receiver_mailbox",&mailbox_name,d_str) != 0){
        printf("%s: client (%s:%s) sent malformed recv request, no receiver_mailbox\n",__PRETTY_FUNCTION__,client->username,client->clientid);
        goto exit;
    }
    if(d_struct_get(received_message,"messages",&messages,d_queue) != 0){
        printf("%s: client (%s:%s) sent malformed recv request, no messages\n",__PRETTY_FUNCTION__,client->username,client->clientid);
        goto exit;
    }

    struct d_queue* receiver_mailbox = NULL;
    if(d_struct_get(client->drpc_server->recv_mailboxes,mailbox_name,&receiver_mailbox,d_queue) != 0){
        printf("%s: client (%s:%s) no such mailbox %s\n",__PRETTY_FUNCTION__,client->username,client->clientid,mailbox_name);
        goto exit;
    }

    size_t messages_len = d_queue_len(messages);
    for(size_t i = 0; i < messages_len; i++){
        queue_push(receiver_mailbox->que,queue_pop(messages->que));
    }
    send.message_type = drpc_ok;
    printf("%s: client (%s:%s) succesfully received messages to %s\n",__PRETTY_FUNCTION__,client->username,client->clientid,mailbox_name);

exit:
    drpc_send_message(client->io,&send);
}
void drpc_handle_mailbox_send(struct d_struct* received_message,struct drpc_connection* client){
    struct drpc_message send = {
        .message = NULL,
        .message_type = drpc_notfound,
    };
    if(received_message == NULL) {send.message_type = drpc_bad; goto exit;}
    char* mailbox_name;
    if(d_struct_get(received_message,"sender_mailbox",&mailbox_name,d_str) != 0){
        printf("%s: client (%s:%s) sent malformed send request \n",__PRETTY_FUNCTION__,client->username,client->clientid);
        goto exit;
    }

    struct d_queue* extracted_messages = NULL;
    if(d_struct_get(client->drpc_server->send_mailboxes,mailbox_name,&extracted_messages,d_queue) != 0){
        printf("%s: client (%s:%s) no such mailbox %s\n",__PRETTY_FUNCTION__,client->username,client->clientid,mailbox_name);
        goto exit;
    }

    struct d_queue* sended_messages = new_d_queue();
    size_t extracted_messages_len = d_queue_len(extracted_messages);
    for(size_t i = 0; i < extracted_messages_len; i++){
        queue_push(sended_messages->que,queue_pop(extracted_messages->que)); //we cannot simply send extracted_messages queue because it will be freed on drpc_send_message.
    }
    send.message = new_d_struct();
    d_struct_set(send.message,"messages",sended_messages,d_queue);
    send.message_type = drpc_ok;
    printf("%s: client (%s:%s) succesfully took messages from %s\n",__PRETTY_FUNCTION__,client->username,client->clientid,mailbox_name);

exit:
    drpc_send_message(client->io,&send);
}

struct __drpc_server_event{
    enum drpc_protocol event;
    struct d_struct* recv_message;
};

struct __drpc_executor_thread_params{
    struct drpc_connection* client;
    int client_perm;

    int already_disconnected;
    struct queue* event_queue;

    sem_t wait; //used to fix issue when infinite loop was able to do this whole drpc_client_executor code WAAAAY ALOWER
};

//04.04.2025 19.30: i dont think this code is any better than previos but i had a strong feeling i should remade it that way
//04.04.2025 21.27: OR I NEED TO FIND BETTER INTER-THREAD COMMUNICATION OR KILL THIS CODE
//04.04.2025 22.12: i was able to make it run better via semaphore but results are still worse
//04.04.2025 22.40: Only plus about that code that i found is: When something generate many drpc packages it will be able to catch them and them process in a row but.....
//04.04.2025 23.17: pohuy commiting it
void* drpc_client_executor(void* params_P){
    struct __drpc_executor_thread_params* params = params_P;

    int stop = 0;
    //attempt to process remaining events
    while(queue_get_len(params->event_queue) > 0 || stop == 0){
        struct __drpc_server_event* event = queue_pop(params->event_queue);
        struct drpc_message send = {0};
        if(event == NULL){
            sem_wait(&params->wait);
            continue;
        }
        switch(event->event){
            case drpc_ping:
                send.message = NULL; send.message_type = drpc_ping;
                if(drpc_send_message(params->client->io,&send) != 0) stop = 1;
                break;
            case drpc_disconnect:
                if(params->already_disconnected == 0){
                    printf("%s: client (%s:%s) successfully disconnected\n",__PRETTY_FUNCTION__,params->client->username,params->client->clientid);
                    params->already_disconnected = 100; //fucking garbage
                }
                stop = 1;
                break;
            case drpc_call:
                printf("%s: function call request from client (%s:%s)\n",__PRETTY_FUNCTION__,params->client->username,params->client->clientid);
                if(drpc_handle_call(event->recv_message,params->client,params->client_perm) != 0) stop = 1;
                break;
            case drpc_mailbox_recv:
                printf("%s: received messages from client (%s:%s)\n",__PRETTY_FUNCTION__,params->client->username,params->client->clientid);
                drpc_handle_mailbox_recv(event->recv_message,params->client);
                break;
            case drpc_mailbox_send:
                printf("%s: send messages to client (%s:%s)\n",__PRETTY_FUNCTION__,params->client->username,params->client->clientid);
                drpc_handle_mailbox_send(event->recv_message,params->client);
                break;
            case drpc_servername:
                printf("%s: client (%s:%s) asked about server name\n",__PRETTY_FUNCTION__,params->client->username,params->client->clientid);
                send.message_type = drpc_servername;
                send.message = new_d_struct();
                char* name = NULL;

                if(params->client->drpc_server->name == NULL) name = "UNKNOWN_DRPC";
                else name = params->client->drpc_server->name;

                d_struct_set(send.message,"drpc_servername",name,d_str);
                if(drpc_send_message(params->client->io,&send) != 0) stop = 1;
                break;

            default:
                stop = 1;
                break;
        }
        d_struct_free(event->recv_message);
        free(event);
    }
    params->already_disconnected = 100;
    return NULL;
}

void drpc_handle_client(struct drpc_connection* client, int client_perm){
    assert(client); //we are fucked

    /*Generic handle thread initialization code*/
    random_str(client->clientid,sizeof(client->clientid));
    pthread_t* self = calloc(1,sizeof(*self)); assert(self);
    *self = pthread_self();
    hashtable_set(client->drpc_server->client_threads,client->clientid,self);

    struct queue* event_queue = queue_create();

    struct __drpc_executor_thread_params* params = calloc(1,sizeof(*params));

    params->client = client;
    params->client_perm = client_perm;
    params->event_queue = event_queue;
    assert(sem_init(&params->wait,0,0) == 0);

    pthread_t executor_thread;
    assert(pthread_create(&executor_thread,NULL,drpc_client_executor,params) == 0);
    /*==================================================*/

    if(client->drpc_server->connection_event_cb != NULL)
        client->drpc_server->connection_event_cb(client,drpc_connected);

    while(client->drpc_server->should_stop == 0 && client->force_disconnect == 0){
        struct drpc_message recv = {0};
        struct __drpc_server_event* event = malloc(sizeof(*event)); assert(event);
        if(drpc_recv_message(client->io,&recv) != 0){
            free(event);
            break;
        }
        event->event = recv.message_type;
        event->recv_message = recv.message;
        queue_push(event_queue,event);
        assert(sem_post(&params->wait) == 0);
    }
    if(params->already_disconnected == 0){
        struct __drpc_server_event* disconnect_event = malloc(sizeof(*disconnect_event)); assert(disconnect_event);
        disconnect_event->event = drpc_disconnect;
        disconnect_event->recv_message = NULL;
        queue_push(event_queue,disconnect_event);
        assert(sem_post(&params->wait) == 0);
    }
    pthread_join(executor_thread,NULL);

    size_t l = queue_get_len(event_queue);
    for(size_t i = 0; i < l; i++) free(queue_pop(event_queue)); //somehow one in 2/5 test runs drpc_disconnect  event was able to stay in queue


    queue_free(event_queue);
    free(params);

    hashtable_remove(client->drpc_server->client_threads,client->clientid);
    pthread_detach(*self);
    free(self);
}

void* drpc_server_client_auth(void* drpc_connection_P){
   struct drpc_connection* client = drpc_connection_P;

   struct drpc_message recv;
   struct drpc_message send;
   int perm = 0;
   if(drpc_recv_message(client->io,&recv) != 0){
       printf("%s: no auth request!\n",__PRETTY_FUNCTION__);
       goto exit;
   }
   if(recv.message_type != drpc_auth || recv.message == NULL){
       send.message_type = drpc_bad;
       send.message = NULL;
       drpc_send_message(client->io,&send);
       printf("%s: request is not auth or malformed!\n",__PRETTY_FUNCTION__);
       goto exit;
   }else{
       char* username;
       uint64_t hash;
       struct drpc_user* user;
       if(d_struct_get(recv.message,"username",&username,d_str) != 0){
           printf("%s: auth malformed1\n",__PRETTY_FUNCTION__);
           d_struct_free(recv.message);
           send.message_type = drpc_bad;
           send.message = NULL;
           drpc_send_message(client->io,&send);
           goto exit;
       }
       if(d_struct_get(recv.message,"passwd_hash",&hash,d_uint64) != 0){
           printf("%s: auth malformed2\n",__PRETTY_FUNCTION__);
           d_struct_free(recv.message);
           send.message_type = drpc_bad;
           send.message = NULL;
           drpc_send_message(client->io,&send);
           goto exit;
       }
       d_struct_unlink(recv.message,"username");
       d_struct_free(recv.message);
       if((user = hashtable_get(client->drpc_server->users,username)) == NULL){
           printf("%s: no such username : %s\n",__PRETTY_FUNCTION__,username);
           send.message_type = drpc_bad;
           send.message = NULL;
           free(username);
           drpc_send_message(client->io,&send);
           goto exit;
       }
       if(user->hash != hash){
           printf("%s: wrong password for : %s\n",__PRETTY_FUNCTION__,username);
           send.message_type = drpc_bad;
           send.message = NULL;
           free(username);
           drpc_send_message(client->io,&send);
           goto exit;
       }
       perm = user->perm;
       client->username = username;

       send.message = new_d_struct();
       send.message_type = drpc_ok;

       uint8_t xor_base[16];
       arc4random_buf(xor_base,sizeof(xor_base));

       d_struct_set(send.message,"encrypt_xor",xor_base,d_sizedbuf,sizeof(xor_base));

       if(drpc_send_message(client->io,&send) != 0) goto exit;

       client->io->aes128_key = calloc(sizeof(xor_base),1); assert(client->io->aes128_key);
       for(int i = 0; i < sizeof(xor_base); i++){
           client->io->aes128_key[i] = xor_base[i] ^ user->aes128_passwd[i];
       }

       printf("%s: client '%s' authenticated succesfully\n",__PRETTY_FUNCTION__,client->username);
   }
   drpc_handle_client(client,perm);
   if(client->drpc_server->connection_event_cb != NULL)
       client->drpc_server->connection_event_cb(client,drpc_disconnected);
exit:
   client->io->close(client->io);
   client->io->free(client->io);
   free(client->username);
   free(client);
   return NULL;
}



void* drpc_server_dispatcher(void* drpc_server_P){
    struct drpc_server* server = drpc_server_P;
    printf("%s: TCP started\n",__PRETTY_FUNCTION__);
    while(server->should_stop == 0){
        socklen_t client_addr_len = sizeof(struct sockaddr_in);
        struct sockaddr_in client_addr;
        int client_fd = accept(server->server_fd,(struct sockaddr*)&client_addr,&client_addr_len);
        if(client_fd > 0){

            printf("%s: picked up client: %s\n",__PRETTY_FUNCTION__,inet_ntoa(client_addr.sin_addr));

            struct drpc_connection* client = calloc(1,sizeof(*client)); assert(client);
            client->io = calloc(1,sizeof(*client->io)); assert(client->io);

            client->io->io_data = calloc(1,sizeof(int)); assert(client->io->io_data);
            *(int*)client->io->io_data = client_fd;

            client->io->close = drpc_tcp_close;
            client->io->free = drpc_tcp_free;
            client->io->send = drpc_tcp_send_message;
            client->io->recv = drpc_tcp_recv_message;

            client->drpc_server = server;

            struct timeval time;
            time.tv_sec = DRPC_IO_TIMEOUT;
            time.tv_usec = 0;
            assert(setsockopt(client_fd,SOL_SOCKET,SO_RCVTIMEO,&time,sizeof(time)) == 0);
            assert(setsockopt(client_fd,SOL_SOCKET,SO_SNDTIMEO,&time,sizeof(time)) == 0);

            pthread_t client_auth;
            assert(pthread_create(&client_auth,NULL,drpc_server_client_auth,client) == 0);
        }else return NULL;
    }
    return NULL;
}
void drpc_server_add_user(struct drpc_server* serv, char* username,char* passwd, int perm){
    struct drpc_user* user = calloc(1,sizeof(*user)); assert(user);

    user->hash = murmur(passwd,strlen(passwd));
    user->perm = perm;

    int cpylen = 0;
    if(strlen(passwd) > sizeof(user->aes128_passwd)) cpylen = sizeof(user->aes128_passwd);
    else cpylen = strlen(passwd);

    memcpy(user->aes128_passwd,passwd,cpylen);

    hashtable_set(serv->users,username,user);
}

void drpc_server_set_servername(struct drpc_server* server, char* name){
    if(server->name != NULL) free(server->name);

    if(name == NULL){
        server->name = NULL;
        return;
    }
    server->name = strdup(name);
    assert(server->name); //i dont know what may happen LOL
}

char* drpc_server_get_servername(struct drpc_server* server){
    return server->name;
}

void drpc_server_set_connection_event_cb(struct drpc_server* server, drpc_connection_event_cb drpc_connection_event_cb){
    server->connection_event_cb = drpc_connection_event_cb;
}
void drpc_server_force_disconnect_client(struct drpc_connection* client){
    client->force_disconnect = 1;
}

//if you are using one fnstorage for multiple functions and you registred one callback for all that function, YOU SHOULD implement some kind of sync mechanism to avoid double-free or other errors
int drpc_server_set_fnstorage_free_cb(struct drpc_server* server, char* fn_name, drpc_fnstorage_free_cb fnstorage_free_cb, void* userdata){
    struct drpc_function* fn = hashtable_get(server->functions,fn_name);
    if(fn == NULL) return 1;
    fn->fnstorage_free_cb = fnstorage_free_cb;
    fn->fnstorage_free_cb_userdata = userdata;
    return 0;
}

struct d_queue* new_drpc_recv_mailbox(struct drpc_server* server, char* mailbox_name){
    struct d_queue* check;
    if(d_struct_get(server->recv_mailboxes,mailbox_name,&check,d_queue) == 0) return check;

    struct d_queue* mailbox = new_d_queue();
    d_struct_set(server->recv_mailboxes,mailbox_name,mailbox,d_queue);
    return mailbox;
}
struct d_queue* new_drpc_send_mailbox(struct drpc_server* server, char* mailbox_name){
    struct d_queue* check;
    if(d_struct_get(server->send_mailboxes,mailbox_name,&check,d_queue) == 0) return check;

    struct d_queue* mailbox = new_d_queue();
    d_struct_set(server->send_mailboxes,mailbox_name,mailbox,d_queue);
    return mailbox;
}

struct d_queue* drpc_get_recv_mailbox(struct drpc_server* server, char* mailbox_name){
    struct d_queue* mailbox = NULL;
    d_struct_get(server->recv_mailboxes,mailbox_name,&mailbox,d_queue);
    return mailbox;
}
void drpc_free_recv_mailbox(struct drpc_server* server, char* mailbox_name){
    d_struct_remove(server->recv_mailboxes,mailbox_name);
}

struct d_queue* drpc_get_send_mailbox(struct drpc_server* server, char* mailbox_name){
    struct d_queue* mailbox = NULL;
    d_struct_get(server->send_mailboxes,mailbox_name,&mailbox,d_queue);
    return mailbox;
}
void drpc_free_send_mailbox(struct drpc_server* server, char* mailbox_name){
    d_struct_remove(server->send_mailboxes,mailbox_name);
}

#ifdef DRPC_DQUEUE_IO
#include "drpc_client.h"

struct drpc_handle_client_thread_wrapper{
    struct drpc_connection* client;
    int client_perm;
};

void* drpc_new_dqueue_handle_client_wrapper(void* params_P){
    struct drpc_handle_client_thread_wrapper* params = params_P;
    drpc_handle_client(params->client,params->client_perm);
    params->client->io->close(params->client->io);
    params->client->io->free(params->client->io);
    free(params->client);
    free(params);
    return NULL;
}

struct drpc_client* drpc_new_dqueue_client(struct drpc_server* server, int client_perm){
    struct drpc_client* client = calloc(1,sizeof(*client)); assert(client);
    client->client_stop = 0;
    assert(pthread_mutex_init(&client->connection_mutex,NULL) == 0);
    client->io = calloc(1,sizeof(*client->io)); assert(client->io);
    client->io->io_data = calloc(1,sizeof(struct drpc_dqueue_io)); assert(client->io->io_data);
    client->io->aes128_key = NULL;
    client->io->close = drpc_dqueue_close;
    client->io->free = drpc_dqueue_free;
    client->io->send = drpc_dqueue_send_message;
    client->io->recv = drpc_dqueue_recv_message;

    struct drpc_connection* server_client = calloc(1,sizeof(*server_client));
    server_client->drpc_server = server;
    server_client->force_disconnect = 0;
    server_client->username = NULL; //generate random username
    server_client->io = calloc(1,sizeof(*server_client->io));
    server_client->io->aes128_key = NULL;
    server_client->io->io_data = calloc(1,sizeof(struct drpc_dqueue_io)); assert(client->io->io_data);
    server_client->io->close = drpc_dqueue_close;
    server_client->io->free = drpc_dqueue_free;
    server_client->io->send = drpc_dqueue_send_message;
    server_client->io->recv = drpc_dqueue_recv_message;

    struct drpc_dqueue_io* client_io_data = client->io->io_data;
    struct drpc_dqueue_io* server_io_data = server_client->io->io_data;
    assert(pthread_mutex_init(&client_io_data->lock,NULL) == 0);
    client_io_data->recv = new_d_queue();
    server_io_data->recv = new_d_queue();

    client_io_data->send = server_io_data;
    server_io_data->send = client_io_data;

    struct drpc_handle_client_thread_wrapper* params = calloc(1,sizeof(*params)); assert(params);
    params->client = server_client;
    params->client_perm = client_perm;

    pthread_t client_thread;
    assert(pthread_create(&client->ping_thread,NULL,drpc_ping_server,client) == 0);
    assert(pthread_create(&client_thread,NULL,drpc_new_dqueue_handle_client_wrapper,params) == 0);
    return client;

}
#endif

#include "drpc_client.h"
#include "drpc_protocol.h"
#include "drpc_que.h"
#include "drpc_queue.h"
#include "drpc_server.h"
#include "drpc_types.h"
#include "drpc_struct.h"
#include "hashtable.c/hashtable.h"

#include <arpa/inet.h>
#include <stdarg.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/socket.h>
#include <string.h>


#include <stdio.h>

#define ENETWORK 10
#define BADREPLY 20

void* drpc_ping_server(void* clientP){
    struct drpc_client* client = clientP;

    while(client->client_stop == 0){
        pthread_mutex_lock(&client->connection_mutex);
        struct drpc_message send,recv;

        send.message_type = drpc_ping;
        send.message = NULL;

        recv.message = NULL;
        recv.message_type = drpc_bad;

        if(drpc_send_message(&send,client->fd) != 0){
            client->client_stop = 1;
            pthread_mutex_unlock(&client->connection_mutex);
            return NULL;
        }
        if(drpc_recv_message(&recv,client->fd) != 0 || recv.message_type != drpc_ping){
            client->client_stop = 1;
            pthread_mutex_unlock(&client->connection_mutex);
            return NULL;
        }
        pthread_mutex_unlock(&client->connection_mutex);
        sleep(4);
        continue;
    }
    pthread_mutex_unlock(&client->connection_mutex);

    return NULL;
}

struct drpc_client* drpc_client_connect(char* ip,uint16_t port, char* username, char* passwd){
    assert(ip != NULL); assert(username != NULL); assert(passwd != NULL);
    struct sockaddr_in connect_to = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };
    assert(inet_aton(ip,&connect_to.sin_addr) != 0);
    int fd = socket(AF_INET,SOCK_STREAM,0);
    if(connect(fd,(struct sockaddr*)&connect_to,sizeof(connect_to)) != 0){
        close(fd);
        return NULL;
    }
    struct drpc_client* client = malloc(sizeof(*client)); assert(client);
    client->client_stop = 1;
    client->fd = fd;

    uint64_t passwd_hash = _hash_fnc(passwd,strlen(passwd));
    struct d_struct* auth = new_d_struct();


    d_struct_set(auth,"username",username,d_str);
    d_struct_set(auth,"passwd_hash",&passwd_hash,d_uint64);

    struct drpc_message send = {
        .message_type = drpc_auth,
        .message = auth,
    };

    if(drpc_send_message(&send,fd) != 0){
        d_struct_free(auth);
        free(client);
        close(fd);
        return NULL;
    }
    d_struct_free(auth);

    struct drpc_message recv = {0};
    if(drpc_recv_message(&recv,fd) != 0){
        free(client);
        close(fd);
        return NULL;
    }
    if(recv.message_type != drpc_ok){
        free(client);
        close(fd);
        return NULL;
    }
    client->client_stop = 0;
    assert(pthread_mutex_init(&client->connection_mutex,NULL) == 0);
    assert(pthread_create(&client->ping_thread,NULL,drpc_ping_server,client) == 0);

    return client;
}

void drpc_client_disconnect(struct drpc_client* client){
    if(client == NULL) return;
    client->client_stop = 1;
    pthread_mutex_lock(&client->connection_mutex);

    struct drpc_message send = {
        .message_type = drpc_disconnect,
        .message = NULL,
    };
    drpc_send_message(&send,client->fd);
    close(client->fd);
    pthread_mutex_unlock(&client->connection_mutex);
    pthread_join(client->ping_thread,NULL);
    free(client);
}


int drpc_client_call(struct drpc_client* client, char* fn_name, enum drpc_types* prototype, size_t prototype_len,void* native_return,...){
    if(client->client_stop != 0) return 1;

    va_list varargs;
    va_start(varargs,native_return);

    struct drpc_type* arguments = calloc(prototype_len,sizeof(*arguments)); assert(arguments);
    struct drpc_que* updated_arguments_que = drpc_que_create();

    void* arg = NULL;
    struct drpc_type_update* update = NULL;

    for(size_t i = 0; i < prototype_len; i++){
        switch(prototype[i]){
            case d_int8:
                int8_to_drpc(&arguments[i],va_arg(varargs,int));
                break;
            case d_uint8:
                uint8_to_drpc(&arguments[i],va_arg(varargs,unsigned int));
                break;
            case d_int16:
                int16_to_drpc(&arguments[i],va_arg(varargs,int));
                break;
            case d_uint16:
                uint16_to_drpc(&arguments[i],va_arg(varargs,unsigned int));
                break;
            case d_int32:
                int32_to_drpc(&arguments[i],va_arg(varargs,int));
                break;
            case d_uint32:
                uint32_to_drpc(&arguments[i],va_arg(varargs,unsigned int));
                break;
            case d_int64:
                int64_to_drpc(&arguments[i],va_arg(varargs,int64_t));
                break;
            case d_uint64:
                uint64_to_drpc(&arguments[i],va_arg(varargs,uint64_t));
                break;
            case d_float:
                float_to_drpc(&arguments[i],va_arg(varargs,double));
                break;
            case d_double:
                double_to_drpc(&arguments[i],va_arg(varargs,double));
                break;

            case d_str:
                arg = va_arg(varargs,char*);

                update = malloc(sizeof(*update)); assert(update);
                update->type = d_str;
                update->ptr = arg;
                drpc_que_push(updated_arguments_que,update);

                str_to_drpc(&arguments[i],arg);
                break;
            case d_sizedbuf:
                arg = va_arg(varargs,char*);

                update = malloc(sizeof(*update)); assert(update);
                update->type = d_sizedbuf;
                update->ptr = arg;
                update->len = va_arg(varargs,size_t);
                drpc_que_push(updated_arguments_que,update);

                sizedbuf_to_drpc(&arguments[i],arg,update->len);
                break;
            case d_struct:
                arg = va_arg(varargs,void*);

                update = malloc(sizeof(*update)); assert(update);
                update->type = d_struct;
                update->ptr = arg;
                drpc_que_push(updated_arguments_que,update);

                d_struct_to_drpc(&arguments[i],arg);
                break;
            case d_array:
                arg = va_arg(varargs,void*);

                update = malloc(sizeof(*update)); assert(update);
                update->type = d_array;
                update->ptr = arg;
                drpc_que_push(updated_arguments_que,update);

                d_array_to_drpc(&arguments[i],arg);
                break;
            case d_queue:
                arg = va_arg(varargs,void*);

                update = malloc(sizeof(*update)); assert(update);
                update->type = d_queue;
                update->ptr = arg;
                drpc_que_push(updated_arguments_que,update);

                d_queue_to_drpc(&arguments[i],arg);
                break;

            default: break;
        }
    }

    struct drpc_call call = {
        .arguments = arguments,
        .arguments_len = prototype_len,
        .fn_name = fn_name,
    };


    struct drpc_message recv;
    struct drpc_message send = {
        .message = drpc_call_to_message(&call),
        .message_type = drpc_call,
    };
    for(size_t i = 0; i < prototype_len; i++){
        drpc_type_free(&arguments[i]);
    }
    free(arguments);

    pthread_mutex_lock(&client->connection_mutex);
    if(drpc_send_message(&send,client->fd) != 0){
        d_struct_free(send.message);

        struct drpc_type_update* freeU = NULL;
        while((freeU = drpc_que_pop(updated_arguments_que)) != NULL) free(freeU);

        drpc_que_free(updated_arguments_que);
        pthread_mutex_unlock(&client->connection_mutex);
        return ENETWORK;
    }
    d_struct_free(send.message);
    if(drpc_recv_message(&recv,client->fd) != 0){
        struct drpc_type_update* freeU = NULL;
        while((freeU = drpc_que_pop(updated_arguments_que)) != NULL) free(freeU);

        drpc_que_free(updated_arguments_que);
        pthread_mutex_unlock(&client->connection_mutex);
        return ENETWORK;
    }
    if(recv.message_type != drpc_return){
        struct drpc_type_update* freeU = NULL;
        while((freeU = drpc_que_pop(updated_arguments_que)) != NULL) free(freeU);

        drpc_que_free(updated_arguments_que);
        pthread_mutex_unlock(&client->connection_mutex);
        return BADREPLY;
    }
    pthread_mutex_unlock(&client->connection_mutex);

    struct drpc_return* ret = message_to_drpc_return(recv.message);
    d_struct_free(recv.message);

    int8_t return_is = -1;

    if(ret->returned.type == d_return_is){
        return_is = drpc_to_return_is(&ret->returned);
    }
    assert(drpc_que_get_len(updated_arguments_que) == ret->updated_arguments_len);

    for(uint8_t i = 0 ; i < ret->updated_arguments_len ; i++){
        struct drpc_type_update* to_update = drpc_que_pop(updated_arguments_que);

        void* unpacked = NULL; size_t strcpy_len = 0; size_t unused;

        switch(to_update->type){
            case d_str:
                unpacked = drpc_to_str(&ret->updated_arguments[i]);

                if(strlen(to_update->ptr) > strlen(unpacked)) strcpy_len = strlen(unpacked);
                else                                          strcpy_len = strlen(to_update->ptr);

                memcpy(to_update->ptr,unpacked,strcpy_len+1);
                free(unpacked);

                if(return_is == i) *(char**)native_return = to_update->ptr;
                break;
            case d_sizedbuf:
                unpacked = drpc_to_sizedbuf(&ret->updated_arguments[i],&unused);
                memcpy(to_update->ptr,unpacked,to_update->len);
                free(unpacked);

                // if(return_is == i) *(char**)native_return = to_update->ptr;
                break;
            case d_struct:
                struct d_struct* dstruct = drpc_to_d_struct(&ret->updated_arguments[i]);
                struct d_struct* doriginal = to_update->ptr;
                d_struct_free_internal(doriginal);

                *doriginal = *dstruct;

                if(return_is == i) *(struct d_struct**)native_return = doriginal;
                free(dstruct);
                break;
            case d_queue:
                struct d_queue* dqueue = drpc_to_d_queue(&ret->updated_arguments[i]);
                struct d_queue* qoriginal = to_update->ptr;
                d_queue_free_internals(qoriginal);

                *qoriginal = *dqueue;

                if(return_is == i) *(struct d_queue**)native_return = qoriginal;
                free(dqueue);
                break;
            default: break;
        }
        free(to_update);
    }
    drpc_que_free(updated_arguments_que);
    if(return_is == -1){
        switch(ret->returned.type){
            case d_int8:
                *(int8_t*)native_return = drpc_to_int8(&ret->returned);
                break;
            case d_uint8:
                *(uint8_t*)native_return = drpc_to_uint8(&ret->returned);
                break;
            case d_int16:
                *(int16_t*)native_return = drpc_to_int16(&ret->returned);
                break;
            case d_uint16:
                *(uint16_t*)native_return = drpc_to_uint16(&ret->returned);
                break;
            case d_int32:
                *(int32_t*)native_return = drpc_to_int32(&ret->returned);
                break;
            case d_uint32:
                *(uint32_t*)native_return = drpc_to_uint32(&ret->returned);
                break;
            case d_int64:
                *(int64_t*)native_return = drpc_to_int64(&ret->returned);
                break;
            case d_uint64:
                *(uint64_t*)native_return = drpc_to_uint64(&ret->returned);
                break;

            case d_float:
                *(float*)native_return = drpc_to_float(&ret->returned);
                break;
            case d_double:
                *(double*)native_return = drpc_to_double(&ret->returned);
                break;

            case d_str:
                *(char**)native_return = drpc_to_str(&ret->returned);
                break;
            case d_struct:
                *(struct d_struct**)native_return = drpc_to_d_struct(&ret->returned);
                break;
            case d_queue:
                *(struct d_queue**)native_return = drpc_to_d_queue(&ret->returned);
                break;

            default: break;
        }
    }
    drpc_return_free(ret); free(ret);
    return 0;
}

int drpc_client_send_delayed(struct drpc_client* client, char* fn_name, struct d_struct* delayed_message){
    if(client->client_stop != 0) return 1;

    struct d_struct* message = new_d_struct();

    d_struct_set(message,"fn_name",fn_name,d_str);
    d_struct_set(message,"payload",delayed_message,d_struct);

    struct drpc_message recv = {0};
    struct drpc_message send = {
        .message = message,
        .message_type = drpc_send_delayed,
    };
    pthread_mutex_lock(&client->connection_mutex);
    if(drpc_send_message(&send,client->fd) != 0){
        d_struct_unlink(message,"payload",d_struct);
        d_struct_free(message);
        pthread_mutex_unlock(&client->connection_mutex);
        return ENETWORK;
    }

    d_struct_unlink(message,"payload",d_struct);
    d_struct_free(message);
    if(drpc_recv_message(&recv,client->fd) != 0 || recv.message_type != drpc_ok){
        pthread_mutex_unlock(&client->connection_mutex);
        return BADREPLY;
    }
    pthread_mutex_unlock(&client->connection_mutex);
    return 0;
}

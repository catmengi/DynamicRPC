#include "drpc_client.h"
#include "drpc_protocol.h"
#include "queue.h"
#include "drpc_queue.h"
#include "drpc_server.h"
#include "drpc_types.h"
#include "drpc_struct.h"
#include "drpc_array.h"
#include "hashtable.c/hashtable.h"

#include <arpa/inet.h>
#include <stdarg.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>


#include <stdio.h>
void* drpc_ping_server(void* clientP){
    struct drpc_client* client = clientP;
    struct drpc_message send,recv;
    while(client->client_stop == 0){
        pthread_mutex_lock(&client->connection_mutex);
        send.message_type = drpc_ping;
        send.message = NULL;

        recv.message = NULL;
        recv.message_type = drpc_bad;

        if(drpc_send_message(client->io,&send) != 0){
            client->client_stop = 1;
            client->io->close(client->io);
            client->io->free(client->io);
            client->io = NULL;
            pthread_mutex_unlock(&client->connection_mutex);
            return NULL;
        }
        if(drpc_recv_message(client->io,&recv) != 0 || recv.message_type != drpc_ping){
            client->client_stop = 1;
            client->io->close(client->io);
            client->io->free(client->io);
            client->io = NULL;
            pthread_mutex_unlock(&client->connection_mutex);
            return NULL;
        }
        pthread_mutex_unlock(&client->connection_mutex);
        sleep(4);
        continue;
    }
    pthread_mutex_lock(&client->connection_mutex);
    send.message = NULL;
    send.message_type = drpc_disconnect;
    drpc_send_message(client->io,&send);
    pthread_mutex_unlock(&client->connection_mutex);

    client->io->close(client->io);
    client->io->free(client->io);
    client->io = NULL;
    return NULL;
}

struct drpc_client* drpc_client_connect(char* host, char* username, char* passwd){
    assert(host != NULL); assert(username != NULL); assert(passwd != NULL);
    struct addrinfo hints = {
        .ai_flags = AI_NUMERICSERV,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE,
        .ai_protocol = 0,
        .ai_addr = NULL,
        .ai_next = NULL,
    };
    struct addrinfo* host_list = NULL;
    struct addrinfo* host_list_org = NULL;
    char* host_ip = strdup(host); assert(host_ip);
    char* host_port = strchr(host_ip,':');
    if(host_port == NULL) return NULL;
    *host_port = '\0'; host_port++;

    if(getaddrinfo(host_ip,host_port,&hints,&host_list_org) != 0){
        free(host_ip);
        return NULL;
    }
    host_list = host_list_org;
    free(host_ip);

    int success = 0;
    struct drpc_client* ret = NULL;
    while(host_list != NULL){
        int fd = socket(host_list->ai_family,SOCK_STREAM,0); //we are also trying IPv6, because somewhere in future drpc_server will also support IPv6
        if(connect(fd,host_list->ai_addr,host_list->ai_addrlen) == 0){
            struct drpc_client* client = malloc(sizeof(*client)); assert(client);
            client->client_stop = 1;

            client->io = calloc(1,sizeof(*client->io)); assert(client->io);
            client->io->io_data = calloc(1,sizeof(int)); assert(client->io->io_data);
            *(int*)client->io->io_data = fd;

            client->io->close = drpc_tcp_close;
            client->io->free = drpc_tcp_free;
            client->io->send = drpc_tcp_send_message;
            client->io->recv = drpc_tcp_recv_message;

            uint64_t passwd_hash = murmur(passwd,strlen(passwd));
            struct d_struct* auth = new_d_struct();


            d_struct_set(auth,"username",username,d_str);
            d_struct_set(auth,"passwd_hash",&passwd_hash,d_uint64);

            struct drpc_message send = {
                .message_type = drpc_auth,
                .message = auth,
            };

            if(drpc_send_message(client->io,&send) != 0){
                client->io->close(client->io);
                client->io->free(client->io);
                free(client);

                host_list = host_list->ai_next;
                continue;
            }

            struct drpc_message recv = {0};
            if(drpc_recv_message(client->io,&recv) != 0){
                client->io->close(client->io);
                client->io->free(client->io);
                free(client);

                host_list = host_list->ai_next;
                continue;
            }
            if(recv.message_type != drpc_ok){
                client->io->close(client->io);
                client->io->free(client->io);
                free(client);

                host_list = host_list->ai_next;
                continue;
            }

            uint8_t* xor_base; size_t xor_len = 0;
            if(d_struct_get(recv.message,"encrypt_xor",&xor_base,d_sizedbuf,&xor_len) != 0){
                d_struct_free(recv.message);
                client->io->close(client->io);
                client->io->free(client->io);
                free(client);

                host_list = host_list->ai_next;
                continue;
            }

            uint8_t aes128_passwd[16] = {0};

            int cpylen = 0;
            if(strlen(passwd) > sizeof(aes128_passwd)) cpylen = sizeof(aes128_passwd);
            else cpylen = strlen(passwd);
            memcpy(aes128_passwd,passwd,cpylen);

            client->io->aes128_key = calloc(1,sizeof(aes128_passwd)); assert(client->io->aes128_key);
            for(int i = 0; i < sizeof(aes128_passwd); i++){
                client->io->aes128_key[i] = xor_base[i] ^ aes128_passwd[i];
            }

            client->client_stop = 0;
            d_struct_free(recv.message);
            assert(pthread_mutex_init(&client->connection_mutex,NULL) == 0);
            assert(pthread_create(&client->ping_thread,NULL,drpc_ping_server,client) == 0);

            ret = client;
            break;
        }
        host_list = host_list->ai_next;
    }
    host_list = host_list_org;
    freeaddrinfo(host_list);
    return ret;
}

void drpc_client_disconnect(struct drpc_client* client){
    if(client == NULL) return;
    if(client->client_stop != 0) return;
    client->client_stop = 1;
    pthread_join(client->ping_thread,NULL);
    free(client);
}


int drpc_client_call(struct drpc_client* client, char* fn_name, enum drpc_types* prototype, size_t prototype_len,void* native_return,...){
    assert(client);
    if(client->client_stop != 0) return DRPC_CLIENTSTOPPED;

    va_list varargs;
    va_start(varargs,native_return);

    struct drpc_type* arguments = calloc(prototype_len,sizeof(*arguments)); assert(arguments);
    struct queue* updated_arguments_que = queue_create();

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
                queue_push(updated_arguments_que,update);

                str_to_drpc(&arguments[i],arg);
                break;
            case d_sizedbuf:
                arg = va_arg(varargs,char*);

                update = malloc(sizeof(*update)); assert(update);
                update->type = d_sizedbuf;
                update->ptr = arg;
                update->len = va_arg(varargs,size_t);
                queue_push(updated_arguments_que,update);

                sizedbuf_to_drpc(&arguments[i],arg,update->len);
                break;
            case d_struct:
                arg = va_arg(varargs,void*);

                update = malloc(sizeof(*update)); assert(update);
                update->type = d_struct;
                update->ptr = arg;
                queue_push(updated_arguments_que,update);

                d_struct_to_drpc(&arguments[i],arg);
                break;
            case d_array:
                arg = va_arg(varargs,void*);

                update = malloc(sizeof(*update)); assert(update);
                update->type = d_array;
                update->ptr = arg;
                queue_push(updated_arguments_que,update);

                d_array_to_drpc(&arguments[i],arg);
                break;
            case d_queue:
                arg = va_arg(varargs,void*);

                update = malloc(sizeof(*update)); assert(update);
                update->type = d_queue;
                update->ptr = arg;
                queue_push(updated_arguments_que,update);

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
    if(drpc_send_message(client->io,&send) != 0){
        struct drpc_type_update* freeU = NULL;
        while((freeU = queue_pop(updated_arguments_que)) != NULL) free(freeU);

        queue_free(updated_arguments_que);
        pthread_mutex_unlock(&client->connection_mutex);
        return DRPC_ENETWORK;
    }

    if(drpc_recv_message(client->io,&recv) != 0){
        struct drpc_type_update* freeU = NULL;
        while((freeU = queue_pop(updated_arguments_que)) != NULL) free(freeU);

        queue_free(updated_arguments_que);
        pthread_mutex_unlock(&client->connection_mutex);
        return DRPC_ENETWORK;
    }
    if(recv.message_type != drpc_return){
        struct drpc_type_update* freeU = NULL;
        while((freeU = queue_pop(updated_arguments_que)) != NULL) free(freeU);

        queue_free(updated_arguments_que);
        pthread_mutex_unlock(&client->connection_mutex);
        return DRPC_BADREPLY;
    }
    pthread_mutex_unlock(&client->connection_mutex);

    struct drpc_return* ret = message_to_drpc_return(recv.message);
    d_struct_free(recv.message);

    int8_t return_is = -1;

    if(ret->returned.type == d_return_is){
        return_is = drpc_to_return_is(&ret->returned);
    }
    assert(queue_get_len(updated_arguments_que) == ret->updated_arguments_len);

    for(uint8_t i = 0 ; i < ret->updated_arguments_len ; i++){
        struct drpc_type_update* to_update = queue_pop(updated_arguments_que);

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

                break;
            case d_struct:
                struct d_struct* dstruct = drpc_to_d_struct(&ret->updated_arguments[i]);
                struct d_struct* doriginalS = to_update->ptr;
                d_struct_free_internal(doriginalS);

                *doriginalS = *dstruct;

                if(return_is == i) *(struct d_struct**)native_return = doriginalS;
                free(dstruct);
                break;
            case d_array:
                struct d_array* darray = drpc_to_d_array(&ret->updated_arguments[i]);
                struct d_array* doriginalA = to_update->ptr;
                d_array_free_internal(doriginalA);

                *doriginalA = *darray;

                if(return_is == i) *(struct d_array**)native_return = doriginalA;
                free(darray);
                break;
            case d_queue:
                struct d_queue* dqueue = drpc_to_d_queue(&ret->updated_arguments[i]);
                struct d_queue* doriginalQ = to_update->ptr;
                d_queue_free_internals(doriginalQ);

                *doriginalQ = *dqueue;

                if(return_is == i) *(struct d_queue**)native_return = doriginalQ;
                free(dqueue);
                break;
            default: break;
        }
        free(to_update);
    }
    queue_free(updated_arguments_que);
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
            case d_array:
                *(struct d_array**)native_return = drpc_to_d_array(&ret->returned);
                break;
            case d_queue:
                *(struct d_queue**)native_return = drpc_to_d_queue(&ret->returned);
                break;

            default: break;
        }
    }
    drpc_return_free(ret); free(ret);
    return DRPC_OK;
}

int drpc_client_mailbox_send(struct drpc_client* client, char* mailbox_name, struct d_queue* messages){
    if(client == NULL) return 1;
    if(client->client_stop != 0) return DRPC_CLIENTSTOPPED;
    struct d_struct* mailbox_msg = new_d_struct();
    d_struct_set(mailbox_msg,"receiver_mailbox",mailbox_name,d_str);
    d_struct_set(mailbox_msg,"messages",messages,d_queue);
    struct drpc_message msg = {
        .message_type = drpc_mailbox,
        .message = mailbox_msg,
    };
    if(drpc_send_message(client->io,&msg) != 0){
        d_struct_free(mailbox_msg);
        return DRPC_ENETWORK;
    }
    struct drpc_message recv;
    if(drpc_recv_message(client->io,&recv) != 0) return DRPC_ENETWORK;
    if(recv.message_type != drpc_ok) return DRPC_BADREPLY;

    return 0;
}

char* drpc_client_get_servername(struct drpc_client* client){
    assert(client);
    if(client->client_stop != 0) return NULL;

    struct drpc_message send;
    struct drpc_message recv;

    send.message = NULL;
    recv.message = NULL;
    recv.message_type = 0;

    send.message_type = drpc_servername;
    pthread_mutex_lock(&client->connection_mutex);

    if(drpc_send_message(client->io,&send) != 0){
        pthread_mutex_unlock(&client->connection_mutex);
        return NULL;
    }

    if(drpc_recv_message(client->io,&recv) != 0){
        pthread_mutex_unlock(&client->connection_mutex);
        return NULL;
    }
    pthread_mutex_unlock(&client->connection_mutex);

    assert(recv.message_type == drpc_servername);
    char* ret = NULL;
    d_struct_get(recv.message,"drpc_servername",&ret,d_str);
    d_struct_unlink(recv.message,"drpc_servername");
    d_struct_free(recv.message);
    return ret;
}

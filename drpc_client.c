#include "drpc_client.h"
#include "drpc_protocol.h"
#include "queue.h"
#include "drpc_queue.h"
#include "drpc_server.h"
#include "drpc_types.h"
#include "drpc_struct.h"
#include "drpc_array.h"

#include <stdarg.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <unistd.h> //sleep();

#ifdef DRPC_TCP_SUPPORT
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#endif


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
        sleep(DRPC_IO_TIMEOUT / 2);
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

#ifdef DRPC_TCP_SUPPORT
struct drpc_client* drpc_client_connect(char* host, char* username, char* passwd){
    assert(host != NULL); assert(username != NULL); assert(passwd != NULL);
    struct addrinfo hints = {
        .ai_flags = AI_NUMERICSERV,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = 0,
        .ai_addr = NULL,
        .ai_next = NULL,
    };
    hints.ai_flags = AI_PASSIVE;

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
#endif

void drpc_client_disconnect(struct drpc_client* client){
    if(client == NULL) return;
    if(client->client_stop != 0) return;
    client->client_stop = 1;
    pthread_join(client->ping_thread,NULL);
    free(client);
}

int drpc_client_call_internal(struct drpc_client* client, char* fn_name, enum drpc_types* prototype, size_t prototype_len,void* native_return,va_list varargs){
    if(client == NULL) return DRPC_BAD;
    if(client->client_stop != 0) return DRPC_CLIENTSTOPPED;

    struct d_array* arguments = new_d_array(prototype_len);
    struct d_struct* call_message = new_d_struct();

    d_struct_set(call_message,"fn_name",fn_name,d_str);
    d_struct_set(call_message,"arguments",arguments,d_array);
    queue_t repackable = queue_create();

    for(size_t i = 0; i < prototype_len; i++){
        if(prototype[i] == d_array || prototype[i] == d_struct || prototype[i] == d_queue || prototype[i] == d_sizedbuf || prototype[i] == d_str){
            void* ptr = va_arg(varargs,void*);
            size_t sizedbuf_len = 0;
            if(prototype[i] == d_sizedbuf)
                sizedbuf_len = va_arg(varargs,size_t);
            d_array_set(arguments,i,ptr,prototype[i],sizedbuf_len);
            struct drpc_type_update* repack = malloc(sizeof(*repack)); assert(repack);
            repack->type = prototype[i];
            repack->ptr = ptr;
            repack->len = sizedbuf_len;
            queue_push(repackable,repack);
            continue;
        }
        uint64_t generic_input = 0; //should be enough to fit everything
        if(prototype[i] == d_uint8 || prototype[i] == d_int8 || prototype[i] == d_uint16 || prototype[i] == d_int16){
            generic_input = va_arg(varargs,int);
        }
        switch(prototype[i]){
            case d_uint32:
                generic_input = va_arg(varargs,uint32_t);
                break;
            case d_int32:
                generic_input = va_arg(varargs,int32_t);
                break;
            case d_uint64:
                generic_input = va_arg(varargs,uint64_t);
                break;
            case d_int64:
                generic_input = va_arg(varargs,int64_t);
                break;
            case d_float:
            case d_double:
                generic_input = va_arg(varargs,double);
                break;
            default: break;
        }
        d_array_set(arguments,i,&generic_input,prototype[i]);
    }
    struct drpc_message recv, send;

    send.message_type = drpc_call;
    send.message = call_message;
    pthread_mutex_lock(&client->connection_mutex);
    if(drpc_send_message(client->io,&send) != 0){
        pthread_mutex_unlock(&client->connection_mutex);
        return DRPC_ENETWORK;
    }
    if(drpc_recv_message(client->io,&recv) != 0){
        pthread_mutex_unlock(&client->connection_mutex);
        return DRPC_ENETWORK;
    }
    pthread_mutex_unlock(&client->connection_mutex);
    if(recv.message_type != drpc_return){
        d_struct_free(recv.message);
        return DRPC_BADREPLY;
    }
    int8_t return_is = -1;
    struct d_array* repacked_arguments = NULL;
    d_struct_get(recv.message,"return_is",&return_is,d_int8);
    if(d_struct_get(recv.message,"repacked_arguments",&repacked_arguments,d_array) == 0){
        size_t repacked_len = d_array_len(repacked_arguments);
        for(size_t i = 0; i < repacked_len; i++){
            enum drpc_types type = d_array_get_type(repacked_arguments,i);
            struct drpc_type_update* repack = queue_pop(repackable);
            assert(repack);
            assert(repack->type == type);
            switch(type){
                case d_struct:
                    struct d_struct* original_dstruct = repack->ptr;
                    struct d_struct* repacked_DS = NULL;
                    assert(d_array_get(repacked_arguments,i,&repacked_DS,type) == 0);
                    assert(d_array_unlink(repacked_arguments,i) == 0);
                    if(original_dstruct != repacked_DS){  //hapens when using TCP
                        d_struct_free_internal(original_dstruct);
                        *original_dstruct = *repacked_DS;
                        free(repacked_DS);
                    }
                    if(return_is == i) *(struct d_struct**)native_return = original_dstruct;
                    break;

                case d_array:
                    struct d_array* original_darray = repack->ptr;
                    struct d_array* repacked_A = NULL;
                    assert(d_array_get(repacked_arguments,i,&repacked_A,type) == 0);
                    assert(d_array_unlink(repacked_arguments,i) == 0);
                    if(original_darray != repacked_A){  //hapens when using TCP
                        d_array_free_internal(original_darray);
                        *original_darray = *repacked_A;
                        free(repacked_A);
                    }
                    if(return_is == i) *(struct d_array**)native_return = original_darray;
                    break;

                case d_queue:
                    struct d_queue* original_dqueue = repack->ptr;
                    struct d_queue* repacked_Q = NULL;
                    assert(d_array_get(repacked_arguments,i,&repacked_Q,type) == 0);
                    assert(d_array_unlink(repacked_arguments,i) == 0);
                    if(original_dqueue != repacked_Q){ //hapens when using TCP
                        d_queue_free_internals(original_dqueue);
                        *original_dqueue = *repacked_Q;
                        free(repacked_Q);
                    }
                    if(return_is == i) *(struct d_queue**)native_return = repacked_Q;
                    break;

                case d_str:
                    char* original_str = repack->ptr;
                    char* repacked_STR = NULL;
                    assert(d_array_get(repacked_arguments,i,&repacked_STR,type) == 0);
                    assert(d_array_unlink(repacked_arguments,i) == 0);
                    if(original_str != repacked_STR){ //hapens when using TCP
                        size_t strcpy_len = (strlen(original_str) > strlen(repacked_STR) ? strlen(repacked_STR) : strlen(original_str)) + 1;
                        memcpy(original_str,repacked_STR,strcpy_len);
                        free(repacked_STR);
                    }
                    if(return_is == i) *(char**)native_return = repacked_STR;
                    break;

                case d_sizedbuf:
                    size_t szbuf_len = 0;
                    char* original_szbuf = repack->ptr;
                    char* repacked_SZBUF = NULL;
                    assert(d_array_get(repacked_arguments,i,&repacked_SZBUF,type,&szbuf_len) == 0);
                    memcpy(original_szbuf,repacked_SZBUF,szbuf_len);
                    break;

                default: break;
            }
            free(repack);
        }

    }
    if(return_is == -1){ //return isnt same as one of arguments
        enum drpc_types type = d_struct_get_type(recv.message,"return");
        if(type != d_void){
            assert(d_struct_get(recv.message,"return",native_return,type) == 0);
            d_struct_unlink(recv.message,"return");
        }
    }
    queue_free(repackable);
    d_struct_free(recv.message);

    return 0;
}

int drpc_client_call(struct drpc_client* client, char* fn_name, enum drpc_types* prototype, size_t prototype_len,void* native_return,...){
    va_list varargs;
    va_start(varargs,native_return);
    return drpc_client_call_internal(client,fn_name,prototype,prototype_len,native_return,varargs);
}

int drpc_client_mailbox_send(struct drpc_client* client, char* mailbox_name, struct d_queue* messages){
    if(client == NULL) return DRPC_BAD;
    if(client->client_stop != 0) return DRPC_CLIENTSTOPPED;
    pthread_mutex_lock(&client->connection_mutex);

    struct drpc_message send = {
        .message_type = drpc_mailbox_recv,
        .message = new_d_struct(),
    };
    d_struct_set(send.message,"receiver_mailbox",mailbox_name,d_str);
    d_struct_set(send.message,"messages",messages,d_queue);
    if(drpc_send_message(client->io,&send) != 0){
        pthread_mutex_unlock(&client->connection_mutex);
        return DRPC_ENETWORK;
    }
    struct drpc_message recv;
    if(drpc_recv_message(client->io,&recv) != 0){
        pthread_mutex_unlock(&client->connection_mutex);
        return DRPC_ENETWORK;
    }
    if(recv.message_type != drpc_ok){
        pthread_mutex_unlock(&client->connection_mutex);
        return DRPC_ENETWORK;
    }
    pthread_mutex_unlock(&client->connection_mutex);
    return DRPC_OK;
}

int drpc_client_mailbox_recv(struct drpc_client* client, char* mailbox_name, struct d_queue* output){
    if(output == NULL) return DRPC_BAD;
    if(client == NULL) return DRPC_BAD;
    if(client->client_stop != 0) return DRPC_CLIENTSTOPPED;
    pthread_mutex_lock(&client->connection_mutex);
    struct drpc_message send = {
        .message_type = drpc_mailbox_send,
        .message = new_d_struct(),
    };
    d_struct_set(send.message,"sender_mailbox",mailbox_name,d_str);
    if(drpc_send_message(client->io,&send) != 0){
        pthread_mutex_unlock(&client->connection_mutex);
        return DRPC_ENETWORK;
    }
    struct drpc_message recv;
    if(drpc_recv_message(client->io,&recv) != 0){
        pthread_mutex_unlock(&client->connection_mutex);
        return DRPC_ENETWORK;
    }
    if(recv.message_type != drpc_ok || recv.message == NULL){
        pthread_mutex_unlock(&client->connection_mutex);
        return DRPC_ENETWORK;
    }

    struct d_queue* messages;
    assert(d_struct_get(recv.message,"messages",&messages,d_queue) == 0);

    size_t messages_len = d_queue_len(messages);
    for(size_t i = 0; i < messages_len; i++){
        queue_push(output->que,queue_pop(messages->que)); //i know this looks bad, but i made no api to automaticly copy one queue to another
    }
    d_struct_free(recv.message);
    pthread_mutex_unlock(&client->connection_mutex);
    return DRPC_OK;
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

void drpc_client_set_userdata(struct drpc_client* client, void* userdata){
    if(client == NULL) return;
    client->userdata = userdata;
}

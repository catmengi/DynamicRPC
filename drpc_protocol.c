#include "drpc_protocol.h"
#include "drpc_struct.h"
#include "drpc_types.h"
#include <time.h>

#ifdef DRPC_TCP_SUPPORT
#include "aes.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <pthread.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void drpc_call_free(struct drpc_call* call){
    free(call->fn_name);

    drpc_types_free(call->arguments,call->arguments_len);

}
void drpc_return_free(struct drpc_return* ret){
    drpc_type_free(&ret->returned);
    drpc_types_free(ret->updated_arguments,ret->updated_arguments_len);
}

struct d_struct* drpc_call_to_message(struct drpc_call* call){
    struct d_struct* message = new_d_struct();

    size_t arguments_buflen = drpc_types_buflen(call->arguments,call->arguments_len);
    char* arguments_buf = malloc(arguments_buflen); assert(arguments_buf);
    drpc_types_buf(call->arguments,call->arguments_len,arguments_buf);

    d_struct_set(message,"packed_arguments",arguments_buf,d_sizedbuf,arguments_buflen);
    free(arguments_buf);

    d_struct_set(message,"fn_name", call->fn_name, d_str);

    return message;
}

struct drpc_call* message_to_drpc_call(struct d_struct* message){
    struct drpc_call* call = calloc(1,sizeof(*call));

    size_t unused = 0;
    char* packed_arguments = NULL;
    if(d_struct_get(message,"packed_arguments",&packed_arguments,d_sizedbuf,&unused) != 0){
        free(call);
        return NULL;
    }

    size_t unpacked_len = 0;
    call->arguments = buf_drpc_types(packed_arguments,&unpacked_len);
    call->arguments_len = (uint8_t)unpacked_len;

    if(d_struct_get(message,"fn_name",&call->fn_name,d_str) != 0){
        drpc_types_free(call->arguments,call->arguments_len);
        free(call);
        return NULL;
    }
    d_struct_unlink(message,"fn_name");
    return call;
}

struct d_struct* drpc_return_to_message(struct drpc_return* drpc_return){
    struct d_struct* message = new_d_struct();

    size_t arguments_buflen = drpc_types_buflen(drpc_return->updated_arguments,drpc_return->updated_arguments_len);
    char* arguments_buf = malloc(arguments_buflen); assert(arguments_buf);
    drpc_types_buf(drpc_return->updated_arguments,drpc_return->updated_arguments_len,arguments_buf);

    d_struct_set(message,"updated_arguments",arguments_buf,d_sizedbuf,arguments_buflen);
    free(arguments_buf);

    size_t returned_buflen = drpc_buflen(&drpc_return->returned);
    char* returned_buf = malloc(returned_buflen);
    drpc_buf(&drpc_return->returned,returned_buf);

    d_struct_set(message,"return",returned_buf,d_sizedbuf,returned_buflen);
    free(returned_buf);


    return message;
}

struct drpc_return* message_to_drpc_return(struct d_struct* message){
    struct drpc_return* drpc_return = calloc(1,sizeof(*drpc_return));

    size_t unused = 0;
    char* updated_arguments_buf = NULL;
    if(d_struct_get(message,"updated_arguments",&updated_arguments_buf,d_sizedbuf,&unused) != 0){
        free(drpc_return);
        return NULL;
    }

    size_t updated_arguments_len = 0;
    drpc_return->updated_arguments = buf_drpc_types(updated_arguments_buf,&updated_arguments_len);
    drpc_return->updated_arguments_len = (uint8_t)updated_arguments_len;

    char* returned_buf = NULL;
    if(d_struct_get(message,"return",&returned_buf,d_sizedbuf,&unused) != 0){
        drpc_types_free(drpc_return->updated_arguments,drpc_return->updated_arguments_len);
        free(drpc_return);
        return NULL;
    }

    buf_drpc(&drpc_return->returned,returned_buf);
    return drpc_return;
}

size_t nextby16 (size_t value) {
    if (value % 16 == 0) {
        return value; // Already divisible by 16
    }
    return (value / 16 + 1) * 16; // Calculate the next multiple of 16
}

uint8_t iv[]  = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

int drpc_send_message(struct drpc_io* io,struct drpc_message* msg){
    if(io == NULL) return 1;
    struct d_struct* message = new_d_struct();

    d_struct_set(message,"message_type",&msg->message_type,d_uint8);
    if(msg->message != NULL){
        d_struct_set(message,"message",msg->message,d_struct);
    }
    return io->send(io,message);
}

int drpc_recv_message(struct drpc_io* io,struct drpc_message* msg){
    if(io == NULL) return 1;
    struct d_struct* container = NULL;
    int ret = io->recv(io,&container);
    if(ret != 0){
        d_struct_free(container);
        return 1;
    }
    assert(d_struct_get(container,"message_type",&msg->message_type,d_uint8) == 0);
    if(d_struct_get(container,"message",&msg->message,d_struct) == 0){
        d_struct_unlink(container,"message");
    }
    d_struct_free(container);
    return 0;
}

#ifdef DRPC_DQUEUE_IO_SUPPORT
void drpc_dqueue_close(struct drpc_io* io){
    if(io->io_data == NULL) return;
    struct drpc_dqueue_io* io_data = io->io_data;
    struct drpc_dqueue_io* receiver_io_data = io_data->send;
    pthread_mutex_lock(&io_data->lock);
    pthread_mutex_lock(&io_data->wait_lock);
    if(receiver_io_data != NULL){
        io_data->send = NULL;
        pthread_mutex_lock(&receiver_io_data->lock);
        pthread_mutex_lock(&receiver_io_data->wait_lock);
        receiver_io_data->send = NULL;
        pthread_mutex_unlock(&receiver_io_data->lock);
        pthread_mutex_unlock(&receiver_io_data->wait_lock);
    }
    sem_destroy(&io_data->recv_wait);
    d_queue_free(io_data->recv);
    io_data->recv = NULL;

    pthread_mutex_unlock(&io_data->lock);
    pthread_mutex_unlock(&io_data->wait_lock);
}
void drpc_dqueue_free(struct drpc_io* io){
    struct drpc_dqueue_io* io_data = io->io_data;
    free(io->io_data);
    free(io);
}

int drpc_dqueue_send_message(struct drpc_io* io, struct d_struct* prepacked_message){
    if(io->io_data == NULL){
        d_struct_free(prepacked_message);
        return 1;
    }
    struct drpc_dqueue_io* io_data = io->io_data;
    if(io_data->send == NULL){
        d_struct_free(prepacked_message);
        return 1;
    }
    pthread_mutex_lock(&io_data->lock);
    pthread_mutex_lock(&io_data->send->lock);

    d_queue_push(io_data->send->recv,prepacked_message,d_struct);
    assert(sem_post(&io_data->send->recv_wait) == 0);

    pthread_mutex_unlock(&io_data->send->lock);
    pthread_mutex_unlock(&io_data->lock);
    return 0;
}
int drpc_dqueue_recv_message(struct drpc_io* io, struct d_struct** output_pointer){
    if(io->io_data == NULL) return 1;
    struct drpc_dqueue_io* io_data = io->io_data;
    struct timespec timeout;;

    clock_gettime(CLOCK_REALTIME, &timeout);

    timeout.tv_sec += DRPC_IO_TIMEOUT;

    pthread_mutex_lock(&io_data->wait_lock); //close in middle of semaphore wait will break everything
    int ret = 1;
    if(io_data->recv != NULL){
        if(sem_timedwait(&io_data->recv_wait,&timeout) == 0)
            if(d_queue_pop(io_data->recv,output_pointer,d_struct) == 0)
                ret = 0;
    }
    pthread_mutex_unlock(&io_data->wait_lock);
    return ret;
}
#endif

#ifdef DRPC_TCP_SUPPORT
void drpc_tcp_close(struct drpc_io* io){
    close(*(int*)io->io_data);
}
void drpc_tcp_free(struct drpc_io* io){
    free(io->aes128_key);
    free(io->io_data);
    free(io);
}

int tcp_send_loop(int fd, void* buf, size_t buflen){
    size_t sent = 0;
    while(sent < buflen){
        size_t cur_sent = send(fd,buf + sent, buflen - sent,MSG_NOSIGNAL);
        if(cur_sent <= 0) break;
        sent += cur_sent;
    }
    if(sent == buflen) return 0;
    else return 1;
}

int tcp_recv_loop(int fd, void* buf, size_t buflen){
    size_t received = 0;
    while(received < buflen){
        size_t cur_sent = recv(fd,buf + received, buflen - received,MSG_NOSIGNAL);
        if(cur_sent <= 0) break;
        received += cur_sent;
    }
    if(received == buflen) return 0;
    else return 1;
}

int drpc_tcp_send_message(struct drpc_io* io, struct d_struct* prepacked_message){
     size_t message_buflen = 0;
     char* send_buf = d_struct_buf(prepacked_message,&message_buflen);

     uint64_t send_buflen = nextby16(message_buflen);
     assert((send_buf = realloc(send_buf,send_buflen)) != NULL);

     memset(send_buf + message_buflen,0,send_buflen - message_buflen);

     char drpc_message_header[sizeof(uint64_t) + sizeof(DRPC_SIGNATURE)];
     memcpy(drpc_message_header,DRPC_SIGNATURE,sizeof(DRPC_SIGNATURE));
     memcpy(drpc_message_header + sizeof(DRPC_SIGNATURE),&send_buflen,sizeof(uint64_t));

     if(tcp_send_loop(*(int*)io->io_data,drpc_message_header,sizeof(drpc_message_header)) != 0){
         d_struct_free(prepacked_message);
         free(send_buf);
         return 1;
    }
    if(io->aes128_key != NULL && DRPC_SIGNATURE[strlen(DRPC_SIGNATURE) - 1] == 'E'){
        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx,io->aes128_key,iv);
        AES_CBC_encrypt_buffer(&ctx,(uint8_t*)send_buf,(size_t)send_buflen);
    }

    int ret = tcp_send_loop(*(int*)io->io_data,send_buf,send_buflen);
    d_struct_free(prepacked_message);
    free(send_buf);
    return ret;
}

int drpc_tcp_recv_message(struct drpc_io* io, struct d_struct** container){
    uint64_t recv_buflen = 0;
    char drpc_message_header[sizeof(uint64_t) + sizeof(DRPC_SIGNATURE)];

    if(tcp_recv_loop(*(int*)io->io_data,&drpc_message_header,sizeof(drpc_message_header)) != 0) return 1;
    if(strcmp(drpc_message_header,DRPC_SIGNATURE) != 0) return 1; // NOT A DRPC MESSAGE;
    memcpy(&recv_buflen,drpc_message_header + sizeof(DRPC_SIGNATURE),sizeof(uint64_t));

    char* recv_buf = malloc(recv_buflen); assert(recv_buf);
    int ret = tcp_recv_loop(*(int*)io->io_data,recv_buf,recv_buflen);

    if(io->aes128_key != NULL && DRPC_SIGNATURE[strlen(DRPC_SIGNATURE) - 1] == 'E'){
        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx,io->aes128_key,iv);
        AES_CBC_decrypt_buffer(&ctx,(uint8_t*)recv_buf,(size_t)recv_buflen);
    }

    *container = new_d_struct();
    buf_d_struct(recv_buf,*container);
    free(recv_buf);
    return 0;
}
#endif


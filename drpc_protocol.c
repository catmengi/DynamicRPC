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
    if(io == NULL) return;
    if(io->io_data == NULL) return;
    struct drpc_dqueue_io* io_data = io->io_data;
    pthread_mutex_lock(&io_data->wait_lock);

    sem_destroy(&io_data->recv_wait);
    d_queue_free(io_data->recv);

    if(io_data->send != NULL)
        io_data->send->send = NULL;

    io_data->recv = NULL;
    io_data->send = NULL;

    pthread_mutex_unlock(&io_data->wait_lock);
}
void drpc_dqueue_free(struct drpc_io* io){
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
    d_queue_push(io_data->send->recv,prepacked_message,d_struct);
    assert(sem_post(&io_data->send->recv_wait) == 0);
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
    if(tcp_recv_loop(*(int*)io->io_data,recv_buf,recv_buflen) != 0) {free(recv_buf); return 1;}

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


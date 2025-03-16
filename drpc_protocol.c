#include "drpc_protocol.h"
#include "drpc_struct.h"
#include "drpc_types.h"

#include "aes.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DRPC_SIGNATURE "DRPCv211+E" // DRPC_SIGNATURE FORMAT: DPRC - name ; v:
                                    // FIRST DIGIT -- code version (changes ????).
                                    // SECOND DIGIT AND THIRD -- network compat version(changes on massive updates),
                                    // LAST LETTER: E -- encryption enabled, other letter -- encryption disabled

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
    d_struct_unlink(message,"fn_name",d_str);
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

//Code below was EDITED by chatGPT

int drpc_send_message(struct drpc_message* msg,uint8_t* aes128_key,int fd){
    struct d_struct* message = new_d_struct();

    uint8_t type = msg->message_type;
    d_struct_set(message, "msg_type", &type, d_uint8);

    if (msg->message != NULL)
        d_struct_set(message, "msg", msg->message, d_struct);

    size_t structbuf_len = 0;
    char* send_buf = d_struct_buf(message, &structbuf_len); assert(send_buf);

    uint64_t send_len = nextby16(structbuf_len + sizeof(DRPC_SIGNATURE));
    assert((send_buf = realloc(send_buf,send_len)) != NULL);

    if(aes128_key && DRPC_SIGNATURE[strlen(DRPC_SIGNATURE) - 1] == 'E'){
        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx,aes128_key,iv);
        AES_CBC_encrypt_buffer(&ctx,(uint8_t*)send_buf,send_len);
    }

    if(msg->message != NULL)
        d_struct_unlink(message, "msg", d_struct);

    // Send the length of the message
    char drpc_message_header[sizeof(uint64_t) + sizeof(DRPC_SIGNATURE)];
    memcpy(drpc_message_header,DRPC_SIGNATURE,sizeof(DRPC_SIGNATURE));
    memcpy(drpc_message_header + sizeof(DRPC_SIGNATURE),&send_len,sizeof(uint64_t));

    if (send(fd, drpc_message_header, sizeof(drpc_message_header), MSG_NOSIGNAL) != sizeof(drpc_message_header)){
        d_struct_free(message);
        free(send_buf);
        return 1;  // Error sending length
    }

    // Send the message in chunks
    size_t total_sent = 0;
    while (total_sent < send_len) {
        ssize_t bytes_sent = send(fd, send_buf + total_sent, send_len - total_sent, MSG_NOSIGNAL);
        if (bytes_sent <= 0) {
            d_struct_free(message);
            free(send_buf);
            return 1;  // Error sending message
        }
        total_sent += bytes_sent;
    }

    d_struct_free(message);
    free(send_buf);
    return 0;  // Success
}

int drpc_recv_message(struct drpc_message* msg,uint8_t* aes128_key,int fd){
    uint64_t drpc_message_len = 0;
    char drpc_message_header[sizeof(uint64_t) + sizeof(DRPC_SIGNATURE)];
    // Receive the length of the incoming message
    if (recv(fd, drpc_message_header, sizeof(drpc_message_header), MSG_NOSIGNAL) != sizeof(drpc_message_header)) {
        return 1;  // Error receiving length
    }
    if(strcmp(drpc_message_header,DRPC_SIGNATURE) != 0) return 1; // NOT A DRPC MESSAGE;
    memcpy(&drpc_message_len,drpc_message_header + sizeof(DRPC_SIGNATURE),sizeof(uint64_t));

    // Allocate buffer for the incoming message
    char* buf = malloc(drpc_message_len);assert(buf);

    // Receive the message in chunks
    size_t total_received = 0;
    while (total_received < drpc_message_len) {
        ssize_t bytes_received = recv(fd, buf + total_received, drpc_message_len - total_received, MSG_NOSIGNAL);
        if (bytes_received <= 0) {
            free(buf);
            return 1;  // Error receiving message
        }
        total_received += bytes_received;
    }

    if(aes128_key && DRPC_SIGNATURE[strlen(DRPC_SIGNATURE) - 1] == 'E'){
        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx,aes128_key,iv);
        AES_CBC_decrypt_buffer(&ctx,(uint8_t*)buf,(size_t)drpc_message_len);
    }

    struct d_struct* container = new_d_struct();
    buf_d_struct(buf, container);
    free(buf);

    uint8_t utype = 0;

    d_struct_get(container, "msg_type", &utype, d_uint8);
    msg->message_type = utype;

    if (d_struct_get(container, "msg", &msg->message, d_struct) == 0) {
        assert(d_struct_unlink(container, "msg", d_struct) == 0);
    }

    d_struct_free(container);
    return 0;  // Success
}


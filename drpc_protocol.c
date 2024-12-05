#include "drpc_protocol.h"
#include "drpc_struct.h"
#include "drpc_types.h"

#include <netinet/in.h>
#include <arpa/inet.h>
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

struct d_struct* drpc_call_to_massage(struct drpc_call* call){
    struct d_struct* massage = new_d_struct();

    size_t arguments_buflen = drpc_types_buflen(call->arguments,call->arguments_len);
    char* arguments_buf = malloc(arguments_buflen); assert(arguments_buf);
    drpc_types_buf(call->arguments,call->arguments_len,arguments_buf);

    d_struct_set(massage,"packed_arguments",arguments_buf,d_sizedbuf,arguments_buflen);
    free(arguments_buf);

    d_struct_set(massage,"fn_name", call->fn_name, d_str);

    return massage;
}

struct drpc_call* massage_to_drpc_call(struct d_struct* massage){
    struct drpc_call* call = calloc(1,sizeof(*call));

    size_t unused = 0;
    char* packed_arguments = NULL;
    if(d_struct_get(massage,"packed_arguments",&packed_arguments,d_sizedbuf,&unused) != 0){
        free(call);
        return NULL;
    }

    size_t unpacked_len = 0;
    call->arguments = buf_drpc_types(packed_arguments,&unpacked_len);
    call->arguments_len = (uint8_t)unpacked_len;

    if(d_struct_get(massage,"fn_name",&call->fn_name,d_str) != 0){
        drpc_types_free(call->arguments,call->arguments_len);
        free(call);
        return NULL;
    }
    d_struct_unlink(massage,"fn_name",d_str);
    return call;
}

struct d_struct* drpc_return_to_massage(struct drpc_return* drpc_return){
    struct d_struct* massage = new_d_struct();

    size_t arguments_buflen = drpc_types_buflen(drpc_return->updated_arguments,drpc_return->updated_arguments_len);
    char* arguments_buf = malloc(arguments_buflen); assert(arguments_buf);
    drpc_types_buf(drpc_return->updated_arguments,drpc_return->updated_arguments_len,arguments_buf);

    d_struct_set(massage,"updated_arguments",arguments_buf,d_sizedbuf,arguments_buflen);
    free(arguments_buf);

    size_t returned_buflen = drpc_buflen(&drpc_return->returned);
    char* returned_buf = malloc(returned_buflen);
    drpc_buf(&drpc_return->returned,returned_buf);

    d_struct_set(massage,"return",returned_buf,d_sizedbuf,returned_buflen);
    free(returned_buf);


    return massage;
}

struct drpc_return* massage_to_drpc_return(struct d_struct* massage){
    struct drpc_return* drpc_return = calloc(1,sizeof(*drpc_return));

    size_t unused = 0;
    char* updated_arguments_buf = NULL;
    if(d_struct_get(massage,"updated_arguments",&updated_arguments_buf,d_sizedbuf,&unused) != 0){
        free(drpc_return);
        return NULL;
    }

    size_t updated_arguments_len = 0;
    drpc_return->updated_arguments = buf_drpc_types(updated_arguments_buf,&updated_arguments_len);
    drpc_return->updated_arguments_len = (uint8_t)updated_arguments_len;

    char* returned_buf = NULL;
    if(d_struct_get(massage,"return",&returned_buf,d_sizedbuf,&unused) != 0){
        drpc_types_free(drpc_return->updated_arguments,drpc_return->updated_arguments_len);
        free(drpc_return);
        return NULL;
    }

    buf_drpc(&drpc_return->returned,returned_buf);
    return drpc_return;
}

int drpc_send_massage(struct drpc_massage* msg, int fd){
    struct d_struct* massage = new_d_struct();

    uint8_t type = msg->massage_type;
    d_struct_set(massage,"msg_type",&type,d_uint8);

    if(msg->massage)
        d_struct_set(massage,"msg",msg->massage,d_struct);

    size_t massage_len = 0;
    char* send_buf = d_struct_buf(massage,&massage_len); uint64_t send_len = massage_len;

    d_struct_unlink(massage,"msg",d_struct);


    int ret = 0;
    if(send(fd,&send_len,sizeof(uint64_t),MSG_NOSIGNAL) != sizeof(uint64_t)){
        ret = 1;
        goto exit;
    }
    if(send(fd,send_buf,massage_len,MSG_NOSIGNAL) != massage_len){
        ret = 1;
        goto exit;
    }

exit:
    d_struct_free(massage);
    free(send_buf);
    return ret;
}
int drpc_recv_massage(struct drpc_massage* msg, int fd){
    uint64_t len64 = 0;
    if(recv(fd,&len64,sizeof(uint64_t),MSG_NOSIGNAL) != sizeof(uint64_t)) return 1;

    size_t recvlen = len64;
    char* buf = calloc(recvlen,sizeof(char)); assert(buf);
    if(recv(fd,buf,recvlen,MSG_NOSIGNAL) != recvlen){free(buf); return 1;}

    struct d_struct* container = new_d_struct();
    buf_d_struct(buf,container);
    free(buf);

    uint8_t utype = 0;

    d_struct_get(container,"msg_type",&utype,d_uint8);
    msg->massage_type = utype;

    if(d_struct_get(container,"msg",&msg->massage,d_struct) == 0)
        d_struct_unlink(container,"msg",d_struct);

    d_struct_free(container);

    return 0;
}

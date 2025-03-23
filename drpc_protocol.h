//привет ворлд!!! Ну привет
#pragma once
#include <pthread.h>
#include <sys/types.h>
#include "drpc_types.h"

enum drpc_protocol{
    drpc_call,           //this used for function call request
    drpc_return,         //this is answer for drpc_call that carries function return
    drpc_servername,     //gets server's name string, if it is NULL it would set to "UNKNOWN_DRPC"

    drpc_nofn,           //function doesnt exist
    drpc_bad,            //arguments missmatch or other errors
    drpc_ok,             //auth ok
    drpc_eperm,

    drpc_auth,

    drpc_send_delayed,   //this sends message for function fn_name, this message will appear in d_delayed_message_queue type. function isnt called by this server protocol method

    drpc_disconnect,

    drpc_ping,
};

struct drpc_connection;

typedef int (*drpc_message_io_send)(struct drpc_connection* io, struct d_struct* prepacked_message);
typedef int (*drpc_message_io_recv)(struct drpc_connection* io, struct d_struct** prepacked_message);
typedef void (*drpc_free_io)(struct drpc_connection* io);

typedef void (*drpc_close_io)(struct drpc_connection* io);

struct drpc_connection{
    void* io_data;
    uint8_t* aes128_key;

    drpc_message_io_recv recv;
    drpc_message_io_send send;

    drpc_close_io close;
    drpc_free_io free;

};

struct drpc_call{
    char* fn_name;

    uint8_t arguments_len;
    struct drpc_type* arguments;
};

struct drpc_return{
    struct drpc_type returned;

    uint8_t updated_arguments_len;
    struct drpc_type* updated_arguments;

};

struct drpc_message{
    uint8_t message_type;
    struct d_struct* message;
};

struct d_struct* drpc_call_to_message(struct drpc_call* call);
struct drpc_call* message_to_drpc_call(struct d_struct* message);
struct d_struct* drpc_return_to_message(struct drpc_return* drpc_return);
struct drpc_return* message_to_drpc_return(struct d_struct* message);

int drpc_send_message(struct drpc_connection* io, struct drpc_message* msg);
int drpc_recv_message(struct drpc_connection* io, struct drpc_message* msg);

void drpc_tcp_close(struct drpc_connection* io);
void drpc_tcp_free(struct drpc_connection* io);
int drpc_tcp_send_message(struct drpc_connection* io, struct d_struct* prepacked_message);
int drpc_tcp_recv_message(struct drpc_connection* io, struct d_struct** container);

void drpc_call_free(struct drpc_call* call);
void drpc_return_free(struct drpc_return* ret);


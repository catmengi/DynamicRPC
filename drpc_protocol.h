//привет ворлд!!! Ну привет
#pragma once
#include <sys/types.h>
#include "drpc_types.h"

enum drpc_protocol{
    drpc_call,           //this used for function call request
    drpc_return,         //this is answer for drpc_call that carries function return

    drpc_nofn,           //function doesnt exist
    drpc_bad,            //arguments missmatch or other errors
    drpc_ok,             //auth ok

    drpc_auth,

    drpc_send_delayed,   //this sends massage for function fn_name, this massage will appear in d_delayed_massage_queue type. function isnt called by this server protocol method

    drpc_disconnect,

    drpc_ping,
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

struct drpc_massage{
    enum drpc_protocol massage_type;
    struct d_struct* massage;
};

struct d_struct* drpc_call_to_massage(struct drpc_call* call);
struct drpc_call* massage_to_drpc_call(struct d_struct* massage);
struct d_struct* drpc_return_to_massage(struct drpc_return* drpc_return);
struct drpc_return* massage_to_drpc_return(struct d_struct* massage);

int drpc_send_massage(struct drpc_massage* msg, int fd);
int drpc_recv_massage(struct drpc_massage* msg, int fd);

void drpc_call_free(struct drpc_call* call);
void drpc_return_free(struct drpc_return* ret);

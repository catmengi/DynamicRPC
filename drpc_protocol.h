//привет ворлд!!! Ну привет
#pragma once
#include <pthread.h>
#include <sys/types.h>
#include "drpc_types.h"

#define DRPC_IO_TIMEOUT 10
#define DRPC_DQUEUE_IO

#define DRPC_SIGNATURE "DRPCv240+E" // DRPC_SIGNATURE FORMAT: DPRC - name ; v:
                                    // FIRST DIGIT -- code version (changes ????).
                                    // SECOND DIGIT AND THIRD -- network compat version(changes on massive updates),
                                    // LAST LETTER: E -- encryption enabled, other letter -- encryption disabled

enum drpc_protocol{
    drpc_call,           //this used for function call request
    drpc_return,         //this is answer for drpc_call that carries function return
    drpc_servername,     //gets server's name string, if it is NULL it would set to "UNKNOWN_DRPC"
    drpc_auth,
    drpc_mailbox_recv,
    drpc_mailbox_send,
    drpc_disconnect,
    drpc_ping,

    drpc_notfound,       //function doesnt exist
    drpc_bad,            //arguments missmatch or other errors
    drpc_ok,             //ok
    drpc_eperm,
};

struct drpc_io;

typedef int (*drpc_io_send)(struct drpc_io* io, struct d_struct* prepacked_message);
typedef int (*drpc_io_recv)(struct drpc_io* io, struct d_struct** output_pointer);
typedef void (*drpc_io_free)(struct drpc_io* io);
typedef void (*drpc_io_close)(struct drpc_io* io);

struct drpc_io{
    void* io_data;
    uint8_t* aes128_key;

    drpc_io_recv recv;
    drpc_io_send send;
    drpc_io_close close;
    drpc_io_free free;
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

#ifdef DRPC_DQUEUE_IO
struct drpc_dqueue_io{
    pthread_mutex_t lock;
    struct d_queue* recv;
    struct drpc_dqueue_io* send;
};
#endif

struct d_struct* drpc_call_to_message(struct drpc_call* call);
struct drpc_call* message_to_drpc_call(struct d_struct* message);
struct d_struct* drpc_return_to_message(struct drpc_return* drpc_return);
struct drpc_return* message_to_drpc_return(struct d_struct* message);

int drpc_send_message(struct drpc_io* io, struct drpc_message* msg);
int drpc_recv_message(struct drpc_io* io, struct drpc_message* msg);

void drpc_tcp_close(struct drpc_io* io);
void drpc_tcp_free(struct drpc_io* io);
int drpc_tcp_send_message(struct drpc_io* io, struct d_struct* prepacked_message);
int drpc_tcp_recv_message(struct drpc_io* io, struct d_struct** container);

#ifdef DRPC_DQUEUE_IO
void drpc_dqueue_close(struct drpc_io* io);
void drpc_dqueue_free(struct drpc_io* io);
int drpc_dqueue_send_message(struct drpc_io* io, struct d_struct* prepacked_message);
int drpc_dqueue_recv_message(struct drpc_io* io, struct d_struct** output_pointer);
#endif

void drpc_call_free(struct drpc_call* call);
void drpc_return_free(struct drpc_return* ret);


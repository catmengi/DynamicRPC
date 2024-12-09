#pragma once

#include "drpc_types.h"

#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>


struct drpc_client{
    uint8_t aes128_key[16];
    int client_stop;
    int fd;
    pthread_t ping_thread;
    pthread_mutex_t connection_mutex;
};

struct drpc_client* drpc_client_connect(char* ip,uint16_t port, char* username, char* passwd);  //connect client to the server. Return NULL on connection error
void drpc_client_disconnect(struct drpc_client* client);  //disconnects and frees client struct

int drpc_client_call(struct drpc_client* client, char* fn_name, enum drpc_types* prototype, size_t prototype_len,void* native_return,...);
                                                            /*
                                                            * fn_name - name of client functions
                                                            * prototype - function prototype, used to parse variable arguments
                                                            * prototype_len - len of prototype
                                                            * native_return - pointer to chunk of memory where function return  will be placed
                                                            * ...    - callee function arguments, d_sizedbuf type should be passed as char*,size_t
                                                            */

int drpc_client_send_delayed(struct drpc_client* client, char* fn_name, struct d_struct* delayed_message);   //send delayed message to function "fn_name", return 0 on success

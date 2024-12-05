#pragma once

#include "drpc_types.h"

#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>


struct drpc_client{
    int client_stop;
    int fd;
    pthread_t ping_thread;
    pthread_mutex_t connection_mutex;
};

struct drpc_client* drpc_client_connect(char* ip,uint16_t port, char* username, char* passwd);
void drpc_client_disconnect(struct drpc_client* client);
int drpc_client_call(struct drpc_client* client, char* fn_name, enum drpc_types* prototype, size_t prototype_len,void* native_return,...);
int drpc_client_send_delayed(struct drpc_client* client, char* fn_name, struct d_struct* delayed_massage);

#pragma once

#include "drpc_protocol.h"
#include "drpc_types.h"

#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

enum drpc_client_errors{
    DRPC_OK,
    DRPC_ENETWORK,
    DRPC_CLIENTSTOPPED,
    DRPC_BADREPLY,
};

struct drpc_client{
    struct drpc_connection* io;
    int client_stop;
    pthread_t ping_thread;
    pthread_mutex_t connection_mutex;
};

struct drpc_client* drpc_client_connect(char* host, char* username, char* passwd);  //connect client to the server. char* host is a string in format "HOST:PORT" Return NULL on connection error

void drpc_client_disconnect(struct drpc_client* client);  //disconnects and frees client struct

int drpc_client_call(struct drpc_client* client, char* fn_name, enum drpc_types* prototype, size_t prototype_len,void* native_return,...);
                                                            /*
                                                            * fn_name - name of client functions
                                                            * prototype - function prototype, used to parse variable arguments
                                                            * prototype_len - len of prototype
                                                            * native_return - pointer to chunk of memory where function return  will be placed
                                                            * ...    - callee function arguments, d_sizedbuf type should be passed as char*,size_t
                                                            */

int drpc_client_send_delayed(struct drpc_client* client, char* fn_name, struct d_queue* messages);   //send delayed message to function "fn_name", return 0 on success
char* drpc_client_get_servername(struct drpc_client* client); // gets drpc_server's name; NON NULL on success

#ifdef DRPC_DQUEUE_IO
void* drpc_ping_server(void* clientP);
#endif

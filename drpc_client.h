#pragma once

#include "drpc_protocol.h"
#include "drpc_types.h"

#include <pthread.h>
#include <sys/types.h>
#include <stdarg.h>

#ifdef DRPC_TCP_SUPPORT
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

enum drpc_client_errors{
    DRPC_OK,
    DRPC_BAD,
    DRPC_ENETWORK,
    DRPC_CLIENTSTOPPED,
    DRPC_BADREPLY,
};

struct drpc_client{
    struct drpc_io* io;
    int client_stop;
    pthread_t ping_thread;
    pthread_mutex_t connection_mutex;

    void* userdata;
};

#ifdef DRPC_TCP_SUPPORT
struct drpc_client* drpc_client_connect(char* host, char* username, char* passwd);  //connect client to the server. char* host is a string in format "HOST:PORT" Return NULL on connection error
#endif

void drpc_client_disconnect(struct drpc_client* client);  //disconnects and frees client struct

//======================================================================================================================================================================================
int drpc_client_call(struct drpc_client* client, char* fn_name, enum drpc_types* prototype, size_t prototype_len,void* native_return,...);
                                                    //Calls a function from drpc server with provided arguments and handles pointer arguments sync and handle return values
                                                    //POINTERS TO (d_str,d_sizedbuf,d_array,d_struct,d_queue) ARE ALWAYS CHANGE AFTER CALL IN THOOSE TYPES: d_queue,d_array,d_struct

                                                    // fn_name - name of function to be called
                                                    // prototype - function prototype, used to parse variable arguments
                                                    // prototype_len - len of prototype
                                                    // native_return - pointer to chunk of memory where function return  will be placed
                                                    // ...    - callee function arguments, d_sizedbuf type should be passed as char*,size_t

                                                    // RETURN: 0 on success
//======================================================================================================================================================================================

char* drpc_client_get_servername(struct drpc_client* client);                                           // gets drpc_server's name; NON NULL on success

int drpc_client_mailbox_send(struct drpc_client* client, char* mailbox_name, struct d_queue* messages); //send queue of messages to server's mailbox with mailbox_name. RETURN: 0 on success

int drpc_client_mailbox_recv(struct drpc_client* client, char* mailbox_name, struct d_queue* output);  //get messages from server's mailbox with mailbox_name as name and pushes them to output queue;
                                                                                                       //RETURN: 0 on success

void drpc_client_set_userdata(struct drpc_client* client, void* userdata); //sets client->userdata to userdata;

#ifdef DRPC_DQUEUE_IO_SUPPORT
void* drpc_ping_server(void* clientP);
#endif

/*DRPC's internal API's that should not be used in user code*/
int drpc_client_call_internal(struct drpc_client* client, char* fn_name, enum drpc_types* prototype, size_t prototype_len,void* native_return,va_list varargs);
/*==========================================================*/

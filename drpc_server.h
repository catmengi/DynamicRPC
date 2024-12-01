#pragma once

#include <sys/types.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdatomic.h>

#include "drpc_types.h"
#include "drpc_que.h"
#include "hashtable.c/hashtable.h"

struct drpc_server{
    void* interfunc;

    hashtable_t users;
    hashtable_t functions;

    uint16_t port;

    pthread_t dispatcher;

    int server_fd;

    int should_stop;

    atomic_ullong client_ammount;
};

struct drpc_function{

    char* fn_name;

    size_t prototype_len;
    enum drpc_types* prototype;
    enum drpc_types return_type;

    void* personal;
    int minimal_permission_level;

    struct drpc_que* delayed_massage_que;

    void* fn;
    ffi_cif* cif;
    ffi_type** ffi_prototype;
};

struct drpc_connection{
    int fd;
    struct sockaddr_in client_addr;

    struct drpc_server* drpc_server;
};

struct drpc_type_update{
    enum drpc_types type;
    size_t len; //if availible
    void* ptr;
};

struct drpc_server* new_drpc_server(uint16_t port);  //creates drpc structure;

void drpc_server_start(struct drpc_server* server); //starts drpc server

void drpc_server_free(struct drpc_server* server);

void drpc_server_register_fn(struct drpc_server* server,char* fn_name, void* fn,
                             enum drpc_types return_type, enum drpc_types* prototype,
                             void* pstorage, size_t prototype_len, int perm);       //register new drpc function; pstorage - pointer for d_fn_pstorage type; perm is minimal permission level to
                                                                                    //call this function

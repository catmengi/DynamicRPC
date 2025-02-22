#pragma once

#include <sys/types.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdatomic.h>
#include <ffi.h>

#include "drpc_queue.h"
#include "drpc_types.h"
#include "drpc_que.h"
#include "hashtable.c/hashtable.h"

enum drpc_connection_event{
    drpc_connected,
    drpc_disconnected,
};

struct drpc_connection;
typedef void (*drpc_connection_event_cb)(struct drpc_connection* client,enum drpc_connection_event);

struct drpc_server{
    char* name; //if not set, drpc_servername would give "UNKNOWN_DRPC"
    void* interfunc;
    hashtable* users;
    hashtable* functions;
    uint16_t port;
    pthread_t dispatcher;
    int server_fd;
    int should_stop;
    atomic_ullong client_ammount;

    drpc_connection_event_cb connection_event_cb;
};

struct drpc_pstorage{
    struct d_queue* delayed_messages;
    void* pstorage;
};

struct drpc_function{
    char* fn_name;

    size_t prototype_len;
    enum drpc_types* prototype;
    enum drpc_types return_type;

    struct drpc_pstorage pstorage;
    int minimal_permission_level;

    void* fn;
    ffi_cif* cif;
    ffi_type** ffi_prototype;
};

struct drpc_connection{
    int fd;
    struct drpc_server* drpc_server;
    struct sockaddr_in client_addr;
    char* username;
    uint8_t aes128_key[16];
};

struct drpc_type_update{
    enum drpc_types type;
    size_t len; //if availible
    void* ptr;
};

struct drpc_user{
    int perm;
    uint64_t hash;

    uint8_t aes128_passwd[16];
};


struct drpc_server* new_drpc_server(uint16_t port);  //creates drpc structure;

void drpc_server_start(struct drpc_server* server); //starts drpc server

void drpc_server_free(struct drpc_server* server);

void drpc_server_register_fn(struct drpc_server* server,char* fn_name, void* fn,
                             enum drpc_types return_type, enum drpc_types* prototype,
                             size_t prototype_len, void* pstorage, int perm);       //register new drpc function; pstorage - pointer for d_fn_pstorage type; perm is minimal permission level to
                                                                                    //call this function
void drpc_server_add_user(struct drpc_server* serv, char* username,char* passwd, int perm);

struct d_queue* drpc_get_delayed_for(struct drpc_server* server, char* fn_name); //gets pstorage.delayed_messages of fn_name for local use

void drpc_server_set_servername(struct drpc_server* server, char* name); //copies name to drpc_server's name variable
char* drpc_server_get_servername(struct drpc_server* server); //gets drpc_server's name variable

void drpc_server_set_connection_event_cb(struct drpc_server* server, drpc_connection_event_cb drpc_connection_event_cb); //set drpc_connection_event_cb

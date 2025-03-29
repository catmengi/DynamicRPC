#pragma once

#include <sys/types.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdatomic.h>
#include <ffi.h>

#include "drpc_protocol.h"
#include "drpc_types.h"
#include "hashtable.c/hashtable.h"

#define DRPC_CLIENTID_LEN 32

enum drpc_connection_event{
    drpc_connected,
    drpc_disconnected,
    drpc_force_disconnected,
};

struct drpc_connection;
typedef void (*drpc_connection_event_cb)(struct drpc_connection* client,enum drpc_connection_event);

struct drpc_server{
    char* name; //if not set, drpc_servername would give "UNKNOWN_DRPC"
    void* interfunc;
    hashtable* users;
    hashtable* functions;
    struct d_struct* mailboxes;
    uint16_t port;
    pthread_t dispatcher;
    int server_fd;
    int should_stop;
    atomic_ullong client_ammount;

    drpc_connection_event_cb connection_event_cb;
};

struct drpc_pstorage{
    struct d_queue* client_messages;
    void* pstorage;
};

struct drpc_function{
    char* fn_name;

    size_t prototype_len;
    enum drpc_types* prototype;
    enum drpc_types return_type;

    void* fnstorage;
    int minimal_permission_level;

    void* fn;
    ffi_cif* cif;
    ffi_type** ffi_prototype;
};

struct drpc_connection{
    struct drpc_io* io;
    struct drpc_server* drpc_server;
    char* username;
    char clientid[DRPC_CLIENTID_LEN];
    void* userdata;

    int force_disconnect;
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

void drpc_server_free(struct drpc_server* server);  //stops and frees drpc server

void drpc_server_register_fn(struct drpc_server* server,char* fn_name, void* fn,
                             enum drpc_types return_type, enum drpc_types* prototype,
                             size_t prototype_len, void* fnstorage, int perm);
                            /*
                             * fn_name - name of function to be registered
                             * fn - function pointer
                             * return_type - return type of function from drpc_types.h
                             * prototype - function prototype made from types from drpc_types.h
                             * prototype_len - length of prototype
                             * fnstorage - pointer that will be used in d_fnstorage type
                             * perm - minimal permission to call this function. -1 - only -1 user can call this function
                            */
void drpc_server_add_user(struct drpc_server* serv, char* username,char* passwd, int perm);
                            /*
                             * username - username
                             * passwd - user's password
                             * perm - user's permission level
                            */

void drpc_server_set_servername(struct drpc_server* server, char* name); //copies name to drpc_server's name variable
char* drpc_server_get_servername(struct drpc_server* server); //gets drpc_server's name variable

void drpc_server_set_connection_event_cb(struct drpc_server* server, drpc_connection_event_cb drpc_connection_event_cb); //set drpc_connection_event_cb
void drpc_server_force_disconnect_client(struct drpc_connection* client); //disconnect client from server side

struct d_queue* new_drpc_mailbox(struct drpc_server* server, char* mailbox_name); //creates a new mailbox with a mailbox_name as name and returns it. Mailbox is struct d_queue
struct d_queue* drpc_get_mailbox(struct drpc_server* server, char* mailbox_name); //returns a mailbox with a mailbox_name as name. If it doesnt exist returns NULL
void drpc_free_mailbox(struct drpc_server* server, char* mailbox_name);          //frees and removes mailbox with mailbox_name as name and all it's data.

#ifdef DRPC_DQUEUE_IO
struct drpc_client* drpc_new_dqueue_client(struct drpc_server* server, int client_perm); //creates a new drpc_client but use a local d_queue instead of TCP socket
#endif

#pragma once
#include "tqueque/tqueque.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include "rpccall.h"
#include <ffi.h>
#include <arpa/inet.h>
#include "hashtable.c/hashtable.h"

typedef void (*rpcserver_client_cb)(void* userdata, struct sockaddr_in);  //callback type for NEW/DISCONNECT handling

struct rpcserver{
    int sfd;
    int stop;
    pthread_mutex_t edit;
    pthread_t accept_thread;
    hashtable_t fn_ht;
    hashtable_t users;
    atomic_ullong clientcount;
    void* interfunc;
    void* newclient_cb_data;
    rpcserver_client_cb newclient_cb;
    void* clientdiscon_cb_data;
    rpcserver_client_cb clientdiscon_cb;
};
struct fn{
    char* fn_name;
    void* fn; //libffi;
    uint8_t nargs;
    enum rpctypes rtype; //return type
    enum rpctypes* argtypes; //function arguments in rpctypes
    ffi_type** ffi_type_free;   //function arguments for libffi
    ffi_cif cif;                //prepared CIF
    void* personal;             //personal storage for PSTORAGE type
    int perm;                   //minimal permission for calling it
};
struct client_thread{
    int client_fd;
    struct rpcserver* serv;
    char client_uniq[16];
    struct sockaddr_in addr;
};


struct rpcserver* rpcserver_create(uint16_t port);   //creates server at specified PORT. returns new server;
void rpcserver_start(struct rpcserver* rpcserver);   //starts pre-created server
void rpcserver_free(struct rpcserver* serv);         //stops and frees server
void rpcserver_stop(struct rpcserver* serv);         //only stops but not frees server

int rpcserver_register_fn(struct rpcserver* serv, void* fn, char* fn_name,        //registers function in rpcserver
                          enum rpctypes rtype, enum rpctypes* argstype,           // serv: rpcserver,  fn: function pointer, fn_name: function name for client
                          uint8_t argsamm, void* pstorage,int perm);              // rtype: type that function will return,  argstype: function arguments types in rpctypes
                                                                                  // argsamm: ammount of function arguments, pstorage: personal storage for PSTORAGE type
                                                                                  // perm: minimal permission for calling this function, -1 - -1 client only can call this function

void rpcserver_unregister_fn(struct rpcserver* serv, char* fn_name);              // unregisters function from server

void rpcserver_load_keys(struct rpcserver* serv, char* filename);                 // loads permmission from "filename" file in format : keyPerm (somekey-1) or (abcd5522)

void rpcserver_add_key(struct rpcserver* serv, char* key, int perm);              // add key into server without file, "perm" is a permission of key

void rpcserver_register_newclient_cb(struct rpcserver* serv,rpcserver_client_cb callback, void* user);      //registers callback that will be called on every new client

void rpcserver_register_clientdiscon_cb(struct rpcserver* serv,rpcserver_client_cb callback, void* user);   //registers callback that will be called on every client disconnection

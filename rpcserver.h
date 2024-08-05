#include "tqueque/tqueque.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#ifndef RPCCALL_H
#define RPCCALL_H
#pragma  once
#include "rpccall.h"
#endif
#include <ffi.h>
#include "hashtable.c/hashtable.h"

struct rpcserver{
    int sfd;
    int stop;
    pthread_mutex_t edit;
    int is_incon;
    pthread_t accept_thread;
    pthread_t reliver;
    void** reliverargs;
    hashtable_t fn_ht;
    hashtable_t users;
    atomic_ullong clientcount;
    void* interfunc;
};
struct fn{
    char* fn_name;
    void* fn; //libffi;
    uint8_t nargs;
    enum rpctypes rtype;
    enum rpctypes* argtypes;
    ffi_type** ffi_type_free;
    ffi_cif cif;
    void* personal;
    int perm;
};
struct client_thread{
    int client_fd;
    struct rpcserver* serv;
};



struct rpcserver* rpcserver_create(uint16_t port);
void rpcserver_start(struct rpcserver* rpcserver);
void rpcserver_free(struct rpcserver* serv);

int rpcserver_register_fn(struct rpcserver* serv, void* fn, char* fn_name,
                          enum rpctypes rtype, enum rpctypes* argstype,
                          uint8_t argsamm, void* pstorage,int perm);
void rpcserver_unregister_fn(struct rpcserver* serv, char* fn_name);
void rpcserver_load_keys(struct rpcserver* serv, char* filename);

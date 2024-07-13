#include "tqueque/tqueque.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#ifndef RPCCALL_H
#define RPCCALL_H
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


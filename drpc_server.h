#pragma once

#include <sys/types.h>
#include <pthread.h>

#include "drpc_types.h"
#include "drpc_que.h"
#include "hashtable.c/hashtable.h"

struct drpc_server{
    void* interfunc;

    hashtable_t users;
    hashtable_t functions;

    uint16_t port;

    pthread_t dispatcher;
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
    struct drpc_server* drpc_server;
    //......
};

struct drpc_type_update{
    enum drpc_types type;
    size_t len; //if availible
    void* ptr;
};

//привет ворлд!!! Ну привет
#pragma once
#include <sys/types.h>
#include "drpc_types.h"

enum drpc_protocol{
    drpc_call,
    drpc_return,
    drpc_nofn,

    drpc_bad,

    drpc_auth,
    drpc_ok,

    drpc_disconnect,
};


struct drpc_call{
    char* fn_name;

    uint8_t arguments_len;
    struct drpc_type* arguments;
};

struct drpc_return{
    struct drpc_type returned;

    uint8_t updated_arguments_len;
    struct drpc_type* updated_arguments;

};

struct drpc_massage{
    enum drpc_protocol massage_type;
    struct d_struct* massage;
};


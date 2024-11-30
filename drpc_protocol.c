#include "drpc_protocol.h"
#include "drpc_struct.h"
#include "drpc_types.h"

#include <assert.h>
#include <stdlib.h>

struct d_struct* drpc_call_to_massage(struct drpc_call* call){
    struct d_struct* massage = new_d_struct();

    size_t arguments_buflen = drpc_types_buflen(call->arguments,call->arguments_len);
    char* arguments_buf = malloc(arguments_buflen); assert(arguments_buf);
    drpc_types_buf(call->arguments,call->arguments_len,arguments_buf);


    return massage;
}

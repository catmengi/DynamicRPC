#include "drpc_protocol.h"
#include "drpc_server.h"
#include "drpc_client.h"
#include "drpc_queue.h"
#include "drpc_struct.h"
#include "drpc_types.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct d_struct* d_struct_check(struct d_struct* check, uint64_t max_len, struct drpc_pstorage* pstorage){
    printf("que %p   ;;;   pstorage %p\n",pstorage->delayed_messages, pstorage->pstorage);

    char str[64];
    for(uint64_t i = 0; i <max_len; i++){
        sprintf(str,"%lu",i);

        uint64_t check_int = 0;
        assert(d_struct_get(check,str,&check_int,d_uint64) == 0);
        assert(check_int == i);
    }
    return check;

}
struct d_queue* d_queue_check(struct d_queue* check, uint64_t maxpop,struct drpc_pstorage* pstorage){
    printf("que %p   ;;;   pstorage %p\n",pstorage->delayed_messages, pstorage->pstorage);

    for(uint64_t i = 0; i <maxpop; i++){

        uint64_t check_int = 0;
        assert(d_queue_pop(check,&check_int,d_uint64) == 0);
        assert(check_int == i);
    }
    return check;
}

int main(void){
    struct drpc_server* server = new_drpc_server(2077);

    enum drpc_types dstruct_check[] = {d_struct,d_uint64, d_fn_pstorage};
    enum drpc_types dqueue_check[] = {d_queue,d_uint64, d_fn_pstorage};

    drpc_server_register_fn(server,"dstruct_check",d_struct_check,d_struct,dstruct_check,sizeof(dstruct_check) / sizeof(dstruct_check[0]),(void*)0x123,0);

    drpc_server_register_fn(server,"dqueue_check",d_queue_check,d_queue,dqueue_check,sizeof(dqueue_check) / sizeof(dqueue_check[0]),(void*)0x12F,0);

    drpc_server_add_user(server,"check_user","i have absurdly long password to check that this will surly work as expected!",1);

    drpc_server_start(server);


    struct drpc_client* client = drpc_client_connect("127.0.0.1",2077,"check_user","i have absurdly long password to check that this will surly work as expected!");

    struct d_struct* check1 = new_d_struct();
    struct d_queue* check2 = new_d_queue();

#define STRUCT_LEN 80000
#define QUEUE_LEN 80000

    char str[64];
    for(uint64_t i = 0; i < STRUCT_LEN; i++){
        sprintf(str,"%lu",i);

        d_struct_set(check1,str,&i,d_uint64);
    }

    for(uint64_t i = 0; i < QUEUE_LEN; i++){
        d_queue_push(check2,&i,d_uint64);
    }

    uint64_t check2_len = d_queue_len(check2);

    void* check1_ret = 0;

    drpc_client_call(client,"dstruct_check",dstruct_check,2,&check1_ret,check1,STRUCT_LEN);
    assert(check1 == check1_ret);

    void* check2_ret = 0;
    drpc_client_call(client,"dqueue_check",dqueue_check,2,&check2_ret,check2,QUEUE_LEN);
    assert(check2 == check2_ret);

    assert(check2_len != d_queue_len(check2));

    d_struct_free(check1);
    d_queue_free(check2);

    drpc_client_disconnect(client);
    drpc_server_free(server);

    sleep(1);

}

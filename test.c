#include "drpc_protocol.h"
#include "drpc_que.h"
#include "drpc_server.h"
#include "drpc_client.h"
#include "drpc_queue.h"
#include "drpc_struct.h"
#include "drpc_types.h"
#include "drpc_array.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STRUCT_LEN 10000
#define QUEUE_LEN 10000
#define ARRAY_LEN 10000
#define TEST_ITERATIONS 1024


void condiscon_cb(struct drpc_connection* connection,enum drpc_connection_event event){
    if(event == drpc_connected) printf("%s: connected\n",connection->username);
    else if(event == drpc_disconnected) printf("%s: disconnected\n",connection->username);
    else printf("%s: FORCE disconnected\n",connection->username);
}

struct d_struct* d_struct_check(struct d_struct* check, uint64_t max_len, struct drpc_pstorage* pstorage){
    printf("que %p   ;;;   pstorage %p\n",pstorage->client_messages, pstorage->pstorage);

    char str[64];
    for(uint64_t i = 0; i < max_len; i++){
        sprintf(str,"%lu",i);

        uint64_t check_int = 0;
        assert(d_struct_get_type(check,str) == d_uint64);
        assert(d_struct_get(check,str,&check_int,d_uint64) == 0);
        assert(d_struct_remove(check,str) == 0);
        assert(check_int == i);
    }
    return check;

}
struct d_queue* d_queue_check(struct d_queue* check, uint64_t maxpop,struct drpc_pstorage* pstorage){
    printf("que %p   ;;;   pstorage %p\n",pstorage->client_messages, pstorage->pstorage);

    for(uint64_t i = 0; i < maxpop; i++){

        uint64_t check_int = 0;
        assert(d_queue_get_type(check) == d_uint64);
        assert(d_queue_pop(check,&check_int,d_uint64) == 0);
        assert(check_int == i);
    }
    return check;
}

struct d_array* d_array_check(struct d_array* check,uint64_t max_len){
    for(uint64_t i = 0; i <max_len; i++){
        assert(d_array_get_type(check,i) == d_uint64 || d_array_get_type(check,i) == d_str);
        if(d_array_get_type(check,i) == d_str){
            char* out = NULL;
            d_array_get(check,i,&out,d_str);
            assert(strcmp(out,"     test") == 0);
        }
        d_array_remove(check,i);
    }
    return check;
}

void check_client(struct drpc_server* server, struct drpc_client* client){
    enum drpc_types dstruct_check[] = {d_struct,d_uint64, d_fn_pstorage};
    enum drpc_types dqueue_check[] = {d_queue,d_uint64, d_fn_pstorage};
    enum drpc_types darray_check[] = {d_array,d_uint64};
    struct d_struct* check1 = new_d_struct();
    struct d_queue* check2 = new_d_queue();
    // struct d_queue* delayed_check = new_d_queue();

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

    char* servername1 = drpc_client_get_servername(client);
    printf("BEFORE: %s\n",servername1);
    free(servername1);

    drpc_server_set_servername(server,"test server name");

    char* servername2 = drpc_client_get_servername(client);
    printf("AFTER: %s\n",servername2);
    free(servername2);

    drpc_server_set_servername(server,"memory leak check");

    char* servername3 = drpc_client_get_servername(client);
    printf("AFTER AGAIN: %s\n",servername3);
    free(servername3);
    clock_t struct_check_timeS = clock();
    drpc_client_call(client,"dstruct_check",dstruct_check,2,&check1_ret,check1,STRUCT_LEN);
    assert(check1 == check1_ret);
    clock_t struct_check_timeF = clock();
    printf("struct check time in ms %f\n", ((float)(struct_check_timeF - struct_check_timeS) / CLOCKS_PER_SEC) * 1000);

    clock_t que_check_timeS = clock();
    void* check2_ret = 0;
    drpc_client_call(client,"dqueue_check",dqueue_check,2,&check2_ret,check2,QUEUE_LEN);
    assert(check2 == check2_ret);
    clock_t que_check_timeF = clock();
    printf("que check time in ms %f\n", ((float)(que_check_timeF - que_check_timeS) / CLOCKS_PER_SEC) * 1000);

    assert(check2_len != d_queue_len(check2));

    struct d_array* darray = new_d_array(ARRAY_LEN+1);
    for(uint64_t i = 0; i < ARRAY_LEN; i++){
        if(i % 2 == 0)
            d_array_set(darray,i,&i,d_uint64);
        else
            d_array_set(darray,i,"     test",d_str);
    }

    clock_t arr_check_timeS = clock();
    void* array_ret = NULL;
    assert(drpc_client_call(client,"darray_check",darray_check,2,&array_ret,darray,ARRAY_LEN) == 0);
    assert(array_ret == darray);

    assert(darray->lookup_size != ARRAY_LEN);
    clock_t arr_check_timeF = clock();
    printf("arr check time in ms %f\n", ((float)(arr_check_timeF - arr_check_timeS) / CLOCKS_PER_SEC) * 1000);



    d_array_free(darray);
    d_struct_free(check1);
    d_queue_free(check2);
}


int main(void){
    struct drpc_server* server = new_drpc_server(2077);

    enum drpc_types dstruct_check[] = {d_struct,d_uint64, d_fn_pstorage};
    enum drpc_types dqueue_check[] = {d_queue,d_uint64, d_fn_pstorage};
    enum drpc_types darray_check[] = {d_array,d_uint64};

    drpc_server_register_fn(server,"dstruct_check",d_struct_check,d_struct,dstruct_check,sizeof(dstruct_check) / sizeof(dstruct_check[0]),(void*)0x123,0);

    drpc_server_register_fn(server,"dqueue_check",d_queue_check,d_queue,dqueue_check,sizeof(dqueue_check) / sizeof(dqueue_check[0]),(void*)0x12F,0);

    drpc_server_register_fn(server,"darray_check",d_array_check,d_array,darray_check,sizeof(darray_check) / sizeof(darray_check[0]),NULL,0);

    drpc_server_add_user(server,"check_user","i have absurdly long password to check that this will surly work as expected!",1);

    drpc_server_set_connection_event_cb(server,condiscon_cb);

    drpc_server_start(server);

    struct drpc_client* client = drpc_client_connect("localhost:2077","check_user","i have absurdly long password to check that this will surly work as expected!");
    struct drpc_client* dqueue_client = drpc_new_dqueue_client(server,-1);
    if(client == NULL){drpc_server_free(server); return 0;}

    struct d_queue* delayed_check = new_d_queue();
    for(uint16_t i = 0; i < 512; i++){
        d_queue_push(delayed_check,&i,d_uint16);
    }
    struct d_queue* delayed_que = drpc_get_message_queue_for(server,"dqueue_check");
    assert(d_queue_len(delayed_que) == 0);
    drpc_client_send_message(client,"dqueue_check",delayed_check);
    assert(d_queue_len(delayed_que) == 512);

    for(int i = 0; i < TEST_ITERATIONS;i++){
        printf("%d : iteration of test\n",i);
        check_client(server,dqueue_client);
    }

    for(int i = 0; i < TEST_ITERATIONS;i++){
        printf("%d : iteration of test\n",i);
        check_client(server,client);
    }

    drpc_client_disconnect(dqueue_client);
    drpc_client_disconnect(client);
    drpc_server_free(server);
}

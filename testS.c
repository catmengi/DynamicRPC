#include "drpc_server.h"
#include "drpc_client.h"
#include "drpc_queue.h"
#include "drpc_types.h"

#include <assert.h>
#include <stdio.h>

void test(struct d_queue* delayed_massages){
    struct d_struct* msg;
    if(d_queue_pop(delayed_massages,&msg,d_struct) != 0) {puts("you are fucked)))");return;}

    char* out;
    d_struct_get(msg,"322",&out,d_str);
    puts(out);
    d_struct_free(msg);
}

int main(void){
    struct drpc_server* server = new_drpc_server(2067);
    drpc_server_start(server);

    enum drpc_types prototype[] = {d_delayed_massage_queue};

    drpc_server_register_fn(server,"fn",test,d_void,prototype,sizeof(prototype) / sizeof(prototype[0]),NULL,0);
    drpc_server_add_user(server,"test_user","0",-1);


    getchar();

    drpc_server_free(server);
}

#include "drpc_server.h"
#include "drpc_client.h"
#include "drpc_queue.h"
#include "drpc_types.h"

#include <assert.h>
#include <stdio.h>

#include <unistd.h>

void input_receiver(struct drpc_pstorage* pstorage){
}

int main(void){

    enum drpc_types prototype[] = {d_fn_pstorage};

    struct drpc_server* server = new_drpc_server(2077);

    drpc_server_add_user(server,"game_client","",1);

    drpc_server_register_fn(server,"input_receiver",input_receiver,d_void,prototype,sizeof(prototype) / sizeof(prototype[0])
        ,NULL,0);

    drpc_server_start(server);

    struct d_queue* input_receiver_que = drpc_get_delayed_for(server,"input_receiver");
    assert(input_receiver_que != NULL);

    while(1){
        struct d_struct* massage = NULL;
        if(d_queue_pop(input_receiver_que,&massage,d_struct) != 0) continue;
        else{
            char input = 'f';
            assert(d_struct_get(massage,"input",&input,d_char) == 0);
            d_struct_free(massage);
            printf("%c\n",input);
        }
    }

}

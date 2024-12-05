#include "drpc_server.h"
#include "drpc_client.h"
#include "drpc_queue.h"
#include "drpc_struct.h"
#include "drpc_types.h"

#include <assert.h>
#include <stdio.h>



int main(void){

    struct d_struct* new = new_d_struct();

    for(size_t i = 0; i < 100000; i++){
        char str[64];
        sprintf(str,"%lu",i);

        d_struct_set(new,str,str,d_str);
    }

    size_t n = 0;
    struct d_struct* check = new_d_struct();
    buf_d_struct(d_struct_buf(new,&n),check);

    struct drpc_client* client = drpc_client_connect("127.0.0.1",2067,"test_user","0");
    struct d_struct* dstruct = new_d_struct();
    d_struct_set(dstruct,"322","NU I PIZDA",d_str);
    printf("\n%d\n",drpc_client_send_delayed(client,"fn",check));
    drpc_client_call(client,"fn",NULL,0,NULL);drpc_client_call(client,"fn",NULL,0,NULL);
    d_struct_free(dstruct);
    drpc_client_disconnect(client);
}

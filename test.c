#include "drpc_protocol.h"
#include "drpc_server.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
void test(){
    puts("I was called !!!!!!");
}
int main(){
    struct drpc_server* serv = new_drpc_server(2077);
    drpc_server_start(serv);

    drpc_server_register_fn(serv,"TEST",test,d_void,NULL,0,NULL,0);

    int client_fd = socket(AF_INET,SOCK_STREAM,0);

    struct sockaddr_in addr = {
        .sin_port = htons(2077),
        .sin_family = AF_INET,
    };
    inet_pton(AF_INET,"127.0.0.1",&addr.sin_addr);

    assert(connect(client_fd,(struct sockaddr*)&addr,sizeof(addr)) == 0);

    struct drpc_massage msg = {
        .massage = NULL,
        .massage_type = drpc_auth,
    };

    drpc_send_massage(&msg,client_fd);

    struct drpc_call call = {
        .fn_name = strdup("TEST"),
        .arguments = NULL,
        .arguments_len = 0,
    };

    msg.massage_type = drpc_call;
    msg.massage = drpc_call_to_massage(&call);

    drpc_send_massage(&msg,client_fd);

    getchar();
    drpc_server_free(serv);
}

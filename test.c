#include "drpc_protocol.h"
#include "drpc_server.h"
#include "drpc_types.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

char* test(char* str1, int32_t b){
    printf("%s, %d\n",str1,b);
    return strdup("ugrdsyutesruiteio");
}
int main(){
    struct drpc_server* serv = new_drpc_server(2077);
    drpc_server_start(serv);
    enum drpc_types prototype[] = {d_str, d_int32};
    drpc_server_register_fn(serv,"TEST",test,d_str,prototype,2,NULL,0);

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
        .arguments = calloc(2,sizeof(*call.arguments)),
        .arguments_len = 2,
    };

    str_to_drpc(&call.arguments[0],"здесь точно нет утечек память Agaagakaneshn");
    int32_to_drpc(&call.arguments[1],52521488);

    msg.massage_type = drpc_call;
    msg.massage = drpc_call_to_massage(&call);

    drpc_send_massage(&msg,client_fd);

    drpc_recv_massage(&msg,client_fd);

    assert(msg.massage_type == drpc_return);
    struct drpc_return* ret = massage_to_drpc_return(msg.massage);


    char* to_free = drpc_to_str(&ret->updated_arguments[0]);
    printf("%s : output of upd[0]\n",to_free); free(to_free);

    to_free = drpc_to_str(&ret->returned);
    printf("%s : output of return\n",to_free); free(to_free);

    drpc_return_free(ret);
    free(ret);

    getchar();
    drpc_server_free(serv);
}

#include "drpc_protocol.h"
#include "drpc_server.h"
#include <stdio.h>
#include <assert.h>
int main(){
    struct drpc_server* serv = new_drpc_server(2077);
    drpc_server_start(serv);

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

    getchar();
    drpc_server_free(serv);
}

#include "drpc_protocol.h"
#include "drpc_server.h"
#include "drpc_struct.h"
#include "drpc_types.h"
#include "drpc_client.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

char* test(char* str1, int32_t b, struct drpc_que* que){
    if(que == NULL) {puts("ПУСТАЯ ОЧЕРЕДЬ");return strdup("wnope");}
    struct drpc_delayed_massage * got = drpc_que_pop(que);

    char* print = NULL;
    d_struct_get(got->massage,"check",&print,d_str);
    puts(print);
    d_struct_free(got->massage);
    free(got->client);
    free(got);

    // printf("%s, %d\n",str1,b);
    return strdup("ugrdsyutesruiteio");
}
int main(){
    struct drpc_server* serv = new_drpc_server(2077);
    drpc_server_start(serv);
    enum drpc_types prototype[] = {d_str, d_int32,d_delayed_massage_queue};
    drpc_server_register_fn(serv,"TEST",test,d_str,prototype,3,NULL,0);
    drpc_server_add_user(serv,"abcd","abcd",-1);

    struct drpc_client* client = drpc_client_connect("127.0.0.1",2077,"abcd","abcd");
    drpc_client_disconnect(client);
    getchar();
    drpc_server_free(serv);
}

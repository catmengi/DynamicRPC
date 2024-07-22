#include "rpcserver.h"
#include <assert.h>
#include <stdio.h>
#include "string.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(){
    struct rpcserver* serv = rpcserver_create(2077);
    assert(serv);
    enum rpctypes fntype[] = {STR,INT32};
    enum rpctypes readtypes[] = {INT32,SIZEDBUF,UINT64};
    puts("crpc server by catmengi\n");
   // hashtable_add(serv->users,"hello da fuck",strlen("hello da fuck") + 1, perm,0);
    rpcserver_register_fn(serv,open,"open",INT32,fntype,sizeof(fntype) / sizeof(fntype[0]),NULL,999);
    rpcserver_register_fn(serv,read,"read",INT64,readtypes,sizeof(readtypes) / sizeof(readtypes[0]),NULL,999);
    rpcserver_load_keys(serv,"keys.txt");
    rpcserver_start(serv);
    printf("server started\n");
    sleep(2);
    getchar();
    rpcserver_free(serv);
}

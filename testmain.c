#include "rpcserver.h"
#include <assert.h>
#include <stdio.h>
#include "string.h"
#include <unistd.h>
struct rpcbuff* rpcbuff_test_fn(char* p,struct rpcbuff* buf,char* sz, uint64_t szl, uint64_t i, char* s){
    size_t index[1] = {0};
    printf("%s: %p %s %s %lu %lu %s\n",__PRETTY_FUNCTION__,buf,p,sz,szl,i,s);
    for(; index[0] < buf->dimsizes[0]; index[0]++){
        rpcbuff_pushto(buf,index,sizeof(index) / sizeof(index[0]),p,strlen(p) + 1);
        printf("%s:%s(%lu)\n",__PRETTY_FUNCTION__, rpcbuff_getlast_from(buf,index,sizeof(index) / sizeof(index[0]),NULL),index[0]);
    }
    return buf;
}
int main(){
    struct rpcserver* serv = rpcserver_create(2077);
    serv->interfunc = (void*)0x1488;
    assert(serv);
    enum rpctypes fntype[] = {PSTORAGE,RPCBUFF,SIZEDBUF,UINT64,UINT64,STR};
    puts("crpc server by catmengi\n");
   // hashtable_add(serv->users,"hello da fuck",strlen("hello da fuck") + 1, perm,0);
    rpcserver_register_fn(serv,rpcbuff_test_fn,"buft",RPCBUFF,fntype,sizeof(fntype) / sizeof(fntype[0]),strdup("fuza world"),999);
    rpcserver_load_keys(serv,"keys.txt");
    rpcserver_start(serv);
    printf("server started\n");
    sleep(2);
    getchar();
    rpcserver_free(serv);
}

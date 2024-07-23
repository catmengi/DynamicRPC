#include "rpcclient.h"
#include <stdlib.h>
#include "rpcmsg.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "rpcpack.h"
#include <stdio.h>
#include <string.h>

int rpc_open(char* pathname,int mode, struct rpccon* con){
    enum rpctypes types[] = {STR,INT32};
    char flag[sizeof(types)/sizeof(types[0])] = {0};
    flag[0] = 0;
    struct rpcret ret = {0};
    if(rpcclient_call(con,"open",types,flag,sizeof(types) / sizeof(types[0]),&ret,pathname,mode)) goto exit;
    int retfd = type_to_int32(&ret.ret);
    free(ret.ret.data);
    return retfd;
exit:
    free(ret.ret.data);
    return -1;
}
ssize_t rpc_read(int fd,char* buf,size_t len,struct rpccon* con){
    enum rpctypes types[] = {INT32,SIZEDBUF};
    char flag[sizeof(types)/sizeof(types[0])] = {0};
    flag[1] = 1;
    struct rpcret ret = {0};
    if(rpcclient_call(con,"read",types,flag,sizeof(types) / sizeof(types[0]),&ret,fd,buf,len)) goto exit;
    int64_t readlen = type_to_int64(&ret.ret);
    if(readlen < 0) return readlen;
    uint64_t lenr;
    char* obuf = unpack_sizedbuf_type(&ret.resargs[0],&lenr);
    if(!obuf) return -1;
    memcpy(buf,obuf,readlen);
    rpctypes_free(ret.resargs,ret.resargs_amm);
    free(ret.ret.data);
    return readlen;
exit:
    rpctypes_free(ret.resargs,ret.resargs_amm);
    free(ret.ret.data);
    return -1;
}
ssize_t rpc_write(int fd,char* buf,size_t len,struct rpccon* con){
    enum rpctypes types[] = {INT32,SIZEDBUF};
    struct rpcret ret = {0};
    char flag[2] = {0};
    if(rpcclient_call(con,"write",types,flag,sizeof(types) / sizeof(types[0]),&ret,fd,buf,len)) goto exit;
    int64_t writelen = type_to_int64(&ret.ret);
    if(writelen < 0) return writelen;
    free(ret.ret.data);
    return writelen;
exit:
    free(ret.ret.data);
    return -1;
}
void rpc_close(int fd,struct rpccon* con){
    enum rpctypes types[] = {INT32};
    struct rpcret ret = {0};
    if(rpcclient_call(con,"close",types,NULL,sizeof(types) / sizeof(types[0]),&ret,fd)) return;
}
int main(){
   int perm;
   struct rpccon con;
   while(rpcserver_connect("localhost","hello da fuck",2077,&con)!=0){;};
   int fd = rpc_open("keys.txt",O_RDWR,&con);
   char buf[256] = {0};
   rpc_read(fd,buf,256,&con);
   printf("%s:buf\n",buf);
   struct rpcmsg goodbye = {DISCON,0,0,0};
   rpcmsg_write_to_fd(&goodbye,con.fd);
}

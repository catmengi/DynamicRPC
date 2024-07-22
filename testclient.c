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
    rpcclient_call(con,"open",types,flag,sizeof(types) / sizeof(types[0]),&ret,"keys.txt",O_RDWR);
    int retfd = type_to_int32(&ret.ret);
    free(ret.ret.data);
    return retfd;
}
ssize_t rpc_read(int fd,char* buf,size_t len,struct rpccon* con){
    enum rpctypes types[] = {INT32,SIZEDBUF};
    char flag[sizeof(types)/sizeof(types[0])] = {0};
    flag[1] = 1;
    struct rpcret ret = {0};
    rpcclient_call(con,"read",types,flag,sizeof(types) / sizeof(types[0]),&ret,fd,buf,len);
    int64_t retfd = type_to_int64(&ret.ret);
    uint64_t lenr;
    char* obuf = unpack_sizedbuf_type(&ret.resargs[0],&lenr);
    memcpy(buf,obuf,lenr);
    rpctypes_free(ret.resargs,ret.resargs_amm);
    free(ret.ret.data);
    return retfd;
}
int main(){
   int perm;
   struct rpccon con;
   rpcserver_connect("localhost","hello world!",2077,&con);
   int fd = rpc_open("keys.txt",O_RDWR,&con);
   char buf[256] = {0};
   rpc_read(fd,buf,256,&con);
   printf("%s:buf\n",buf);
   struct rpcmsg goodbye = {DISCON,0,0,0};
   rpcmsg_write_to_fd(&goodbye,con.fd);
}

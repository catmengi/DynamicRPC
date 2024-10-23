#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "lb_endian.h"
#include <stdlib.h>
#include "rpccall.h"
#include <assert.h>
#include <string.h>
#include <sys/socket.h>
int rpcmsg_write_to_fd(struct rpcmsg* msg, int fd){

    char header[sizeof(uint64_t) + sizeof(char)] = {0};
    header[0] = msg->msg_type;
    uint64_t be64_payload_len = cpu_to_be64(msg->payload_len);
    memcpy(&header[1],&be64_payload_len,sizeof(uint64_t));
    if(send(fd,header,9,MSG_NOSIGNAL) <= 0) return 1;
    uint64_t sent = 0;
    uint64_t bufsize = msg->payload_len;
    const char *pbuffer = (const char*) msg->payload;
    while (bufsize > 0)
    {
        int n = send(fd, pbuffer, bufsize, MSG_NOSIGNAL);
        sent += n;
        if (n <= 0 || errno != 0) return 1;
        pbuffer += n;
        bufsize -= n;
    }
    return 0;
}

int get_rpcmsg_from_fd(struct rpcmsg* msg ,int fd){
    memset(msg,0,sizeof(*msg));
    char header[sizeof(uint64_t) + sizeof(char)] = {0};
    if(recv(fd,header,9,MSG_NOSIGNAL) <= 0) return 1;
    msg->msg_type = header[0];
    uint64_t be64_payload_len = 0;
    memcpy(&be64_payload_len,&header[1],sizeof(uint64_t));
    msg->payload_len = be64_to_cpu(be64_payload_len);
    msg->payload = NULL;
    if(msg->payload_len > 0)msg->payload = calloc(msg->payload_len, sizeof(char));
    if(msg->payload == NULL && msg->payload_len > 0) return 1;
    size_t bufsize = msg->payload_len;
    char *pbuffer = (char*) msg->payload;
    while (bufsize > 0)
    {
        int n = recv(fd, pbuffer, bufsize, MSG_NOSIGNAL);
        if (n <= 0 || errno != 0) {free(msg->payload);return 1;}
        pbuffer += n;
        bufsize -= n;
    }
    return 0;
}

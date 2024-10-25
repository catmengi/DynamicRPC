#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include "lb_endian.h"
#include <stdlib.h>
#include "rpccall.h"
#include <assert.h>
#include <string.h>
#include <sys/socket.h>


int send_rpcmsg(struct rpcmsg* msg, int fd){
    char* buf = malloc(sizeof(char) + sizeof(uint64_t) + msg->payload_len);
    uint64_t tosend = sizeof(char);
    assert(buf);
    char* wr = buf;
    *wr = msg->msg_type; wr++;
    if(msg->msg_type != OK && msg->msg_type != BAD && msg->msg_type != PING && msg->msg_type != DISCON){
        tosend += sizeof(uint64_t) + msg->payload_len;
        uint64_t be64_len = cpu_to_be64(msg->payload_len);
        memcpy(wr,&be64_len, sizeof(uint64_t)); wr += sizeof(uint64_t);
        memcpy(wr,msg->payload,msg->payload_len);
    }
    free(msg->payload);
    uint64_t bufsize = tosend;
    char *pbuffer = buf;
    while (bufsize > 0)
    {
        int n = send(fd, pbuffer, bufsize, MSG_NOSIGNAL);
        if (n <= 0) {free(buf);return 1;}
        pbuffer += n;
        bufsize -= n;
    }
    free(buf);
    memset(msg,0,sizeof(*msg));
    return 0;
}


int get_rpcmsg(struct rpcmsg* msg ,int fd){
    memset(msg,0,sizeof(*msg));
    if(recv(fd,&msg->msg_type,sizeof(char),MSG_NOSIGNAL) <= 0) return 1;
    if(msg->msg_type != OK && msg->msg_type != BAD && msg->msg_type != PING && msg->msg_type != DISCON){
        uint64_t be64_len = 0;
        if(recv(fd,&be64_len,sizeof(uint64_t),MSG_NOSIGNAL) <= 0) return 1;
        msg->payload_len = be64_to_cpu(be64_len);
        msg->payload = malloc(msg->payload_len);
        assert(msg->payload);
        uint64_t bufsize = msg->payload_len;
        char *pbuffer = (char*) msg->payload;
        while (bufsize > 0)
        {
            int n = recv(fd, pbuffer, bufsize, MSG_NOSIGNAL);
            if (n <= 0) {free(msg->payload);return 1;}
            pbuffer += n;
            bufsize -= n;
        }
    }
    return 0;
}

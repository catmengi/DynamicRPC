#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <libubox/utils.h>
#include <stdlib.h>
#include "rpccall.h"
#include <assert.h>
#include <string.h>
#include <sys/socket.h>
ssize_t rpcmsg_write_to_fd(struct rpcmsg* msg, int fd){
    assert(msg);
    uint8_t crc = 0;
    for(uint64_t i = 0; i < msg->payload_len; i++){
        crc = crc ^ msg->payload[i] ^ i;
    }
    uint64_t be64_payload_len = cpu_to_be64(msg->payload_len);
    char type = msg->msg_type;
    ssize_t n = 0;
    ssize_t sent = 0;
    errno = 0;
    sent += n = send(fd, &type, sizeof(char),MSG_NOSIGNAL);
    if(errno != 0 || n <= 0) return -1;
    sent += n = send(fd, &be64_payload_len, sizeof(uint64_t),MSG_NOSIGNAL);
    if(errno != 0 || n <= 0) return -1;
    size_t bufsize = msg->payload_len;
    const char *pbuffer = (const char*) msg->payload;
    while (bufsize > 0)
    {
        int n = send(fd, pbuffer, bufsize, MSG_NOSIGNAL);
        sent += n;
        if (n <= 0 || errno != 0) return -1;
        pbuffer += n;
        bufsize -= n;
    }
    sent += send(fd, &crc, sizeof(uint8_t),MSG_NOSIGNAL);
    return sent;
}

int get_rpcmsg_from_fd(struct rpcmsg* msg ,int fd){
    if(fd < 0) return 1;
    memset(msg,0,sizeof(*msg));
    char type;
    recv(fd,&type,sizeof(char),MSG_NOSIGNAL);
    msg->msg_type = type;
    uint64_t be64_payload_len = 0;
    recv(fd, &be64_payload_len, sizeof(uint64_t),MSG_NOSIGNAL);
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
    uint8_t crc = 0;
    for(uint64_t i = 0; i < msg->payload_len; i++){
        crc = crc ^ msg->payload[i] ^ i;
    }
    uint8_t got_crc = 0;
    recv(fd, &got_crc, sizeof(uint8_t),MSG_NOSIGNAL);
    if(got_crc == crc) return 0;
    free(msg->payload); return 1;
}

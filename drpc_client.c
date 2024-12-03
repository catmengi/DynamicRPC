#include "drpc_client.h"
#include "drpc_protocol.h"
#include "drpc_types.h"
#include "drpc_struct.h"
#include "hashtable.c/hashtable.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/socket.h>
#include <string.h>

void* drpc_ping_server(void* clientP){
    struct drpc_client* client = clientP;
    client->client_stop = 0;
    assert(pthread_mutex_init(&client->connection_mutex,NULL) == 0);

    pthread_mutex_lock(&client->connection_mutex);
    while(client->client_stop == 0){
        struct drpc_massage send,recv;

        send.massage_type = drpc_ping;
        send.massage = NULL;

        recv.massage = NULL;
        recv.massage_type = drpc_bad;

        if(drpc_send_massage(&send,client->fd) != 0){
            client->client_stop = 1;
            break;
        }
        if(drpc_recv_massage(&recv,client->fd) != 0 || recv.massage_type != drpc_ping){
            client->client_stop = 1;
            break;
        }
        pthread_mutex_unlock(&client->connection_mutex);
        sleep(4);
        continue;
    }
    pthread_mutex_unlock(&client->connection_mutex);

    return NULL;
}

struct drpc_client* drpc_client_connect(char* ip,uint16_t port, char* username, char* passwd){
    assert(ip != NULL); assert(username != NULL); assert(passwd != NULL);
    struct sockaddr_in connect_to = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };
    assert(inet_aton(ip,&connect_to.sin_addr) != 0);
    int fd = socket(AF_INET,SOCK_STREAM,0);
    if(connect(fd,(struct sockaddr*)&connect_to,sizeof(connect_to)) != 0){
        close(fd);
        return NULL;
    }
    struct drpc_client* client = malloc(sizeof(*client)); assert(client);
    client->fd = fd;

    uint64_t passwd_hash = _hash_fnc(passwd,strlen(passwd));
    struct d_struct* auth = new_d_struct();


    d_struct_set(auth,"username",username,d_str);
    d_struct_set(auth,"passwd_hash",&passwd_hash,d_uint64);

    struct drpc_massage send = {
        .massage_type = drpc_auth,
        .massage = auth,
    };

    if(drpc_send_massage(&send,fd) != 0){
        d_struct_free(auth);
        free(client);
        close(fd);
        return NULL;
    }
    d_struct_free(auth);

    struct drpc_massage recv = {drpc_bad,NULL};
    if(drpc_recv_massage(&recv,fd) != 0){
        free(client);
        close(fd);
        return NULL;
    }
    if(recv.massage_type != drpc_ok){
        free(client);
        close(fd);
        return NULL;
    }
    assert(pthread_create(&client->ping_thread,NULL,drpc_ping_server,client) == 0);

    return client;
}

void drpc_client_disconnect(struct drpc_client* client){
    client->client_stop = 1;
    pthread_mutex_lock(&client->connection_mutex);

    struct drpc_massage send = {
        .massage_type = drpc_disconnect,
        .massage = NULL,
    };
    drpc_send_massage(&send,client->fd);
    close(client->fd);
    pthread_mutex_unlock(&client->connection_mutex);
    pthread_join(client->ping_thread,NULL);
    free(client);
}

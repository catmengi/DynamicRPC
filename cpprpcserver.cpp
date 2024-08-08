#include "rpcserver.h"
#include <iostream>
#include "rpcserver.hpp"

Drpcserver::~Drpcserver() {
    rpcserver_free(this->serv);
}

Drpcserver::Drpcserver(uint16_t port) {
    this->serv = rpcserver_create(port);
}

void Drpcserver::start() {
    rpcserver_start(this->serv);
}

void Drpcserver::stop() {
    rpcserver_stop(this->serv);
}

int Drpcserver::register_fn(void* fn, char* fn_name,
                    enum rpctypes rtype, enum rpctypes* argstype,
                    uint8_t argsamm, void* pstorage,int perm) {
    return rpcserver_register_fn(this->serv,fn,fn_name,rtype,argstype,argsamm,pstorage,perm);
}
void Drpcserver::unregister_fn(char* fn_name) {
    rpcserver_unregister_fn(this->serv,fn_name);
}
void Drpcserver::load_password(char* filename) {
    rpcserver_load_keys(this->serv, filename);
}
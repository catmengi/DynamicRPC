#pragma once
#include <iostream>
class Drpcserver {
public:
    ~Drpcserver();
    Drpcserver(uint16_t port);
    int register_fn(void* fn, char* fn_name,
                    enum rpctypes rtype, enum rpctypes* argstype,
                    uint8_t argsamm, void* pstorage,int perm);//register specified function
    void unregister_fn(char* fn_name);//unregister specified function
    void load_password(char* filename);//load password with permission from file
    void start();//start server
    void stop();//stop
private:
    struct rpcserver* serv;
};
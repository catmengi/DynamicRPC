#DynamicRPC

**DynamicRPC** is a pure-c rpc client-server framework, designed to be expandable and doesnt require IDL. Made with libffi to call functions by pointers without know their prototypes at runtime

------------


#Build

 If you want server-client build you could use something like this
 
 `gcc -c rpc*.c hashtable.c/hashtable.c tqueque/tqueque.c `
 
 and then link they with your .o files and libffi and pthread
 
** OR**
 
 `gcc -o some_project rpc*.c hashtable.c/hashtable.c tqueque.c/tqueque.c -lffi -pthread`
 
------------


#API

**Type system**: 
All type-system stuff are in [rpctypes.h](http://github.com/catmengi/DynamicRPC/blob/master/rpctypes.h "rpctypes.h")
Most of the types are binding for native C types: **CHAR, STR, UINT16..64, INT16..64, VOID**

Other types are combination of C types or custom types designed to handle structures or 
multi-dimensional arrays like: **SIZEDBUF, RPCBUFF, RPCSTRUCT**

Also there are some special types like **PSTORAGE, INTERFUNC, FINGERPRINT**  (DOESNT exist for client side (not passed from client side))
for them refer to [rpctypes.h](http://github.com/catmengi/DynamicRPC/blob/master/rpctypes.h "rpctypes.h")

All this types are declared in **enum rpctypes**
Example of its usage:  `enum rpctypes proto[] = {UINT64, STR, RPCBUFF}`

**NOTE: SIZEDBUF is declared different for server and client, on server it SHOULD be declared with UINT64 in front of it, on client it SHOULD be standalone(without required UINT64), example:**

`enum rpctypes server[] = {SIZEDBUF,UINT64}`

`enum rpctypes client[] = {SIZEDBUF}`
 
 
examples of function prototype by server and client

client:   `enum rpctypes Pread[] = {INT32,SIZEDBUF};`

server:   `enum rpctypes Pread[] = {INT32,SIZEDBUF,UINT64};`


**Note №2: SIZEDBUF is not returnable!**
------------


**RPCBUFF API**

`struct rpcbuff* rpcbuff_create(uint64_t* dimsizes,uint64_t dimsizes_len)`

Creates multi-dimensional array with specified dimension sizes by **dimsizes** and ammount of dimension by **dimsizes_len**, returns ready to use rpcbuff

Arguments can be NULL for 1 element rpcbuff

------------


`void rpcbuff_free(struct rpcbuff* rpcbuff)`

Free rpcbuff

------------


`int rpcbuff_getfrom(struct  rpcbuff* rpcbuff, uint64_t* index, size_t index_len,void* raw,uint64_t* otype_len,enum rpctypes type)`

Get type that matches **type** at **index** with len of **index_len** and unpacks it to **raw** (**SHOULD** be pointer to pointer in case with **SIZEDBUF, STR, RPCBUFF, RPCSTRUCT**)
return 0 on success

------------


`int rpcbuff_pushto(struct rpcbuff* rpcbuff, uint64_t* index, size_t index_len, void* ptype,uint64_t type_len,enum rpctypes type)`

Packs and pushes **ptype**( pointer only) with **type** and **type_len** as len (if type is  **SIZEDBUF**)  to an **index** with len of **index len**
return 0 on success

**NOTE** : **RPCBUFF,RPCSTRUCT,STR,SIZEDBUF** are stored by pointers and their modifications will be applyed without rpcbuff_pushto

**NOTE №2** : **STR (also SIZEDBUF)** is copyed on rpcbuff_pushto but not packed, since this, it doesnt need to be freed after rpcbuff_getlast and modifications on pointer got by rpcbuff_getlast will be applyed immidiatly

------------


`void rpcbuff_remove(struct rpcbuff* rpcbuff, uint64_t* index, size_t index_len)`

Remove type at **index** with len of **index_len**

------------


`void rpcbuff_unlink(struct rpcbuff* rpcbuff, uint64_t* index, size_t index_len)`

Marks type at specified **index** with len of **index_len** to NOT be freed by `rpcbuff_free`



**RPCSTRUCT API**

  NOT DONE YET, please refer to [rpctypes.h](http://github.com/catmengi/DynamicRPC/blob/master/rpctypes.h "rpctypes.h")
  
  
  

------------

**Server API**

`struct rpcserver* rpcserver_create(uint16_t port)` 

creates new server at specified **port**; return struct rpcserver* at success

------------


`void rpcserver_start(struct rpcserver* rpcserver)`  starts server main thread

`void rpcserver_free(struct rpcserver* serv); `        stops and free server

`void rpcserver_stop(struct rpcserver* serv); `        stops but doesnt free

------------


`int rpcserver_register_fn(struct rpcserver* serv, void* fn, char* fn_name,
                          enum rpctypes rtype, enum rpctypes* fn_arguments,
                          uint8_t fn_arguments_len, void* pstorage,int perm)`
									   
This function registers function pointer **fn** with **fn_name**, return type specified by **rtype**,
prototype specified by **fn_arguments** and ammount of them specified by **fn_arguments_len**.
Also it sets pointer that will be used in **PSTORAGE** type (can be NULL), and minimal permission to call it by **perm**

`void rpcserver_add_key(struct rpcserver* serv, char* key, int perm);`
Adds a key with a permission **perm** into a server


------------

`void rpcserver_set_interfunc(struct rpcserver* self, void* interfunc);`
Set pointer for INTERFUNC type

------------

**there are also some APIs not covered here** refer to [rpcserver.h](https://github.com/catmengi/DynamicRPC/blob/master/rpcserver.h "rpcserver.h") for them

------------

**Client API**

`struct rpcclient* rpcclient_connect(char* host,int portno,char* key)`

connects to a server by address **host**, at port **portno** with **key** as password
return **struct rpcclient** pointer on success or NULL on failures




`int rpcclient_call(struct rpcclient* self,char* fn,enum rpctypes* fn_prototype,char* flags, uint8_t fn_prototype_len,void* fnret,...)`

Calls a function with name **fn**, with prototype **fn_prototype**, flags **flags**, len of prototype and flags is **fn_prototype_len**, **fnret** is a pointer to a memory where function result will be placed. Function arguments will be in **variable arguments** of a rpcclient_call

Flags stand for should this argument will be resened to a client, it supports folowing type:
**SIZEDBUF,STR,RPCBUFF,RPCSTRUCT**. If flags is NULL none of arguments will be resended.

Arguments are passed like in printf, but with folowing exceptions
1. **SIZEDBUF** is passed as char*,uint64_t


**NOTE** : currently return will be ALWAYS SEPARATED and in DIFFERENT chunk of memory than ANY arguments (even if on server side function returns passed argument with pointer type) because current rpc protocol doesnt know is return are one of arguments




`void rpcclient_disconnect(struct rpcclient* self)` Disconnects from server




`char* rpcclient_get_fingerprint(struct rpcclient* self)`
Get unique string of client, same as FINGERPRINT type on server


**there are also some APIs not covered here** refer to [rpcclient.h](https://github.com/catmengi/DynamicRPC/blob/master/rpcclient.h "rpcclient.h") for them






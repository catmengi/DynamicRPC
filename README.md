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

Also there are some special types like **PSTORAGE, INTERFUNC, UNIQSTR**
for them refer to [rpctypes.h](http://github.com/catmengi/DynamicRPC/blob/master/rpctypes.h "rpctypes.h")

All this types are declared in **enum rpctypes**
Example of its usage:  `enum rpctypes proto[] = {UINT64, STR, RPCBUFF}`

**NOTE: SIZEDBUF is declared different for server and client, on server in SHOULD be declared with UINT64 in front of it, on client it SHOULD be standalone(without UINT64), example:**

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

------------


`void rpcbuff_free(struct rpcbuff* rpcbuff)`
Free rpcbuff

------------


`int rpcbuff_getlast_from(struct  rpcbuff* rpcbuff, uint64_t* index, size_t index_len,void* raw,uint64_t* otype_len,enum rpctypes type)`
Get type that matches **type** at **index** with len of **index_len** and unpacks it to **raw** (**SHOULD** be pointer to pointer in case with **SIZEDBUF, STR, RPCBUFF, RPCSTRUCT**)
return 0 on sucess, otherwise type at this index cannot be found

------------


`int rpcbuff_pushto(struct rpcbuff* rpcbuff, uint64_t* index, size_t index_len, void* ptype,uint64_t type_len,enum rpctypes type)`
Packs and pushes **ptype**( pointer`type*` only) with **type** and **type_len** as len (if type is  **SIZEDBUF**)  to an **index** with len of **index len**
return 0 on sucess

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
`struct rpcserver* rpcserver_create(uint16_t port)` creates new server at specified **port** 
return **struct rpcserver* ** at success

------------


`void rpcserver_start(struct rpcserver* rpcserver)`  starts server main thread
`void rpcserver_free(struct rpcserver* serv); `        stops and free server
`void rpcserver_stop(struct rpcserver* serv); `        stops but doesnt free

------------


`int rpcserver_register_fn(struct rpcserver* serv, void* fn, char* fn_name,
                                       enum rpctypes rtype, enum rpctypes* argstype,
                                       uint8_t argsamm, void* pstorage,int perm)`
									   
This function registers function pointer **fn** with **fn_name**, return type specified by **rtype**,
arguments specified by **argstype** and ammount of them specified by **argsamm**.
Also it sets pointer that will be used in **PSTORAGE** type (can be NULL), and minimal permission to call it by **perm**

`void rpcserver_add_key(struct rpcserver* serv, char* key, int perm);`
Adds a key with a permission by **perm** into a server

**there are also some APIs not covered here** refer to [rpcserver.h](https://github.com/catmengi/DynamicRPC/blob/master/rpcserver.h "rpcserver.h") for them

------------

**Client API**
`struct rpcclient* rpcclient_connect(char* host,int portno,char* key)`
connects to a server by address **host**, at port **portno** with **key** as password
return **struct rpcclient** pointer on success or NULL on failures

------------

`int rpcclient_call(struct rpcclient* self,char* fn,enum rpctypes* rpctypes,char* flags, uint8_t rpctypes_len,void* fnret,...)`
Calls a function with name **fn**, with prototype **rpctypes**, flags **flags**, len of prototype and flags if **rpctypes_len**, **fnret** is a pointer to a memory where function result will be placed. Function arguments will be in **variable arguments** of a rpcclient_call

Flags stand for should this argument will be resened to a client, it supports folowing type:
**SIZEDBUF,STR,RPCBUFF,RPCSTRUCT**. If flags is NULL none of arguments will be resended.

Arguments are passed like in printf, but with folowing exceptions
1. **SIZEDBUF** is passed as char*,uint64_t
1. **RPCBUFF,RPCSTRUCT,STR** is passed by pointers of their types


------------



`void rpcclient_disconnect(struct rpcclient* self)` Disconnects from server

------------


**there are also some APIs not covered here** refer to [rpcclient.h](https://github.com/catmengi/DynamicRPC/blob/master/rpcclient.h "rpcclient.h") for them






DynamicRPC is a RPC framework that dont uses IDL and support argument-sync beetwen client a server


#API

**Type system**

All types is declared in `enum drpc_types` from drpc_types.h

Basicly there are 3 types of types:
   1.Simple types
   2.Pointer(resendable) types
   3.Server-only types
   
   
   **Simple types**:
-    d_char       //same as C char and d_int8
-    d_int8       //same as C int8_t
-    d_uint8      //same as C uint8_t
-    d_int16      //same as C int16_t
-    d_uint16    //same as C uint16_t
-    d_int32      //same as C int32_t
-    d_uint32    //same as C uint32_t
-    d_int64,    //same as C int64_t
-    d_uint64   //same as C uint64_t
-    d_float      //same as C float
-    d_double  //same as C double

**Pointer types**:
This types will be always synced beetwen client and server, if server change argument of this type it will be changed on client too
   -  d_str               //C char*
   -  d_sizedbuf     //this type will be passed to function as char*,size_t
   -  d_struct         //struct d_struct*, it have it's own API
   -  d_queue       //struct d_queue*, it have it's own API
   
   
  **Server-only types**:
  This types exist only in server-side function prototype and not sended by client
   -  d_fn_pstorage //provide struct drpc_pstorage*
   -  d_clientinfo     //provide struct drpc_connection*
   -  d_interfunc     //provide your register void* pointer
   
   

------------


  **drpc_server API**
  All of this API's are in "drpc_server.h"
  
  
  `struct drpc_server* new_drpc_server(uint16_t port)`  allocate a new drpc_server at port
  `void drpc_server_start(struct drpc_server* server)`    starts an allocated drpc_server
  `void drpc_server_free(struct drpc_server* server)`     stops and frees drpc_server
  

------------


  `void drpc_server_register_fn(struct drpc_server* server,char* fn_name, void* fn,
                             enum drpc_types return_type, enum drpc_types* prototype,
                             size_t prototype_len, void* pstorage, int perm)`
							 
**fn_name** -- name of function for client-side
**fn** -- function pointer
**prototype** -- C function prototype in `enum drpc_types` format
**pstorage** --  `struct drpc_pstorage.pstorage ` will be set to this arguments
**perm** -- minimal permission level of client to call this function or send delayed message to it. If it -1 then client's permission level should be also -1

------------

`void drpc_server_add_user(struct drpc_server* serv, char* username,char* passwd, int perm)`

**username** -- name of user that will be used for client auth
**passwd**     -- password that will be used for client auth and be part of AES128 key
**perm**         -- client's permission level

------------
`struct drpc_pstorage{
    struct d_queue* delayed_messages;
    void* pstorage;
};`
delayed_messages is the queue of client's delayed messages for this function.
All elements in queue are in d_struct type

------------



**Client API**:
all of this API's are in "drpc_client.h"

`struct drpc_client* drpc_client_connect(char* ip,uint16_t port, char* username, char* passwd)`

connect to the drpc_server

**ip** -- server's IP
**port** -- server's port
**username** -- auth username
**passwd** -- auth password

RETURN: non-NULL struct drpc_client pointer on success

------------

`void drpc_client_disconnect(struct drpc_client* client)` disconnect from server and frees client struct


------------

`int drpc_client_call(struct drpc_client* client, char* fn_name, enum drpc_types* prototype, size_t   prototype_len,void* native_return,...)`

call function on drpc_server

** fn_name** - name of client functions
 **prototype** - function prototype, used to parse variable arguments
 **prototype_len** - len of prototype
 **native_return** - pointer to chunk of memory where function return  will be placed
** ...**    - callee function arguments, d_sizedbuf type should be passed as char*,size_t

RETURN: 0 - success, otherwise error

------------

`int drpc_client_send_delayed(struct drpc_client* client, char* fn_name, struct d_struct* delayed_message)`

send delayed_message to the function on drpc_server

**fn_name** -- function who receive this message
**delayed_message** -- message to send

RETURN: 0 - success, otherwise error


------------

**d_queue and d_struct API**

thoose are declared in [drpc_queue.h](https://github.com/catmengi/DynamicRPC/blob/v2/drpc_queue.h "drpc_queue.h") and [drpc_struct.h](https://github.com/catmengi/DynamicRPC/blob/v2/drpc_struct.h "drpc_struct.h")

API's are documented there





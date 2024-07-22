
README.md IS IN WIP STATE


crpc is a rpc client/server p2p library. It's main feature is resend some type of args(sizedbuf,rpcbuff)

SIZEDBUF is a 1d array data type, it pushes as char*,uint64_t in function in server:
  
    server definition in func: (SIZEDBUF,UINT64)
    client definition: (SIZEDBUF)
    support resending with flag: 1

RPCBUFF is a array type for arrays that dimensions number, and sizes are unknown on compilation time. This is a struct
and have it's own function to interact with it
  
    support resending with flag: 1

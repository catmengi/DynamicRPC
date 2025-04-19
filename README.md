# I dont want to continue this project anymore because it have no architecture and become very hard to fix
# I will start new RPC project sometime soon when i will have enough time

TCP IO is VERY buggy right now, it have A LOT OF MEMORY LEAKS. I think i should rewrite TCP in future to fix this

DynamicRPC is a RPC framework that dont uses IDL and support argument-sync beetwen client a server

**most APIs are documentated in drpc_*.h headers**

**dependencies:** libffi, pthread


**supported platforms:** POSIX with support of libffi and pthread

**tested on:** Linux (x86_64, aarch64), ESP32S3 n16r8

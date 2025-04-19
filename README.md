v2 branch is DEAD. V3 version is in development

TCP IO is VERY buggy right now, it have A LOT OF MEMORY LEAKS. I think i should rewrite TCP in future to fix this

DynamicRPC is a RPC framework that dont uses IDL and support argument-sync beetwen client a server

**most APIs are documentated in drpc_*.h headers**

**dependencies:** libffi, pthread


**supported platforms:** POSIX with support of libffi and pthread

**tested on:** Linux (x86_64, aarch64), ESP32S3 n16r8

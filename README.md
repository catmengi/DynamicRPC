DynamicRPC is a RPC framework that dont uses IDL and support argument-sync beetwen client a server

**most APIs are documentated in drpc_*.h headers**

**dependencies:** libffi, pthread


**supported platforms:** POSIX with support of libffi and pthread

**tested on:** Linux (x86_64, aarch64), ESP32S3 n16r8


Building without TCP should be supported but not tested on platform without unix socket headers

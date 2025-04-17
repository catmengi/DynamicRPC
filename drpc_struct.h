#pragma once

#include "drpc_types.h"
#include "hashtable.c/hashtable.h"


struct d_struct{
    hashtable* hashtable;
    size_t current_len;   //ammount of elements in d_struct currently

    queue_t heap_keys;
};

struct d_struct_element{
    void* data; //may be packed type or may be a pointer(d_sizedbuf, d_struct, d_array, d_struct); d_sizedbuf and d_str are copyed on setting
    char is_packed; //used for simpler detection of pointer types
    char type;

    size_t sizedbuf_len;
};


struct d_struct* new_d_struct();

//======================================================================================================================================================================================
void d_struct_set(struct d_struct* dstruct,char* key, void* native_type, enum drpc_types type,...);
                                                    //serializes native_type and store it. If pointer type was set (d_str,d_sizedbuf,d_array,d_struct,d_queue)
                                                    //it CAN be changed without re-setting it because it is stored as pointer until _buf serialization happens.
                                                    // POINTER TYPES WILL BE FREED WHEN STRUCTURE IS FREED IF IT WASNT UNLINKED(OR IN CASE of d_queue POPPED)

                                                    // key         -- name of field to set
                                                    // native_type -- pointer to native C type that will be pushed. If it is a NON pointer type you should pass this: &input else just pass: input
                                                    // type        -- type of native_type, used to serilialize-deserialize
                                                    // ...         -- used with d_sizedbuf type, you should provide len of d_sizedbuf here

                                                    // RETURN: 0 on success*
//======================================================================================================================================================================================

//======================================================================================================================================================================================
int d_struct_get(struct d_struct* dstruct,char* key, void* native_type, enum drpc_types type,...);
                                                    //get element the unserialize it and store into native_type. In case of pointer types just store retrieved pointer into native_type

                                                    // key         -- name of field to get
                                                    // native_type -- pointer to memory where this type will be written. Generaly you should pass something like this to it: &output
                                                    // type        -- type of native_type, used to serilialize-deserialize them
                                                    // ...         -- used with d_sizedbuf type, you should provide pointer to size_t, it will store d_sizedbuf length there

                                                    // RETURN: 0 on success
//======================================================================================================================================================================================
int d_struct_remove(struct d_struct* dstruct, char* key);               //remove element with name key and free it's data RETURN: 0 on success
int d_struct_unlink(struct d_struct* dstruct, char* key);               //remove element with name key but DOESNT free it's data. RETURN: 0 on success
void d_struct_free(struct d_struct* dstruct);                           //free d_struct and all it's data
char** d_struct_get_fields(struct d_struct* dstruct, size_t* len);      //returns an array of string with d_struct elements' keys. length of this array will be placed into len
enum drpc_types d_struct_get_type(struct d_struct* dstruct, char* key); //returns a type of element with name key. RETURN: d_void on error

struct d_struct* d_struct_copy(struct d_struct* dstruct);               //return copy of dstruct




/*DRPC's internal API's that should not be used in user code*/
void d_struct_free_internal(struct d_struct* dstruct);
char* d_struct_buf(struct d_struct* dstruct, size_t* buflen);
void buf_d_struct(char* buf, struct d_struct* dstruct);
/*==========================================================*/

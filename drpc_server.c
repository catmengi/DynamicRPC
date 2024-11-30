#include "drpc_struct.h"
#include "drpc_types.h"
#include "drpc_protocol.h"
#include "drpc_server.h"
#include "drpc_que.h"


#include <assert.h>
#include <string.h>
#include <ffi.h>
#include <stdlib.h>

#include <stdio.h>
#include <sys/types.h>

struct drpc_server* new_drpc_server(uint16_t port){
    struct drpc_server* drpc_serv = calloc(1,sizeof(*drpc_serv)); assert(drpc_serv);

    hashtable_create(&drpc_serv->functions,8,2);
    hashtable_create(&drpc_serv->users,8,2);

    drpc_serv->port = port;

    return drpc_serv;
}

void drpc_server_register_fn(struct drpc_server* server,char* fn_name, void* fn,
                             enum drpc_types return_type, enum drpc_types* prototype,
                             void* pstorage, size_t prototype_len, int perm){
    struct drpc_function* fn_info = calloc(1,sizeof(*fn_info)); assert(fn_info);

    fn_info->delayed_massage_que = drpc_que_create();
    fn_info->fn_name = strdup(fn_name);
    fn_info->fn = fn;
    fn_info->minimal_permission_level = perm;
    fn_info->return_type = return_type;
    fn_info->personal = pstorage;
    fn_info->prototype_len = prototype_len;
    fn_info->prototype = calloc(fn_info->prototype_len, sizeof(enum drpc_types));
    memcpy(fn_info->prototype,prototype, sizeof(enum drpc_types) * prototype_len);

    assert(hashtable_add(server->functions,fn_name, strlen(fn_name) + 1,fn_info,0) == 0);

}

enum drpc_types* drpc_types_extract_prototype(struct drpc_type* drpc_types,size_t drpc_types_len){
    if(drpc_types == NULL) return NULL;
    enum drpc_types* ret = malloc(drpc_types_len * sizeof(enum drpc_types));
    for(uint64_t i = 0; i < drpc_types_len; i++){
        ret[i] = drpc_types[i].type;
    }
    return ret;
}

int is_arguments_equal_prototype(enum drpc_types* serv, size_t servlen, enum drpc_types* client, size_t clientlen){
    if(!serv && !client) return 0;
    struct drpc_que* check_que = drpc_que_create();
    assert(check_que);
    size_t newservlen = 0;
    for(size_t i = 0; i < servlen;i++){
        if(serv[i] != d_interfunc && serv[i] != d_fn_pstorage && serv[i] != d_clientinfo){
            drpc_que_push(check_que,&serv[i]);
            newservlen++;
        }
    }
    if((serv && !client) || (!serv && client)) {
        if(clientlen == 0 && newservlen == 0){
            drpc_que_free(check_que);
            return 0;
        }
        drpc_que_free(check_que);
        return 1;
    }
    if(newservlen != clientlen) {drpc_que_free(check_que);return 1;}
    enum drpc_types* newserv = calloc(newservlen,sizeof(enum drpc_types));
    assert(newserv);
    for(size_t i = 0; i < newservlen; i++){
        newserv[i] = *(enum drpc_types*)drpc_que_pop(check_que);
    }
    int ret = 0;
    for(size_t i = 0; i < clientlen; i++)
        if(newserv[i] != client[i]) {ret = 1;break;}
    drpc_que_free(check_que);
    free(newserv);
    return ret;
}


void** ffi_from_drpc(struct drpc_type* arguments,enum drpc_types* prototype,size_t prototype_len,size_t* ffi_len,struct drpc_que* to_repack, struct drpc_que* fill_later){
    size_t adjusted_len = drpc_proto_to_ffi_len_adjust(prototype,prototype_len);
    void** ffi_arguments = calloc(adjusted_len, sizeof(void*)); assert(ffi_arguments);

    *ffi_len = adjusted_len;
    size_t j = 0; size_t k = 0;
    for(size_t i = 0; i < prototype_len; i++){
        if(prototype[i] == d_fn_pstorage){
            ffi_arguments[k] = calloc(1,sizeof(void*));
            assert(ffi_arguments[k]);
            struct drpc_type_update* fill_later_info = calloc(1,sizeof(*fill_later_info)); assert(fill_later_info);
            fill_later_info->type = d_fn_pstorage;
            fill_later_info->ptr = &ffi_arguments[k];
            drpc_que_push(fill_later,fill_later_info);
            k++;
            continue;
        }
        if(prototype[i] == d_clientinfo){
            ffi_arguments[k] = calloc(1,sizeof(void*));
            assert(ffi_arguments[k]);
            struct drpc_type_update* fill_later_info = calloc(1,sizeof(*fill_later_info)); assert(fill_later_info);
            fill_later_info->type = d_clientinfo;
            fill_later_info->ptr = &ffi_arguments[k];
            drpc_que_push(fill_later,fill_later_info);
            k++;
            continue;
        }
        if(prototype[i] == d_interfunc){
            ffi_arguments[k] = calloc(1,sizeof(void*));
            assert(ffi_arguments[k]);
            struct drpc_type_update* fill_later_info = calloc(1,sizeof(*fill_later_info)); assert(fill_later_info);
            fill_later_info->type = d_interfunc;
            fill_later_info->ptr = &ffi_arguments[k];
            drpc_que_push(fill_later,fill_later_info);
            k++;
            continue;
        }
        if(prototype[i] == d_delayed_massage_queue){
            ffi_arguments[k] = calloc(1,sizeof(void*));
            assert(ffi_arguments[k]);
            struct drpc_type_update* fill_later_info = calloc(1,sizeof(*fill_later_info)); assert(fill_later_info);
            fill_later_info->type = d_delayed_massage_queue;
            fill_later_info->ptr = &ffi_arguments[k];
            drpc_que_push(fill_later,fill_later_info);
            k++;
            continue;
        }
            if(arguments[j].type == d_array){
                ffi_arguments[k] = calloc(1,sizeof(void*));
                assert(ffi_arguments[k]);
                *(void**)ffi_arguments[k] = drpc_to_d_array(&arguments[j]);

                struct drpc_type_update* update = calloc(1,sizeof(*update));
                update->type = d_array;
                update->ptr = *(void**)ffi_arguments[k];
                drpc_que_push(to_repack,update);

                j++; k++;
                continue;
            }
            if(arguments[j].type == d_struct){
                ffi_arguments[k] = calloc(1,sizeof(void*));
                assert(ffi_arguments[k]);
                *(void**)ffi_arguments[k] = drpc_to_d_struct(&arguments[j]);

                struct drpc_type_update* update = calloc(1,sizeof(*update));
                update->type = d_struct;
                update->ptr = *(void**)ffi_arguments[k];
                drpc_que_push(to_repack,update);

                j++;k++;
                continue;
            }
            if(arguments[j].type == d_str){
                ffi_arguments[k] = calloc(1,sizeof(void*));
                assert(ffi_arguments[k]);
                *(void**)ffi_arguments[k] = drpc_to_str(&arguments[j]);

                struct drpc_type_update* update = calloc(1,sizeof(*update));
                update->type = d_str;
                update->ptr = *(void**)ffi_arguments[k];
                drpc_que_push(to_repack,update);

                j++;k++;
                continue;
            }
            if(arguments[j].type == d_sizedbuf){
                ffi_arguments[k] = calloc(1,sizeof(void*));
                assert(ffi_arguments[k]);

                size_t sizedbuf_len = 0;
                *(void**)ffi_arguments[k] = drpc_to_sizedbuf(&arguments[j],&sizedbuf_len);

                struct drpc_type_update* update = calloc(1,sizeof(*update));
                update->type = d_sizedbuf;
                update->ptr = *(void**)ffi_arguments[k];
                update->len = sizedbuf_len;
                k++;
                ffi_arguments[k] = calloc(1,sizeof(size_t));
                assert(ffi_arguments[k]);
                *(size_t*)ffi_arguments[k] = sizedbuf_len;

                drpc_que_push(to_repack,update);

                j++;k++;
                continue;
            }
            if(arguments[j].type == d_int8){
                ffi_arguments[k] = calloc(1,sizeof(int8_t*));
                assert(ffi_arguments[k]);
                *(int8_t*)ffi_arguments[k] = drpc_to_int8(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_uint8){
                ffi_arguments[k] = calloc(1,sizeof(uint8_t*));
                assert(ffi_arguments[k]);
                *(uint8_t*)ffi_arguments[k] = drpc_to_uint8(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_int16){
                ffi_arguments[k] = calloc(1,sizeof(int16_t*));
                assert(ffi_arguments[k]);
                *(int16_t*)ffi_arguments[k] = drpc_to_int16(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_uint16){
                ffi_arguments[k] = calloc(1,sizeof(uint16_t*));
                assert(ffi_arguments[k]);
                *(uint16_t*)ffi_arguments[k] = drpc_to_uint16(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_int32){
                ffi_arguments[k] = calloc(1,sizeof(int32_t*));
                assert(ffi_arguments[k]);
                *(int32_t*)ffi_arguments[k] = drpc_to_int32(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_uint32){
                ffi_arguments[k] = calloc(1,sizeof(uint32_t*));
                assert(ffi_arguments[k]);
                *(uint32_t*)ffi_arguments[k] = drpc_to_uint32(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_int64){
                ffi_arguments[k] = calloc(1,sizeof(int64_t*));
                assert(ffi_arguments[k]);
                *(int64_t*)ffi_arguments[k] = drpc_to_int64(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_uint64){
                ffi_arguments[k] = calloc(1,sizeof(uint32_t*));
                assert(ffi_arguments[k]);
                *(uint64_t*)ffi_arguments[k] = drpc_to_uint64(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_float){
                ffi_arguments[k] = calloc(1,sizeof(float*));
                assert(ffi_arguments[k]);
                *(float*)ffi_arguments[k] = drpc_to_float(&arguments[j]);
                j++;k++;
                continue;
            }
            if(arguments[j].type == d_double){
                ffi_arguments[k] = calloc(1,sizeof(double*));
                assert(ffi_arguments[k]);
                *(double*)ffi_arguments[k] = drpc_to_double(&arguments[j]);
                j++;k++;
                continue;
            }
    }
    return ffi_arguments;
}

int drpc_server_call_fn(struct drpc_type* arguments,uint8_t arguments_len, struct drpc_function* fn_info, struct drpc_connection* client_info, struct drpc_return* returned){
    if(fn_info->return_type == d_sizedbuf || fn_info->return_type == d_fn_pstorage
    || fn_info->return_type == d_clientinfo || fn_info->return_type == d_interfunc
    || fn_info->return_type == d_delayed_massage_queue) return 1;
    enum drpc_types* extracted_prototype = drpc_types_extract_prototype(arguments,arguments_len);
    if(is_arguments_equal_prototype(fn_info->prototype,fn_info->prototype_len,extracted_prototype,arguments_len)){
        free(extracted_prototype);
        return 1;
    }
    struct drpc_que* to_repack = drpc_que_create();
    struct drpc_que* to_fill = drpc_que_create();

    size_t ffi_len = 0;
    ffi_arg native_return = 0;
    int8_t return_is = -1;

    void** ffi_arguments = ffi_from_drpc(arguments,fn_info->prototype,fn_info->prototype_len,&ffi_len,to_repack,to_fill);
    if(fn_info->cif == NULL){
        fn_info->cif = calloc(1,sizeof(*fn_info->cif)); assert(fn_info->cif);
        assert(ffi_prep_cif(fn_info->cif,FFI_DEFAULT_ABI,ffi_len,(ffi_type*)drpc_ffi_convert_table[fn_info->return_type],
                    (fn_info->ffi_prototype = drpc_proto_to_ffi(fn_info->prototype, fn_info->prototype_len))) == FFI_OK);
    }
    free(extracted_prototype);

    size_t to_fill_len = drpc_que_get_len(to_fill);
    for(size_t i = 0; i <to_fill_len; i++){
        struct drpc_type_update* to_update = drpc_que_pop(to_fill);
        switch(to_update->type){
            case d_fn_pstorage:
                **(void***)to_update->ptr = fn_info->personal;
                break;
            case d_interfunc:
                **(void***)to_update->ptr = client_info->drpc_server->interfunc;
                break;
            case d_clientinfo:
                **(void***)to_update->ptr = client_info;
                break;
            case d_delayed_massage_queue:
                **(void***)to_update->ptr = fn_info->delayed_massage_que;
            default:
                break;
        }
        free(to_update);
    }
    drpc_types_free(arguments,arguments_len);
    ffi_call(fn_info->cif,FFI_FN(fn_info->fn),&native_return,ffi_arguments);

    size_t repack_len = drpc_que_get_len(to_repack);

    if(repack_len > 0){
        returned->updated_arguments = calloc(repack_len,sizeof(*returned->updated_arguments));
        assert(returned->updated_arguments);
    }else returned->updated_arguments = NULL;
    returned->updated_arguments_len = repack_len;

    for(size_t i = 0; i < repack_len; i++){
        struct drpc_type_update* repack = drpc_que_pop(to_repack);
        assert(!(repack->type == d_sizedbuf && repack->ptr == (void*)native_return));

        if(repack->type == fn_info->return_type && repack->ptr == (void*)native_return) return_is = i;

        switch(repack->type){
            case d_str:
                str_to_drpc(&returned->updated_arguments[i],repack->ptr);
                free(repack->ptr);
                break;
            case d_sizedbuf:
                sizedbuf_to_drpc(&returned->updated_arguments[i],repack->ptr,repack->len);
                free(repack->ptr);
                break;
            case d_struct:
                d_struct_to_drpc(&returned->updated_arguments[i],repack->ptr);
                d_struct_free(repack->ptr);
                break;
            case d_array:
                d_array_to_drpc(&returned->updated_arguments[i],repack->ptr);
                //TODO: d_array_free(repack->ptr);
                break;
            default: break;
        }
        free(repack);

    }

    for(size_t i = 0; i <ffi_len; i++){
        free(ffi_arguments[i]);
    }
    free(ffi_arguments);

    if(return_is == -1){
        switch(fn_info->return_type){
            case d_void:
                void_to_drpc(&returned->returned);
                break;
            case d_int8:
                int8_to_drpc(&returned->returned,(int8_t)native_return);
                break;
            case d_uint8:
                uint8_to_drpc(&returned->returned,(uint8_t)native_return);
                break;
            case d_int16:
                int16_to_drpc(&returned->returned,(int16_t)native_return);
                break;
            case d_uint16:
                uint16_to_drpc(&returned->returned,(uint16_t)native_return);
                break;
            case d_int32:
                int32_to_drpc(&returned->returned,(int32_t)native_return);
                break;
            case d_uint32:
                uint32_to_drpc(&returned->returned,(uint32_t)native_return);
                break;
            case d_int64:
                int64_to_drpc(&returned->returned,(int64_t)native_return);
                break;
            case d_uint64:
                uint64_to_drpc(&returned->returned,(uint64_t)native_return);
                break;

            case d_float:
                float_to_drpc(&returned->returned,(float)native_return);
                break;
            case d_double:
                double_to_drpc(&returned->returned,(double)native_return);
                break;

            case d_str:
                if((char*)native_return == NULL) void_to_drpc(&returned->returned);
                else                             str_to_drpc(&returned->returned,(char*)native_return);
                free((char*)native_return);
                break;

            case d_array:
                if((char*)native_return == NULL) void_to_drpc(&returned->returned);
                else                             d_array_to_drpc(&returned->returned,(void*)native_return);
                //d_array_free((void*)native_return);
                break;
            case d_struct:
                if((char*)native_return == NULL) void_to_drpc(&returned->returned);
                else                             d_struct_to_drpc(&returned->returned,(void*)native_return);
                d_struct_free((void*)native_return);
                break;
            default: break;
        }
    }else{
        return_is_to_drpc(&returned->returned,return_is);
    }
    drpc_que_free(to_repack);
    drpc_que_free(to_fill);
    return 0;
}

void test(struct d_struct* dstruct){
    char* to_print = NULL;

    d_struct_get(dstruct,"125",&to_print,d_str);
    puts(to_print);

    d_struct_set(dstruct,"125","УТЕЧКИ НЕТ ЮХУ",d_str);
}

int main(){
    struct drpc_function fn = {0};
    enum drpc_types prototype[] = {d_struct};
    fn.fn = FFI_FN(test);
    fn.fn_name = "test";
    fn.return_type = d_void;
    fn.prototype = prototype;
    fn.prototype_len = 1;

    struct drpc_return ret = {0};
    struct drpc_type* arguments = calloc(1,sizeof(*arguments));

    struct d_struct* dstruct = new_d_struct();
    d_struct_set(dstruct,"125","тестовое сообщение для утечки памяти!",d_str);
    d_struct_to_drpc(&arguments[0],(void*)dstruct);
    d_struct_free(dstruct);

    drpc_server_call_fn(arguments,1,&fn,NULL,&ret);
    struct d_struct* unpacked = drpc_to_d_struct(&ret.updated_arguments[0]);
    char* to_print = NULL;

    d_struct_get(unpacked,"125",&to_print,d_str);
    puts(to_print);

    d_struct_free(unpacked);

    drpc_types_free(ret.updated_arguments,ret.updated_arguments_len);

    free(fn.cif);
    free(fn.ffi_prototype);

}

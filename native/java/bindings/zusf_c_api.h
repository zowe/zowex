#ifndef ZUSF_C_API_H
#define ZUSF_C_API_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Generic response structs
typedef struct {
    char* error_message; // Non-null if an error occurred
} ZUSFBasicResponse_C;

typedef struct {
    char* data;          // The string response content
    char* error_message; // Non-null if an error occurred
} ZUSFStringResponse_C;

// ListOptions mapping
typedef struct {
    bool all_files;
    bool long_format;
    int max_depth;
} ZUSFListOptions_C;

// API Functions
ZUSFBasicResponse_C* zusf_c_create_uss_file(const char* file, const char* mode);
ZUSFBasicResponse_C* zusf_c_create_uss_dir(const char* file, const char* mode);
ZUSFStringResponse_C* zusf_c_list_uss_dir(const char* path, const ZUSFListOptions_C* options);
ZUSFBasicResponse_C* zusf_c_move_uss_file_or_dir(const char* source, const char* destination);
ZUSFStringResponse_C* zusf_c_read_uss_file(const char* file, const char* codepage);
ZUSFStringResponse_C* zusf_c_write_uss_file(const char* file, const char* data, const char* codepage, const char* etag);
ZUSFBasicResponse_C* zusf_c_chmod_uss_item(const char* file, const char* mode, bool recursive);
ZUSFBasicResponse_C* zusf_c_delete_uss_item(const char* file, bool recursive);
ZUSFBasicResponse_C* zusf_c_chown_uss_item(const char* file, const char* owner, bool recursive);
ZUSFBasicResponse_C* zusf_c_chtag_uss_item(const char* file, const char* tag, bool recursive);

// Free functions
void zusf_c_free_basic_response(ZUSFBasicResponse_C* response);
void zusf_c_free_string_response(ZUSFStringResponse_C* response);

#ifdef __cplusplus
}
#endif

#endif // ZUSF_C_API_H

#ifndef ZDS_C_API_H
#define ZDS_C_API_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Generic response structs
typedef struct {
    char* error_message; // Non-null if an error occurred
} ZDSBasicResponse_C;

typedef struct {
    char* data;          // The string response content
    char* error_message; // Non-null if an error occurred
} ZDSStringResponse_C;

// ZDSEntry structure for dataset listings
typedef struct {
    char* name;
    char* dsorg;
    char* volser;
    char* recfm;
    bool migrated;
} ZDSEntry_C;

typedef struct {
    ZDSEntry_C* entries;
    size_t count;
    char* error_message;
} ZDSListResponse_C;

// ZDSMem structure for member listings
typedef struct {
    char* name;
} ZDSMem_C;

typedef struct {
    ZDSMem_C* members;
    size_t count;
    char* error_message;
} ZDSMemListResponse_C;

// DS_ATTRIBUTES structure
typedef struct {
    const char* alcunit;
    int blksize;
    int dirblk;
    const char* dsorg;
    int primary;
    const char* recfm;
    int lrecl;
    const char* dataclass;
    const char* unit;
    const char* dsntype;
    const char* mgntclass;
    const char* dsname;
    int avgblk;
    int secondary;
    int size;
    const char* storclass;
    const char* vol;
} DS_ATTRIBUTES_C;

// API Functions
ZDSBasicResponse_C* zds_c_create_data_set(const char* dsn, const DS_ATTRIBUTES_C* attributes);
ZDSListResponse_C* zds_c_list_data_sets(const char* dsn);
ZDSStringResponse_C* zds_c_read_data_set(const char* dsn, const char* codepage);
ZDSStringResponse_C* zds_c_write_data_set(const char* dsn, const char* data, const char* codepage, const char* etag);
ZDSBasicResponse_C* zds_c_delete_data_set(const char* dsn);
ZDSBasicResponse_C* zds_c_create_member(const char* dsn);
ZDSMemListResponse_C* zds_c_list_members(const char* dsn);

// Free functions
void zds_c_free_basic_response(ZDSBasicResponse_C* response);
void zds_c_free_string_response(ZDSStringResponse_C* response);
void zds_c_free_list_response(ZDSListResponse_C* response);
void zds_c_free_mem_list_response(ZDSMemListResponse_C* response);

#ifdef __cplusplus
}
#endif

#endif // ZDS_C_API_H

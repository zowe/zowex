#ifndef ZJB_C_API_H
#define ZJB_C_API_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Generic response structs
typedef struct {
    char* error_message; // Non-null if an error occurred
} ZJBBasicResponse_C;

typedef struct {
    char* data;          // The string response content
    char* error_message; // Non-null if an error occurred
} ZJBStringResponse_C;

// ZJob structure
typedef struct {
    char* jobname;
    char* jobid;
    char* owner;
    char* status;
    char* full_status;
    char* retcode;
    char* correlator;
} ZJob_C;

typedef struct {
    ZJob_C* jobs;
    size_t count;
    char* error_message;
} ZJobListResponse_C;

typedef struct {
    ZJob_C* job; // Single job
    char* error_message;
} ZJobResponse_C;

// ZJobDD structure
typedef struct {
    char* jobid;
    char* ddn;
    char* dsn;
    char* stepname;
    char* procstep;
    int key;
} ZJobDD_C;

typedef struct {
    ZJobDD_C* dds;
    size_t count;
    char* error_message;
} ZJobDDListResponse_C;

// API Functions
ZJobListResponse_C* zjb_c_list_jobs_by_owner(const char* owner_name, const char* prefix, const char* status);
ZJobResponse_C* zjb_c_get_job_status(const char* jobid);
ZJobDDListResponse_C* zjb_c_list_spool_files(const char* jobid);
ZJBStringResponse_C* zjb_c_read_spool_file(const char* jobid, int key);
ZJBStringResponse_C* zjb_c_get_job_jcl(const char* jobid);
ZJBStringResponse_C* zjb_c_submit_job(const char* jcl_content);
ZJBBasicResponse_C* zjb_c_delete_job(const char* jobid);

// Free functions
void zjb_c_free_basic_response(ZJBBasicResponse_C* response);
void zjb_c_free_string_response(ZJBStringResponse_C* response);
void zjb_c_free_job_list_response(ZJobListResponse_C* response);
void zjb_c_free_job_response(ZJobResponse_C* response);
void zjb_c_free_job_dd_list_response(ZJobDDListResponse_C* response);

#ifdef __cplusplus
}
#endif

#endif // ZJB_C_API_H

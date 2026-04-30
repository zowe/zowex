#include "zjb_c_api.h"
#include "../../c/zjb.hpp"
#include "../../python/bindings/conversion.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

static char* copy_string(const std::string& str) {
    if (str.empty()) return nullptr;
    char* copy = (char*)malloc(str.length() + 1);
    if (copy) {
        std::strcpy(copy, str.c_str());
    }
    return copy;
}

static void convert_jobs_to_ascii(std::vector<ZJob>& jobs) {
    for (auto& job : jobs) {
        e2a_inplace(job.jobname);
        e2a_inplace(job.jobid);
        e2a_inplace(job.owner);
        e2a_inplace(job.status);
        e2a_inplace(job.full_status);
        e2a_inplace(job.retcode);
        e2a_inplace(job.correlator);
    }
}

static void populate_zjob_c(ZJob_C* c_job, const ZJob& cpp_job) {
    c_job->jobname = copy_string(cpp_job.jobname);
    c_job->jobid = copy_string(cpp_job.jobid);
    c_job->owner = copy_string(cpp_job.owner);
    c_job->status = copy_string(cpp_job.status);
    c_job->full_status = copy_string(cpp_job.full_status);
    c_job->retcode = copy_string(cpp_job.retcode);
    c_job->correlator = copy_string(cpp_job.correlator);
}

extern "C" {

ZJobListResponse_C* zjb_c_list_jobs_by_owner(const char* owner_name, const char* prefix, const char* status) {
    ZJobListResponse_C* response = (ZJobListResponse_C*)calloc(1, sizeof(ZJobListResponse_C));
    try {
        std::vector<ZJob> jobs;
        ZJB zjb = {0};

        std::string owner_str = owner_name ? owner_name : "";
        std::string prefix_str = prefix ? prefix : "";
        std::string status_str = status ? status : "";

        a2e_inplace(owner_str);
        a2e_inplace(prefix_str);
        a2e_inplace(status_str);

        int rc = zjb_list_by_owner(&zjb, owner_str, prefix_str, status_str, jobs);

        if (rc != 0) {
            std::string diag(zjb.diag.e_msg, zjb.diag.e_msg_len);
            diag.push_back('\0');
            e2a_inplace(diag);
            diag.pop_back();
            response->error_message = copy_string(diag);
        } else {
            convert_jobs_to_ascii(jobs);
            response->count = jobs.size();
            if (response->count > 0) {
                response->jobs = (ZJob_C*)calloc(response->count, sizeof(ZJob_C));
                for (size_t i = 0; i < response->count; i++) {
                    populate_zjob_c(&response->jobs[i], jobs[i]);
                }
            }
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZJobResponse_C* zjb_c_get_job_status(const char* jobid) {
    ZJobResponse_C* response = (ZJobResponse_C*)calloc(1, sizeof(ZJobResponse_C));
    try {
        ZJob job = {0};
        ZJB zjb = {0};

        std::string jobid_str = jobid ? jobid : "";
        a2e_inplace(jobid_str);

        int rc = zjb_view(&zjb, jobid_str, job);

        if (rc != 0) {
            std::string diag(zjb.diag.e_msg, zjb.diag.e_msg_len);
            diag.push_back('\0');
            e2a_inplace(diag);
            diag.pop_back();
            response->error_message = copy_string(diag);
        } else {
            std::vector<ZJob> jobs = {job};
            convert_jobs_to_ascii(jobs);
            response->job = (ZJob_C*)calloc(1, sizeof(ZJob_C));
            populate_zjob_c(response->job, jobs[0]);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZJobDDListResponse_C* zjb_c_list_spool_files(const char* jobid) {
    ZJobDDListResponse_C* response = (ZJobDDListResponse_C*)calloc(1, sizeof(ZJobDDListResponse_C));
    try {
        std::vector<ZJobDD> jobDDs;
        ZJB zjb = {0};

        std::string jobid_str = jobid ? jobid : "";
        a2e_inplace(jobid_str);

        int rc = zjb_list_dds(&zjb, jobid_str, jobDDs);

        if (rc != 0) {
            std::string diag(zjb.diag.e_msg, zjb.diag.e_msg_len);
            diag.push_back('\0');
            e2a_inplace(diag);
            diag.pop_back();
            response->error_message = copy_string(diag);
        } else {
            response->count = jobDDs.size();
            if (response->count > 0) {
                response->dds = (ZJobDD_C*)calloc(response->count, sizeof(ZJobDD_C));
                for (size_t i = 0; i < response->count; i++) {
                    e2a_inplace(jobDDs[i].jobid);
                    e2a_inplace(jobDDs[i].ddn);
                    e2a_inplace(jobDDs[i].dsn);
                    e2a_inplace(jobDDs[i].stepname);
                    e2a_inplace(jobDDs[i].procstep);

                    response->dds[i].jobid = copy_string(jobDDs[i].jobid);
                    response->dds[i].ddn = copy_string(jobDDs[i].ddn);
                    response->dds[i].dsn = copy_string(jobDDs[i].dsn);
                    response->dds[i].stepname = copy_string(jobDDs[i].stepname);
                    response->dds[i].procstep = copy_string(jobDDs[i].procstep);
                    response->dds[i].key = jobDDs[i].key;
                }
            }
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZJBStringResponse_C* zjb_c_read_spool_file(const char* jobid, int key) {
    ZJBStringResponse_C* response = (ZJBStringResponse_C*)calloc(1, sizeof(ZJBStringResponse_C));
    try {
        std::string out_response;
        ZJB zjb = {0};

        std::string jobid_str = jobid ? jobid : "";
        a2e_inplace(jobid_str);

        int rc = zjb_read_jobs_output_by_key(&zjb, jobid_str, key, out_response);

        if (rc != 0) {
            std::string diag(zjb.diag.e_msg, zjb.diag.e_msg_len);
            diag.push_back('\0');
            e2a_inplace(diag);
            diag.pop_back();
            response->error_message = copy_string(diag);
        } else {
            e2a_inplace(out_response);
            response->data = copy_string(out_response);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZJBStringResponse_C* zjb_c_get_job_jcl(const char* jobid) {
    ZJBStringResponse_C* response = (ZJBStringResponse_C*)calloc(1, sizeof(ZJBStringResponse_C));
    try {
        std::string out_response;
        ZJB zjb = {0};

        std::string jobid_str = jobid ? jobid : "";
        a2e_inplace(jobid_str);

        int rc = zjb_read_job_jcl(&zjb, jobid_str, out_response);

        if (rc != 0) {
            std::string diag(zjb.diag.e_msg, zjb.diag.e_msg_len);
            diag.push_back('\0');
            e2a_inplace(diag);
            diag.pop_back();
            response->error_message = copy_string(diag);
        } else {
            e2a_inplace(out_response);
            response->data = copy_string(out_response);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZJBStringResponse_C* zjb_c_submit_job(const char* jcl_content) {
    ZJBStringResponse_C* response = (ZJBStringResponse_C*)calloc(1, sizeof(ZJBStringResponse_C));
    try {
        std::string jobid_out;
        ZJB zjb = {0};

        std::string jcl_str = jcl_content ? jcl_content : "";
        a2e_inplace(jcl_str);

        int rc = zjb_submit(&zjb, jcl_str, jobid_out);

        if (rc != 0) {
            std::string diag(zjb.diag.e_msg, zjb.diag.e_msg_len);
            diag.push_back('\0');
            e2a_inplace(diag);
            diag.pop_back();
            response->error_message = copy_string(diag);
        } else {
            e2a_inplace(jobid_out);
            response->data = copy_string(jobid_out);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZJBBasicResponse_C* zjb_c_delete_job(const char* jobid) {
    ZJBBasicResponse_C* response = (ZJBBasicResponse_C*)calloc(1, sizeof(ZJBBasicResponse_C));
    try {
        ZJB zjb = {0};
        std::string jobid_str = jobid ? jobid : "";
        a2e_inplace(jobid_str);

        int rc = zjb_delete(&zjb, jobid_str);

        if (rc != 0) {
            std::string diag(zjb.diag.e_msg, zjb.diag.e_msg_len);
            diag.push_back('\0');
            e2a_inplace(diag);
            diag.pop_back();
            response->error_message = copy_string(diag);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

static void free_zjob_c(ZJob_C* job) {
    if (job) {
        free(job->jobname);
        free(job->jobid);
        free(job->owner);
        free(job->status);
        free(job->full_status);
        free(job->retcode);
        free(job->correlator);
    }
}

void zjb_c_free_basic_response(ZJBBasicResponse_C* response) {
    if (response) {
        free(response->error_message);
        free(response);
    }
}

void zjb_c_free_string_response(ZJBStringResponse_C* response) {
    if (response) {
        free(response->data);
        free(response->error_message);
        free(response);
    }
}

void zjb_c_free_job_list_response(ZJobListResponse_C* response) {
    if (response) {
        if (response->jobs) {
            for (size_t i = 0; i < response->count; i++) {
                free_zjob_c(&response->jobs[i]);
            }
            free(response->jobs);
        }
        free(response->error_message);
        free(response);
    }
}

void zjb_c_free_job_response(ZJobResponse_C* response) {
    if (response) {
        if (response->job) {
            free_zjob_c(response->job);
            free(response->job);
        }
        free(response->error_message);
        free(response);
    }
}

void zjb_c_free_job_dd_list_response(ZJobDDListResponse_C* response) {
    if (response) {
        if (response->dds) {
            for (size_t i = 0; i < response->count; i++) {
                free(response->dds[i].jobid);
                free(response->dds[i].ddn);
                free(response->dds[i].dsn);
                free(response->dds[i].stepname);
                free(response->dds[i].procstep);
            }
            free(response->dds);
        }
        free(response->error_message);
        free(response);
    }
}

} // extern "C"
#include "zds_c_api.h"
#include "../../c/zds.hpp"
#include "../../python/bindings/conversion.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>

static char* copy_string(const std::string& str) {
    if (str.empty()) return nullptr;
    char* copy = (char*)malloc(str.length() + 1);
    if (copy) {
        std::strcpy(copy, str.c_str());
    }
    return copy;
}

extern "C" {

ZDSBasicResponse_C* zds_c_create_data_set(const char* dsn, const DS_ATTRIBUTES_C* attributes) {
    ZDSBasicResponse_C* response = (ZDSBasicResponse_C*)calloc(1, sizeof(ZDSBasicResponse_C));
    try {
        ZDS zds = {0};
        std::string diag_resp;
        DS_ATTRIBUTES attrs_copy = {0};
        
        std::string dsn_str = dsn ? dsn : "";
        a2e_inplace(dsn_str);

        if (attributes) {
            if (attributes->alcunit) attrs_copy.alcunit = attributes->alcunit;
            if (attributes->dsorg) attrs_copy.dsorg = attributes->dsorg;
            if (attributes->recfm) attrs_copy.recfm = attributes->recfm;
            if (attributes->dataclass) attrs_copy.dataclass = attributes->dataclass;
            if (attributes->unit) attrs_copy.unit = attributes->unit;
            if (attributes->dsntype) attrs_copy.dsntype = attributes->dsntype;
            if (attributes->mgntclass) attrs_copy.mgntclass = attributes->mgntclass;
            if (attributes->dsname) attrs_copy.dsname = attributes->dsname;
            if (attributes->storclass) attrs_copy.storclass = attributes->storclass;
            if (attributes->vol) attrs_copy.vol = attributes->vol;
            
            attrs_copy.blksize = attributes->blksize;
            attrs_copy.dirblk = attributes->dirblk;
            attrs_copy.primary = attributes->primary;
            attrs_copy.lrecl = attributes->lrecl;
            attrs_copy.avgblk = attributes->avgblk;
            attrs_copy.secondary = attributes->secondary;
            attrs_copy.size = attributes->size;
        }

        a2e_inplace(attrs_copy.alcunit);
        a2e_inplace(attrs_copy.dsorg);
        a2e_inplace(attrs_copy.recfm);
        a2e_inplace(attrs_copy.dataclass);
        a2e_inplace(attrs_copy.unit);
        a2e_inplace(attrs_copy.dsntype);
        a2e_inplace(attrs_copy.mgntclass);
        a2e_inplace(attrs_copy.dsname);
        a2e_inplace(attrs_copy.storclass);
        a2e_inplace(attrs_copy.vol);

        int rc = zds_create_dsn(&zds, dsn_str, attrs_copy, diag_resp);
        if (rc != 0) {
            std::string diag(diag_resp, diag_resp.length());
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

ZDSListResponse_C* zds_c_list_data_sets(const char* dsn) {
    ZDSListResponse_C* response = (ZDSListResponse_C*)calloc(1, sizeof(ZDSListResponse_C));
    try {
        std::vector<ZDSEntry> entries;
        ZDS zds = {0};
        std::string dsn_str = dsn ? dsn : "";
        a2e_inplace(dsn_str);
        
        int rc = zds_list_data_sets(&zds, dsn_str, entries, true);
        if (rc != 0) {
            std::string diag(zds.diag.e_msg, zds.diag.e_msg_len);
            diag.push_back('\0');
            e2a_inplace(diag);
            diag.pop_back();
            response->error_message = copy_string(diag);
        } else {
            response->count = entries.size();
            if (response->count > 0) {
                response->entries = (ZDSEntry_C*)calloc(response->count, sizeof(ZDSEntry_C));
                for (size_t i = 0; i < response->count; i++) {
                    e2a_inplace(entries[i].name);
                    e2a_inplace(entries[i].dsorg);
                    e2a_inplace(entries[i].volser);
                    e2a_inplace(entries[i].recfm);
                    
                    response->entries[i].name = copy_string(entries[i].name);
                    response->entries[i].dsorg = copy_string(entries[i].dsorg);
                    response->entries[i].volser = copy_string(entries[i].volser);
                    response->entries[i].recfm = copy_string(entries[i].recfm);
                    response->entries[i].migrated = entries[i].migrated;
                }
            }
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZDSStringResponse_C* zds_c_read_data_set(const char* dsn, const char* codepage) {
    ZDSStringResponse_C* response = (ZDSStringResponse_C*)calloc(1, sizeof(ZDSStringResponse_C));
    try {
        ZDS zds = {0};
        if (codepage && std::strlen(codepage) > 0) {
            std::string cp(codepage);
            zds.encoding_opts.data_type = (cp == "binary") ? eDataTypeBinary : eDataTypeText;
            strncpy(zds.encoding_opts.codepage, codepage, sizeof(zds.encoding_opts.codepage) - 1);
        }
        
        std::string dsn_str = dsn ? dsn : "";
        a2e_inplace(dsn_str);
        ZDSReadOpts read_opts{ .zds = &zds, .dsname = dsn_str };
        
        std::string data_response;
        int rc = zds_read(read_opts, data_response);
        if (rc != 0) {
            std::string diag(zds.diag.e_msg, zds.diag.e_msg_len);
            diag.push_back('\0');
            e2a_inplace(diag);
            diag.pop_back();
            response->error_message = copy_string(diag);
        } else {
            e2a_inplace(data_response);
            response->data = copy_string(data_response);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZDSStringResponse_C* zds_c_write_data_set(const char* dsn, const char* data, const char* codepage, const char* etag) {
    ZDSStringResponse_C* response = (ZDSStringResponse_C*)calloc(1, sizeof(ZDSStringResponse_C));
    try {
        ZDS zds = {0};
        if (codepage && std::strlen(codepage) > 0) {
            std::string cp(codepage);
            zds.encoding_opts.data_type = (cp == "binary") ? eDataTypeBinary : eDataTypeText;
            strncpy(zds.encoding_opts.codepage, codepage, sizeof(zds.encoding_opts.codepage) - 1);
        }
        if (etag && std::strlen(etag) > 0) {
            strncpy(zds.etag, etag, sizeof(zds.etag) - 1);
        }
        
        std::string dsn_str = dsn ? dsn : "";
        std::string data_str = data ? data : "";
        a2e_inplace(dsn_str);
        a2e_inplace(data_str);
        
        ZDSWriteOpts write_opts{ .zds = &zds, .dsname = dsn_str };
        int rc = zds_write(write_opts, data_str);
        if (rc != 0) {
            std::string diag(zds.diag.e_msg, zds.diag.e_msg_len);
            diag.push_back('\0');
            e2a_inplace(diag);
            diag.pop_back();
            response->error_message = copy_string(diag);
        } else {
            std::string new_etag(zds.etag);
            e2a_inplace(new_etag);
            response->data = copy_string(new_etag);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZDSBasicResponse_C* zds_c_delete_data_set(const char* dsn) {
    ZDSBasicResponse_C* response = (ZDSBasicResponse_C*)calloc(1, sizeof(ZDSBasicResponse_C));
    try {
        ZDS zds = {0};
        std::string dsn_str = dsn ? dsn : "";
        a2e_inplace(dsn_str);
        int rc = zds_delete_dsn(&zds, dsn_str);
        if (rc != 0) {
            std::string diag(zds.diag.e_msg, zds.diag.e_msg_len);
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

ZDSBasicResponse_C* zds_c_create_member(const char* dsn) {
    ZDSBasicResponse_C* response = (ZDSBasicResponse_C*)calloc(1, sizeof(ZDSBasicResponse_C));
    try {
        ZDS zds = {0};
        std::string dsn_str = dsn ? dsn : "";
        a2e_inplace(dsn_str);
        std::string empty_data = "";
        a2e_inplace(empty_data);
        
        ZDSWriteOpts write_opts{ .zds = &zds, .dsname = dsn_str };
        int rc = zds_write(write_opts, empty_data);
        if (rc != 0) {
            std::string diag(zds.diag.e_msg, zds.diag.e_msg_len);
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

ZDSMemListResponse_C* zds_c_list_members(const char* dsn) {
    ZDSMemListResponse_C* response = (ZDSMemListResponse_C*)calloc(1, sizeof(ZDSMemListResponse_C));
    try {
        std::vector<ZDSMem> members;
        ZDS zds = {0};
        std::string dsn_str = dsn ? dsn : "";
        a2e_inplace(dsn_str);
        
        int rc = zds_list_members(&zds, dsn_str, members, "", false);
        if (rc != 0) {
            std::string diag(zds.diag.e_msg, zds.diag.e_msg_len);
            diag.push_back('\0');
            e2a_inplace(diag);
            diag.pop_back();
            response->error_message = copy_string(diag);
        } else {
            response->count = members.size();
            if (response->count > 0) {
                response->members = (ZDSMem_C*)calloc(response->count, sizeof(ZDSMem_C));
                for (size_t i = 0; i < response->count; i++) {
                    e2a_inplace(members[i].name);
                    response->members[i].name = copy_string(members[i].name);
                }
            }
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

void zds_c_free_basic_response(ZDSBasicResponse_C* response) {
    if (response) {
        free(response->error_message);
        free(response);
    }
}

void zds_c_free_string_response(ZDSStringResponse_C* response) {
    if (response) {
        free(response->data);
        free(response->error_message);
        free(response);
    }
}

void zds_c_free_list_response(ZDSListResponse_C* response) {
    if (response) {
        if (response->entries) {
            for (size_t i = 0; i < response->count; i++) {
                free(response->entries[i].name);
                free(response->entries[i].dsorg);
                free(response->entries[i].volser);
                free(response->entries[i].recfm);
            }
            free(response->entries);
        }
        free(response->error_message);
        free(response);
    }
}

void zds_c_free_mem_list_response(ZDSMemListResponse_C* response) {
    if (response) {
        if (response->members) {
            for (size_t i = 0; i < response->count; i++) {
                free(response->members[i].name);
            }
            free(response->members);
        }
        free(response->error_message);
        free(response);
    }
}

} // extern "C"

#include "zusf_c_api.h"
#include "../../c/zusf.hpp"
#include <string>
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

ZUSFBasicResponse_C* zusf_c_create_uss_file(const char* file, const char* mode) {
    ZUSFBasicResponse_C* response = (ZUSFBasicResponse_C*)calloc(1, sizeof(ZUSFBasicResponse_C));
    try {
        ZUSF ctx = {0};
        std::string file_str = file ? file : "";
        std::string mode_str = mode ? mode : "644";
        mode_t octal_mode = std::stoi(mode_str, nullptr, 8);
        if (zusf_create_uss_file_or_dir(&ctx, file_str, octal_mode, CreateOptions(false)) != 0) {
            response->error_message = copy_string(ctx.diag.e_msg);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZUSFBasicResponse_C* zusf_c_create_uss_dir(const char* file, const char* mode) {
    ZUSFBasicResponse_C* response = (ZUSFBasicResponse_C*)calloc(1, sizeof(ZUSFBasicResponse_C));
    try {
        ZUSF ctx = {0};
        std::string file_str = file ? file : "";
        std::string mode_str = mode ? mode : "755";
        mode_t octal_mode = std::stoi(mode_str, nullptr, 8);
        if (zusf_create_uss_file_or_dir(&ctx, file_str, octal_mode, CreateOptions(true)) != 0) {
            response->error_message = copy_string(ctx.diag.e_msg);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZUSFBasicResponse_C* zusf_c_move_uss_file_or_dir(const char* source, const char* destination) {
    ZUSFBasicResponse_C* response = (ZUSFBasicResponse_C*)calloc(1, sizeof(ZUSFBasicResponse_C));
    try {
        ZUSF ctx = {0};
        std::string source_str = source ? source : "";
        std::string dest_str = destination ? destination : "";
        if (zusf_move_uss_file_or_dir(&ctx, source_str, dest_str) != 0) {
            response->error_message = copy_string(ctx.diag.e_msg);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZUSFStringResponse_C* zusf_c_list_uss_dir(const char* path, const ZUSFListOptions_C* options) {
    ZUSFStringResponse_C* response = (ZUSFStringResponse_C*)calloc(1, sizeof(ZUSFStringResponse_C));
    try {
        ZUSF ctx = {0};
        std::string out;
        std::string path_str = path ? path : "";
        ListOptions list_opts(false, false, 1);
        if (options) {
            list_opts.all_files = options->all_files;
            list_opts.long_format = options->long_format;
            list_opts.max_depth = options->max_depth;
        }
        if (zusf_list_uss_file_path(&ctx, path_str, out, list_opts, true) != 0) { // pass true for use_csv_format? Python SWIG bindings don't seem to pass it to the C++ code, wait, Python bindings calls zusf_list_uss_file_path with out which returns CSV usually. Let's see, zusf.hpp has `bool use_csv_format = false` as default. The SWIG file didn't pass it. Wait, `list_uss_dir` in `zusf_py.cpp` calls `zusf_list_uss_file_path(&ctx, path.c_str(), out, options)` which uses default `false` for CSV? Oh, wait. In `zusf_py.cpp` it calls `zusf_list_uss_file_path(&ctx, path.c_str(), out, options)`. Wait, let me check. Let's just use what's there.
            response->error_message = copy_string(ctx.diag.e_msg);
        } else {
            response->data = copy_string(out);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZUSFStringResponse_C* zusf_c_read_uss_file(const char* file, const char* codepage) {
    ZUSFStringResponse_C* response = (ZUSFStringResponse_C*)calloc(1, sizeof(ZUSFStringResponse_C));
    try {
        ZUSF ctx = {0};
        if (codepage && std::strlen(codepage) > 0) {
            std::string cp(codepage);
            ctx.encoding_opts.data_type = (cp == "binary") ? eDataTypeBinary : eDataTypeText;
            strncpy(ctx.encoding_opts.codepage, codepage, sizeof(ctx.encoding_opts.codepage) - 1);
        }
        std::string file_str = file ? file : "";
        std::string data_response;
        if (zusf_read_from_uss_file(&ctx, file_str, data_response) != 0) {
            response->error_message = copy_string(ctx.diag.e_msg);
        } else {
            response->data = copy_string(data_response);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZUSFStringResponse_C* zusf_c_write_uss_file(const char* file, const char* data, const char* codepage, const char* etag) {
    ZUSFStringResponse_C* response = (ZUSFStringResponse_C*)calloc(1, sizeof(ZUSFStringResponse_C));
    try {
        ZUSF ctx = {0};
        if (codepage && std::strlen(codepage) > 0) {
            std::string cp(codepage);
            ctx.encoding_opts.data_type = (cp == "binary") ? eDataTypeBinary : eDataTypeText;
            strncpy(ctx.encoding_opts.codepage, codepage, sizeof(ctx.encoding_opts.codepage) - 1);
        }
        if (etag && std::strlen(etag) > 0) {
            strncpy(ctx.etag, etag, sizeof(ctx.etag) - 1);
        }
        std::string file_str = file ? file : "";
        std::string data_copy = data ? data : "";
        if (zusf_write_to_uss_file(&ctx, file_str, data_copy) != 0) {
            response->error_message = copy_string(ctx.diag.e_msg);
        } else {
            response->data = copy_string(ctx.etag);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZUSFBasicResponse_C* zusf_c_chmod_uss_item(const char* file, const char* mode, bool recursive) {
    ZUSFBasicResponse_C* response = (ZUSFBasicResponse_C*)calloc(1, sizeof(ZUSFBasicResponse_C));
    try {
        ZUSF ctx = {0};
        std::string file_str = file ? file : "";
        std::string mode_str = mode ? mode : "0";
        mode_t octal_mode = std::stoi(mode_str, nullptr, 8);
        if (zusf_chmod_uss_file_or_dir(&ctx, file_str, octal_mode, recursive) != 0) {
            response->error_message = copy_string(ctx.diag.e_msg);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZUSFBasicResponse_C* zusf_c_delete_uss_item(const char* file, bool recursive) {
    ZUSFBasicResponse_C* response = (ZUSFBasicResponse_C*)calloc(1, sizeof(ZUSFBasicResponse_C));
    try {
        ZUSF ctx = {0};
        std::string file_str = file ? file : "";
        if (zusf_delete_uss_item(&ctx, file_str, recursive) != 0) {
            response->error_message = copy_string(ctx.diag.e_msg);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZUSFBasicResponse_C* zusf_c_chown_uss_item(const char* file, const char* owner, bool recursive) {
    ZUSFBasicResponse_C* response = (ZUSFBasicResponse_C*)calloc(1, sizeof(ZUSFBasicResponse_C));
    try {
        ZUSF ctx = {0};
        std::string file_str = file ? file : "";
        std::string owner_str = owner ? owner : "";
        if (zusf_chown_uss_file_or_dir(&ctx, file_str, owner_str, recursive) != 0) {
            response->error_message = copy_string(ctx.diag.e_msg);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

ZUSFBasicResponse_C* zusf_c_chtag_uss_item(const char* file, const char* tag, bool recursive) {
    ZUSFBasicResponse_C* response = (ZUSFBasicResponse_C*)calloc(1, sizeof(ZUSFBasicResponse_C));
    try {
        ZUSF ctx = {0};
        std::string file_str = file ? file : "";
        std::string tag_str = tag ? tag : "";
        if (zusf_chtag_uss_file_or_dir(&ctx, file_str, tag_str, recursive) != 0) {
            response->error_message = copy_string(ctx.diag.e_msg);
        }
    } catch (const std::exception& e) {
        response->error_message = copy_string(e.what());
    }
    return response;
}

void zusf_c_free_basic_response(ZUSFBasicResponse_C* response) {
    if (response) {
        free(response->error_message);
        free(response);
    }
}

void zusf_c_free_string_response(ZUSFStringResponse_C* response) {
    if (response) {
        free(response->data);
        free(response->error_message);
        free(response);
    }
}

} // extern "C"
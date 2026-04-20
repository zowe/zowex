/**
 * This program and the accompanying materials are made available under the terms of the
 * Eclipse Public License v2.0 which accompanies this distribution, and is available at
 * https://www.eclipse.org/legal/epl-v20.html
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Copyright Contributors to the Zowe Project.
 *
 */

#ifndef _OPEN_SYS_FILE_EXT
#define _OPEN_SYS_FILE_EXT 1
#endif

#include "ds.hpp"
#include "common_args.hpp"
#include "../zds.hpp"
#include "../zut.hpp"
#include <string>
#include <string_view>
#include <vector>

using namespace ast;
using namespace parser;
using namespace commands::common;

namespace ds
{
int process_data_set_create_result(InvocationContext &context, ZDS *zds, int rc, const std::string &dsn, std::string_view response)
{
  if (0 != rc)
  {
    context.error_stream() << "Error: could not create data set: '" << dsn << "' rc: '" << rc << "'" << std::endl;
    context.error_stream() << "  Details:\n"
                           << response << std::endl;
    return RTNCD_FAILURE;
  }

  // Handle member creation if specified
  size_t start = dsn.find_first_of('(');
  size_t end = dsn.find_last_of(')');
  if (start != std::string::npos && end != std::string::npos && end > start)
  {
    std::string member_name = dsn.substr(start + 1, end - start - 1);
    std::string data = "";
    ZDSWriteOpts write_opts{.zds = zds, .dsname = dsn};
    rc = zds_write(write_opts, data);
    if (0 != rc)
    {
      context.output_stream() << "Error: could not write to data set: '" << dsn << "' rc: '" << rc << "'" << std::endl;
      context.output_stream() << "  Details: " << zds->diag.e_msg << std::endl;
      return RTNCD_FAILURE;
    }
    context.output_stream() << "Data set and/or member created: '" << dsn << "'" << std::endl;
  }
  else
  {
    context.output_stream() << "Data set created: '" << dsn << "'" << std::endl;
  }

  return rc;
}

int create_with_attributes(InvocationContext &context)
{
  int rc = 0;
  std::string dsn = context.get<std::string>("dsn", "");
  ZDS zds{};
  DS_ATTRIBUTES attributes{};

  // Extract all the optional creation attributes
  if (context.has("alcunit"))
  {
    attributes.alcunit = context.get<std::string>("alcunit", "");
  }
  if (context.has("blksize"))
  {
    attributes.blksize = context.get<long long>("blksize", 0);
  }
  if (context.has("dirblk"))
  {
    attributes.dirblk = context.get<long long>("dirblk", 0);
  }
  if (context.has("dsorg"))
  {
    attributes.dsorg = context.get<std::string>("dsorg", "");
  }
  if (context.has("primary"))
  {
    attributes.primary = context.get<long long>("primary", 0);
  }
  if (context.has("recfm"))
  {
    attributes.recfm = context.get<std::string>("recfm", "");
  }
  if (context.has("lrecl"))
  {
    attributes.lrecl = context.get<long long>("lrecl", -1);
  }
  if (context.has("dataclass"))
  {
    attributes.dataclass = context.get<std::string>("dataclass", "");
  }
  if (context.has("unit"))
  {
    attributes.unit = context.get<std::string>("unit", "");
  }
  if (context.has("dsntype"))
  {
    attributes.dsntype = context.get<std::string>("dsntype", "");
  }
  if (context.has("mgntclass"))
  {
    attributes.mgntclass = context.get<std::string>("mgntclass", "");
  }
  if (context.has("dsname"))
  {
    attributes.dsname = context.get<std::string>("dsname", "");
  }
  if (context.has("avgblk"))
  {
    attributes.avgblk = context.get<long long>("avgblk", 0);
  }
  if (context.has("secondary"))
  {
    attributes.secondary = context.get<long long>("secondary", 0);
  }
  if (context.has("size"))
  {
    attributes.size = context.get<long long>("size", 0);
  }
  if (context.has("storclass"))
  {
    attributes.storclass = context.get<std::string>("storclass", "");
  }
  if (context.has("vol"))
  {
    attributes.vol = context.get<std::string>("vol", "");
  }

  std::string response;
  rc = zds_create_dsn(&zds, dsn, attributes, response);
  return process_data_set_create_result(context, &zds, rc, dsn, response);
}

const ast::Node build_ds_object(const ZDSEntry &entry, bool attributes)
{
  const auto obj_entry = obj();
  std::string trimmed_name = entry.name;
  obj_entry->set("name", str(zut_rtrim(trimmed_name)));

  if (!attributes)
    return obj_entry;

  if (entry.alloc != -1)
    obj_entry->set("alloc", i64(entry.alloc));
  if (entry.allocx != -1)
    obj_entry->set("allocx", i64(entry.allocx));
  if (entry.blksize != -1)
    obj_entry->set("blksize", i64(entry.blksize));
  if (!entry.cdate.empty())
    obj_entry->set("cdate", str(entry.cdate));
  if (!entry.dataclass.empty())
    obj_entry->set("dataclass", str(entry.dataclass));
  if (entry.devtype != 0)
    obj_entry->set("devtype", str(zut_hex_to_string(entry.devtype)));
  if (!entry.dsntype.empty())
    obj_entry->set("dsntype", str(entry.dsntype));
  if (!entry.dsorg.empty())
    obj_entry->set("dsorg", str(entry.dsorg));
  if (!entry.edate.empty())
    obj_entry->set("edate", str(entry.edate));
  if (entry.alloc != -1)
    obj_entry->set("encrypted", boolean(entry.encrypted));
  if (entry.lrecl != -1)
    obj_entry->set("lrecl", i64(entry.lrecl));
  if (!entry.mgmtclass.empty())
    obj_entry->set("mgmtclass", str(entry.mgmtclass));
  obj_entry->set("migrated", boolean(entry.migrated));
  obj_entry->set("multivolume", boolean(entry.multivolume));
  if (entry.primary != -1)
    obj_entry->set("primary", i64(entry.primary));
  if (!entry.rdate.empty())
    obj_entry->set("rdate", str(entry.rdate));
  if (!entry.recfm.empty())
    obj_entry->set("recfm", str(entry.recfm));
  if (entry.secondary != -1)
    obj_entry->set("secondary", i64(entry.secondary));
  if (!entry.spacu.empty())
    obj_entry->set("spacu", str(entry.spacu));
  if (!entry.storclass.empty())
    obj_entry->set("storclass", str(entry.storclass));
  if (entry.usedp != -1)
    obj_entry->set("usedp", i64(entry.usedp));
  if (entry.usedx != -1)
    obj_entry->set("usedx", i64(entry.usedx));
  obj_entry->set("volser", str(entry.volser));
  const auto volsers_array = arr();
  for (auto it = entry.volsers.begin(); it != entry.volsers.end(); ++it)
    volsers_array->push(str(*it));
  obj_entry->set("volsers", volsers_array);

  return obj_entry;
}

const ast::Node build_member_object(const ZDSMem &member, bool attributes)
{
  const auto obj_member = obj();

  std::string trimmed_name = member.name;
  obj_member->set("name", str(zut_rtrim(trimmed_name)));

  if (!attributes)
    return obj_member;

  if (member.vers != -1)
    obj_member->set("vers", i64(member.vers));

  if (member.mod != -1)
    obj_member->set("mod", i64(member.mod));

  if (!member.c4date.empty())
    obj_member->set("c4date", str(member.c4date));

  if (!member.m4date.empty())
    obj_member->set("m4date", str(member.m4date));

  if (!member.mtime.empty())
    obj_member->set("mtime", str(member.mtime));

  if (member.cnorc != -1)
    obj_member->set("cnorc", i64(member.cnorc));

  if (member.inorc != -1)
    obj_member->set("inorc", i64(member.inorc));

  if (member.mnorc != -1)
    obj_member->set("mnorc", i64(member.mnorc));

  if (!member.user.empty())
    obj_member->set("user", str(member.user));

  obj_member->set("sclm", str(member.sclm ? "Y" : "N"));

  return obj_member;
}

int handle_data_set_create_fb(InvocationContext &context)
{
  int rc = 0;
  std::string dsn = context.get<std::string>("dsn", "");
  ZDS zds{};
  std::string response;
  rc = zds_create_dsn_fb(&zds, dsn, response);
  return process_data_set_create_result(context, &zds, rc, dsn, response);
}

int handle_data_set_create_vb(InvocationContext &context)
{
  int rc = 0;
  std::string dsn = context.get<std::string>("dsn", "");
  ZDS zds{};
  std::string response;
  rc = zds_create_dsn_vb(&zds, dsn, response);
  return process_data_set_create_result(context, &zds, rc, dsn, response);
}

int handle_data_set_create_adata(InvocationContext &context)
{
  int rc = 0;
  std::string dsn = context.get<std::string>("dsn", "");
  ZDS zds{};
  std::string response;
  rc = zds_create_dsn_adata(&zds, dsn, response);
  return process_data_set_create_result(context, &zds, rc, dsn, response);
}

int handle_data_set_create_loadlib(InvocationContext &context)
{
  int rc = 0;
  std::string dsn = context.get<std::string>("dsn", "");
  ZDS zds{};
  std::string response;
  rc = zds_create_dsn_loadlib(&zds, dsn, response);
  return process_data_set_create_result(context, &zds, rc, dsn, response);
}

int handle_data_set_create_member(InvocationContext &context)
{
  int rc = 0;
  std::string dsn = context.get<std::string>("dsn", "");
  ZDS zds{};
  std::string response;
  std::vector<ZDSEntry> entries;

  size_t start = dsn.find_first_of('(');
  size_t end = dsn.find_last_of(')');
  std::string member_name;
  if (start != std::string::npos && end != std::string::npos && end > start)
  {
    member_name = dsn.substr(start + 1, end - start - 1);
    std::string dataset_name = dsn.substr(0, start);

    rc = zds_list_data_sets(&zds, dataset_name, entries);
    if (RTNCD_WARNING < rc || entries.size() == 0)
    {
      context.output_stream() << "Error: could not create data set member: '" << dataset_name << "' rc: '" << rc << "'" << std::endl;
      context.output_stream() << "  Details:\n"
                              << zds.diag.e_msg << std::endl;
      return RTNCD_FAILURE;
    }

    std::string data = "";
    ZDSWriteOpts write_opts{.zds = &zds, .dsname = dsn};
    rc = zds_write(write_opts, data);
    if (0 != rc)
    {
      context.output_stream() << "Error: could not write to data set: '" << dsn << "' rc: '" << rc << "'" << std::endl;
      context.output_stream() << "  Details: " << zds.diag.e_msg << std::endl;
      return RTNCD_FAILURE;
    }
    context.output_stream() << "Data set and/or member created: '" << dsn << "'" << std::endl;
  }
  else
  {
    context.output_stream() << "Error: could not find member name in dsn: '" << dsn << "'" << std::endl;
    return RTNCD_FAILURE;
  }

  return rc;
}

int handle_data_set_view(InvocationContext &context)
{
  int rc = 0;
  std::string dsn = context.get<std::string>("dsn", "");
  std::string ddname;
  ZDS zds{};
  std::vector<std::string> dds;

  if (context.has("encoding"))
  {
    zut_prepare_encoding(context.get<std::string>("encoding", ""), &zds.encoding_opts);
  }
  if (context.has("local-encoding"))
  {
    const auto source_encoding = context.get<std::string>("local-encoding", "");
    if (!source_encoding.empty() && source_encoding.size() < sizeof(zds.encoding_opts.source_codepage))
    {
      memcpy(zds.encoding_opts.source_codepage, source_encoding.data(), source_encoding.length() + 1);
    }
  }
  if (context.has("volser"))
  {
    std::string volser_value = context.get<std::string>("volser", "");
    if (!volser_value.empty())
    {
      dds.push_back("alloc dd(input) da('" + dsn + "') shr vol(" + volser_value + ")");
      ZDIAG diag{};
      rc = zut_loop_dynalloc(diag, dds);
      if (0 != rc)
      {
        context.error_stream() << diag.e_msg << std::endl;
        return RTNCD_FAILURE;
      }
      ddname = "INPUT";
    }
  }

  bool has_pipe_path = context.has("pipe-path");
  std::string pipe_path = context.get<std::string>("pipe-path", "");
  const auto result = obj();

  ZDSReadOpts read_opts{.zds = &zds, .ddname = ddname, .dsname = dsn};

  if (has_pipe_path && !pipe_path.empty())
  {
    size_t content_len = 0;
    rc = zds_read_streamed(read_opts, pipe_path, &content_len);

    if (context.get<bool>("return-etag", false))
    {
      std::string temp_content;
      auto read_rc = zds_read(read_opts, temp_content);
      if (read_rc == 0)
      {
        const auto etag = zut_calc_adler32_checksum(temp_content);
        std::stringstream etag_stream;
        etag_stream << std::hex << etag << std::dec;
        if (!context.is_redirecting_output())
        {
          context.output_stream() << "etag: " << etag_stream.str() << std::endl;
        }
        result->set("etag", str(etag_stream.str()));
      }
    }

    if (!context.is_redirecting_output())
    {
      context.output_stream() << "size: " << content_len << std::endl;
    }
    result->set("contentLen", i64(content_len));
  }
  else
  {
    std::string response;
    rc = zds_read(read_opts, response);
    if (0 != rc)
    {
      context.error_stream() << "Error: could not read data set: '" << dsn << "' rc: '" << rc << "'" << std::endl;
      context.error_stream() << "  Details: " << zds.diag.e_msg << std::endl;
      return RTNCD_FAILURE;
    }

    if (context.get<bool>("return-etag", false))
    {
      const auto etag = zut_calc_adler32_checksum(response);
      std::stringstream etag_stream;
      etag_stream << std::hex << etag << std::dec;
      if (!context.is_redirecting_output())
      {
        context.output_stream() << "etag: " << etag_stream.str() << std::endl;
        context.output_stream() << "data: ";
      }
      result->set("etag", str(etag_stream.str()));
    }

    bool has_encoding = context.has("encoding");
    bool response_format_bytes = context.get<bool>("response-format-bytes", false);

    if (has_encoding && response_format_bytes)
    {
      zut_print_string_as_bytes(response, &context.output_stream());
    }
    else
    {
      context.output_stream() << response;
    }
  }

  if (dds.size() > 0)
  {
    ZDIAG diag{};
    rc = zut_free_dynalloc_dds(diag, dds);
    if (0 != rc)
    {
      context.error_stream() << diag.e_msg << std::endl;
      return RTNCD_FAILURE;
    }
  }

  context.set_object(result);

  return rc;
}

int handle_data_set_list(InvocationContext &context)
{
  int rc = 0;
  ZLOG_DEBUG("[>] handle_data_set_list");
  std::string dsn = context.get<std::string>("dsn", "");

  if (dsn.length() > MAX_DS_LENGTH)
  {
    context.error_stream() << "Error: data set pattern exceeds 44 character length limit" << std::endl;
    return RTNCD_FAILURE;
  }

  dsn += ".**";

  long long max_entries = context.get<long long>("max-entries", 0);
  bool warn = context.get<bool>("warn", true);
  bool attributes = context.get<bool>("attributes", false);

  ZDS zds{};
  if (max_entries > 0)
  {
    zds.max_entries = max_entries;
  }
  std::vector<ZDSEntry> entries;

  const auto num_attr_fields = 10;
  bool emit_csv = context.get<bool>("response-format-csv", false);
  rc = zds_list_data_sets(&zds, dsn, entries, attributes);
  if (RTNCD_SUCCESS == rc || RTNCD_WARNING == rc)
  {
    std::vector<std::string> fields;
    fields.reserve((attributes ? num_attr_fields : 0) + 1);
    const auto entries_array = arr();

    for (auto &entry : entries)
    {
      if (emit_csv)
      {
        fields.push_back(entry.name);
        if (attributes)
        {
          fields.push_back(entry.multivolume ? (entry.volser + "+") : entry.volser);
          fields.push_back(entry.devtype != 0 ? zut_hex_to_string(entry.devtype) : "");
          fields.push_back(entry.dsorg);
          fields.push_back(entry.recfm);
          fields.push_back(entry.lrecl == -1 ? "" : std::to_string(entry.lrecl));
          fields.push_back(entry.blksize == -1 ? "" : std::to_string(entry.blksize));
          fields.push_back(entry.primary == -1 ? "" : std::to_string(entry.primary));
          fields.push_back(entry.secondary == -1 ? "" : std::to_string(entry.secondary));
          fields.push_back(entry.dsntype);
          fields.emplace_back(entry.migrated ? "YES" : "NO");
        }
        context.output_stream() << zut_format_as_csv(fields) << std::endl;
        fields.clear();
      }
      else
      {
        if (attributes)
        {
          context.output_stream() << std::left
                                  << std::setw(44) << entry.name << " "
                                  << std::setw(7) << (entry.multivolume ? (entry.volser + "+") : entry.volser) << " "
                                  << std::setw(7) << (entry.devtype != 0 ? zut_hex_to_string(entry.devtype) : "") << " "
                                  << std::setw(4) << entry.dsorg << " "
                                  << std::setw(6) << entry.recfm << " "
                                  << std::setw(6) << (entry.lrecl == -1 ? "" : std::to_string(entry.lrecl)) << " "
                                  << std::setw(6) << (entry.blksize == -1 ? "" : std::to_string(entry.blksize)) << " "
                                  << std::setw(10) << (entry.primary == -1 ? "" : std::to_string(entry.primary)) << " "
                                  << std::setw(10) << (entry.secondary == -1 ? "" : std::to_string(entry.secondary)) << " "
                                  << std::setw(8) << entry.dsntype << " "
                                  << (entry.migrated ? "YES" : "NO")
                                  << std::endl;
        }
        else
        {
          context.output_stream() << std::left << std::setw(44) << entry.name << std::endl;
        }
      }

      const auto ds_obj = build_ds_object(entry, attributes);
      entries_array->push(ds_obj);
    }

    const auto result = obj();
    result->set("items", entries_array);
    result->set("returnedRows", i64(entries.size()));
    context.set_object(result);
  }
  if (RTNCD_WARNING == rc)
  {
    if (warn)
    {
      if (ZDS_RSNCD_MAXED_ENTRIES_REACHED == zds.diag.detail_rc)
      {
        context.error_stream() << "Warning: results truncated" << std::endl;
      }
      else if (ZDS_RSNCD_NOT_FOUND == zds.diag.detail_rc)
      {
        context.error_stream() << "Warning: no matching results found" << std::endl;
      }
    }
  }

  if (RTNCD_SUCCESS != rc && RTNCD_WARNING != rc)
  {
    context.error_stream() << "Error: could not list data set: '" << dsn << "' rc: '" << rc << "'" << std::endl;
    context.error_stream() << "  Details: " << zds.diag.e_msg << std::endl;
    return RTNCD_FAILURE;
  }

  return (!warn && rc == RTNCD_WARNING) ? RTNCD_SUCCESS : rc;
}

int handle_data_set_list_members(InvocationContext &context)
{
  int rc = 0;
  std::string dsn = context.get<std::string>("dsn", "");
  long long max_entries = context.get<long long>("max-entries", 0);
  bool warn = context.get<bool>("warn", true);
  bool attributes = context.get<bool>("attributes", false);
  bool emit_csv = context.get<bool>("response-format-csv", false);
  std::string pattern = context.get<std::string>("pattern", "");

  ZDS zds{};
  if (max_entries > 0)
  {
    zds.max_entries = max_entries;
  }
  std::vector<ZDSMem> members;
  rc = zds_list_members(&zds, dsn, members, pattern, attributes);

  if (RTNCD_SUCCESS == rc || RTNCD_WARNING == rc)
  {
    std::vector<std::string> fields;
    const auto entries_array = arr();
    for (std::vector<ZDSMem>::iterator it = members.begin(); it != members.end(); ++it)
    {
      if (emit_csv)
      {
        fields.push_back(it->name);

        if (attributes)
        {
          fields.push_back(it->vers == -1 ? "" : std::to_string(it->vers));
          fields.push_back(it->mod == -1 ? "" : std::to_string(it->mod));
          fields.push_back(it->c4date);
          fields.push_back(it->m4date);
          fields.push_back(it->mtime);
          fields.push_back(it->cnorc == -1 ? "" : std::to_string(it->cnorc));
          fields.push_back(it->inorc == -1 ? "" : std::to_string(it->inorc));
          fields.push_back(it->mnorc == -1 ? "" : std::to_string(it->mnorc));
          fields.push_back(it->user);
          fields.push_back(it->sclm ? "Y" : "N");
        }

        context.output_stream() << zut_format_as_csv(fields) << std::endl;
        fields.clear();
      }
      else
      {
        if (attributes)
        {
          context.output_stream() << std::left
                                  << std::setw(12) << it->name << " "
                                  << std::setw(4) << (it->vers == -1 ? "" : std::to_string(it->vers)) << " "
                                  << std::setw(4) << (it->mod == -1 ? "" : std::to_string(it->mod)) << " "
                                  << std::setw(10) << it->c4date << " "
                                  << std::setw(10) << it->m4date << " "
                                  << std::setw(8) << it->mtime << " "
                                  << std::setw(6) << (it->cnorc == -1 ? "" : std::to_string(it->cnorc)) << " "
                                  << std::setw(6) << (it->inorc == -1 ? "" : std::to_string(it->inorc)) << " "
                                  << std::setw(6) << (it->mnorc == -1 ? "" : std::to_string(it->mnorc)) << " "
                                  << std::setw(8) << it->user << " "
                                  << (it->sclm ? "Y" : "N")
                                  << std::endl;
        }
        else
        {
          context.output_stream() << std::left << std::setw(12) << it->name << std::endl;
        }
      }

      const auto entry = build_member_object(*it, attributes);
      entries_array->push(entry);
    }
    const auto result = obj();
    result->set("items", entries_array);
    result->set("returnedRows", i64(members.size()));
    context.set_object(result);
  }
  if (RTNCD_WARNING == rc)
  {
    if (warn)
    {
      if (ZDS_RSNCD_MAXED_ENTRIES_REACHED == zds.diag.detail_rc)
      {
        context.error_stream() << "Warning: results truncated" << std::endl;
      }
    }
  }
  if (RTNCD_SUCCESS != rc && RTNCD_WARNING != rc)
  {
    context.error_stream() << "Error: " << zds.diag.e_msg << std::endl;
    return RTNCD_FAILURE;
  }

  return (!warn && rc == RTNCD_WARNING) ? RTNCD_SUCCESS : rc;
}

int handle_data_set_write(InvocationContext &context)
{
  int rc = 0;
  std::string dsn = context.get<std::string>("dsn", "");
  ZDS zds{};
  std::vector<std::string> dds;

  if (context.has("encoding"))
  {
    zut_prepare_encoding(context.get<std::string>("encoding", ""), &zds.encoding_opts);
  }
  if (context.has("local-encoding"))
  {
    const auto source_encoding = context.get<std::string>("local-encoding", "");
    if (!source_encoding.empty() && source_encoding.size() < sizeof(zds.encoding_opts.source_codepage))
    {
      memcpy(zds.encoding_opts.source_codepage, source_encoding.data(), source_encoding.length() + 1);
    }
  }

  if (context.has("etag"))
  {
    std::string etag_value = context.get<std::string>("etag", "");
    if (etag_value.empty())
    {
      // Adler-32 etags that consist only of decimal digits (no a-f) are
      // lexed as integers rather than strings; recover the original hex string
      // by formatting the stored integer back as decimal (its digits are the etag)
      const long long *etag_int = context.get_if<long long>("etag");
      if (etag_int)
      {
        std::stringstream ss;
        ss << *etag_int;
        etag_value = ss.str();
      }
    }
    if (!etag_value.empty())
    {
      strcpy(zds.etag, etag_value.c_str());
    }
  }

  if (context.has("volser"))
  {
    std::string volser_value = context.get<std::string>("volser", "");
    if (!volser_value.empty())
    {
      dds.push_back("alloc dd(output) da('" + dsn + "') shr vol(" + volser_value + ")");
      ZDIAG diag{};
      rc = zut_loop_dynalloc(diag, dds);
      if (0 != rc)
      {
        context.error_stream() << diag.e_msg << std::endl;
        return RTNCD_FAILURE;
      }
      strcpy(zds.ddname, "OUTPUT");
    }
  }

  bool has_pipe_path = context.has("pipe-path");
  std::string pipe_path = context.get<std::string>("pipe-path", "");
  size_t content_len = 0;
  const auto result = obj();

  if (has_pipe_path && !pipe_path.empty())
  {
    ZDSWriteOpts write_opts{.zds = &zds, .dsname = dsn};
    rc = zds_write_streamed(write_opts, pipe_path, &content_len);
    result->set("contentLen", i64(content_len));
  }
  else
  {
    std::string data = zut_read_input(context.input_stream());
    ZDSWriteOpts write_opts{.zds = &zds, .dsname = dsn};
    rc = zds_write(write_opts, data);
  }

  if (dds.size() > 0)
  {
    ZDIAG diag{};
    int free_rc = zut_free_dynalloc_dds(diag, dds);
    if (0 != free_rc)
    {
      context.error_stream() << diag.e_msg << std::endl;
      return RTNCD_FAILURE;
    }
  }

  // Handle truncation warning
  if (RTNCD_WARNING == rc && ZDS_RSNCD_TRUNCATION_WARNING == zds.diag.detail_rc)
  {
    context.error_stream() << "\nWarning: " << zds.diag.e_msg << std::endl;
    result->set("truncationWarning", str(zds.diag.e_msg));
    // Continues w/ logic below to output success RC - operation succeeded with warning
  }
  else if (0 != rc)
  {
    context.error_stream() << "Error: could not write to data set: '" << dsn << "' rc: '" << rc << "'" << std::endl;
    context.error_stream() << "  Details: " << zds.diag.e_msg << std::endl;
    return RTNCD_FAILURE;
  }

  if (context.get<bool>("etag-only", false))
  {
    context.output_stream() << "etag: " << zds.etag << std::endl;
    if (content_len > 0)
      context.output_stream() << "size: " << content_len << std::endl;
  }
  else
  {
    context.output_stream() << "Wrote data to '" << dsn << "'" << std::endl;
  }

  result->set("etag", str(zds.etag));
  context.set_object(result);

  return RTNCD_SUCCESS;
}

int handle_data_set_delete(InvocationContext &context)
{
  int rc = 0;
  std::string dsn = context.get<std::string>("dsn", "");
  ZDS zds{};
  rc = zds_delete_dsn(&zds, dsn);

  if (0 != rc)
  {
    context.error_stream() << "Error: could not delete data set: '" << dsn << "' rc: '" << rc << "'" << std::endl;
    context.error_stream() << "  Details: " << zds.diag.e_msg << std::endl;
    return RTNCD_FAILURE;
  }
  context.output_stream() << "Data set '" << dsn << "' deleted" << std::endl;

  return rc;
}

int handle_data_set_rename(InvocationContext &context)
{
  int rc = 0;
  std::string dsn_before = context.get<std::string>("dsname-before", "");
  std::string dsn_after = context.get<std::string>("dsname-after", "");
  ZDS zds{};

  rc = zds_rename_dsn(&zds, dsn_before, dsn_after);

  if (0 != rc)
  {
    context.error_stream() << "Error: Could not rename data set: '" << dsn_before << "' rc: '" << rc << "'" << std::endl;
    context.error_stream() << " Details: " << zds.diag.e_msg << std::endl;
    return RTNCD_FAILURE;
  }
  context.output_stream() << "Data set '" << dsn_before << "' renamed to '" << dsn_after << "'" << std::endl;

  return rc;
}

int handle_rename_member(InvocationContext &context)
{
  int rc = 0;
  std::string dsname = context.get<std::string>("dsn", "");
  std::string member_before = context.get<std::string>("member-before", "");
  std::string member_after = context.get<std::string>("member-after", "");
  ZDS zds{};

  rc = zds_rename_members(&zds, dsname, member_before, member_after);
  std::string source_member = dsname + "(" + member_before + ")";
  if (0 != rc)
  {
    context.error_stream() << "Error: Could not rename member: '" << source_member << "' rc: '" << rc << "'" << std::endl;
    context.error_stream() << " Details: " << zds.diag.e_msg << std::endl;
    return RTNCD_FAILURE;
  }
  context.output_stream() << "Data set member '" << member_before << "' renamed to '" << member_after << "'" << std::endl;

  return rc;
}

int handle_data_set_restore(InvocationContext &context)
{
  int rc = 0;
  std::string dsn = context.get<std::string>("dsn", "");

  // perform dynalloc
  std::vector<std::string> dds;
  dds.reserve(2);
  dds.push_back("alloc da('" + dsn + "') shr");
  dds.push_back("free da('" + dsn + "')");

  ZDIAG diag{};
  rc = zut_loop_dynalloc(diag, dds);
  if (0 != rc)
  {
    context.error_stream() << "Error: could not restore data set: '" << dsn << "' rc: '" << rc << "'" << std::endl;
    context.error_stream() << "Details: " << diag.e_msg << std::endl;
    return RTNCD_FAILURE;
  }

  context.output_stream() << "Data set '" << dsn << "' restored" << std::endl;

  return rc;
}

int handle_data_set_compress(InvocationContext &context)
{
  int rc = 0;
  std::string dsn = context.get<std::string>("dsn", "");

  std::transform(dsn.begin(), dsn.end(), dsn.begin(), ::toupper);

  bool is_pds = false;

  std::string dsn_formatted = "//'" + dsn + "'";
  FILE *dir = fopen(dsn_formatted.c_str(), "r");
  if (dir)
  {
    fldata_t file_info = {0};
    char file_name[64] = {0};
    if (0 == fldata(dir, file_name, &file_info))
    {
      if (file_info.__dsorgPDSdir && !file_info.__dsorgPDSE)
      {
        is_pds = true;
      }
    }
    fclose(dir);
  }

  if (!is_pds)
  {
    context.error_stream() << "Error: data set '" << dsn << "' is not a PDS" << std::endl;
    return RTNCD_FAILURE;
  }

  // perform dynalloc
  std::vector<std::string> dds;
  dds.reserve(4);
  dds.push_back("alloc dd(a) da('" + dsn + "') shr");
  dds.push_back("alloc dd(b) da('" + dsn + "') shr");
  dds.push_back("alloc dd(sysprint) lrecl(80) recfm(f,b) blksize(80)");
  dds.push_back("alloc dd(sysin) lrecl(80) recfm(f,b) blksize(80)");

  ZDIAG diag{};
  rc = zut_loop_dynalloc(diag, dds);
  if (0 != rc)
  {
    context.error_stream() << "Error: allocation failed" << std::endl;
    context.error_stream() << "  Details: " << diag.e_msg << std::endl;
    return RTNCD_FAILURE;
  }

  // write control statements
  ZDS zds{};
  ZDSWriteOpts write_opts{.zds = &zds, .ddname = "sysin"};
  std::string copy_stmt = "        COPY OUTDD=B,INDD=A";
  rc = zds_write(write_opts, copy_stmt);
  if (0 != rc)
  {
    context.error_stream() << "Error: could not write to dd: '" << "sysin" << "' rc: '" << rc << "'" << std::endl;
    context.error_stream() << "  Details: " << zds.diag.e_msg << std::endl;
    zut_free_dynalloc_dds(diag, dds);
    return RTNCD_FAILURE;
  }

  // perform compress
  rc = zut_run("IEBCOPY");
  if (RTNCD_SUCCESS != rc)
  {
    context.error_stream() << "Error: could not invoke IEBCOPY rc: '" << rc << "'" << std::endl;
    zut_free_dynalloc_dds(diag, dds);
    return RTNCD_FAILURE;
  }

  // read output from iebcopy
  std::string output;
  ZDSReadOpts iebcopy_read_opts{.zds = &zds, .ddname = "SYSPRINT", .dsname = dsn};
  rc = zds_read(iebcopy_read_opts, output);
  if (0 != rc)
  {
    context.error_stream() << "Error: could not read from dd: '" << "SYSPRINT" << "' rc: '" << rc << "'" << std::endl;
    context.error_stream() << "  Details: " << zds.diag.e_msg << std::endl;
    context.error_stream() << output << std::endl;
    zut_free_dynalloc_dds(diag, dds);
    return RTNCD_FAILURE;
  }
  context.output_stream() << "Data set '" << dsn << "' compressed" << std::endl;

  // free dynalloc dds
  rc = zut_free_dynalloc_dds(diag, dds);
  if (0 != rc)
  {
    context.error_stream() << "Error: allocation failed" << std::endl;
    context.error_stream() << "  Details: " << diag.e_msg << std::endl;
    return RTNCD_FAILURE;
  }

  return RTNCD_SUCCESS;
}

int handle_data_set_copy(InvocationContext &context)
{
  const std::string source = context.get<std::string>("source", "");
  const std::string target = context.get<std::string>("target", "");

  ZDS zds{};
  ZDSCopyOptions options;
  options.replace = context.get<bool>("replace", false);
  options.delete_target_members = context.get<bool>("delete-target-members", false);

  int rc = zds_copy_dsn(&zds, source, target, &options);

  if (rc != RTNCD_SUCCESS)
  {
    context.error_stream() << "Error: copy failed" << std::endl;
    if (zds.diag.e_msg_len > 0)
    {
      context.error_stream() << "  Details: " << zds.diag.e_msg << std::endl;
    }
    return RTNCD_FAILURE;
  }

  if (options.target_created)
  {
    context.output_stream() << "New data set '" << target << "' created and copied from '" << source << "'" << std::endl;
  }
  else if (options.member_created)
  {
    context.output_stream() << "New member '" << target << "' created and copied from '" << source << "'" << std::endl;
  }
  else if (options.delete_target_members)
  {
    context.output_stream() << "Target members deleted and data set '" << target << "' replaced with contents of '" << source << "'" << std::endl;
  }
  else if (options.replace)
  {
    context.output_stream() << "Data set '" << target << "' has been updated with contents of '" << source << "'" << std::endl;
  }
  else
  {
    context.output_stream() << "Data set '" << source << "' copied to '" << target << "'" << std::endl;
  }
  return RTNCD_SUCCESS;
}

void register_commands(parser::Command &root_command)
{
  // Data set command group
  auto data_set_cmd = command_ptr(new Command("data-set", "z/OS data set operations"));
  data_set_cmd->add_alias("ds");

  // Create subcommand
  auto ds_create_cmd = command_ptr(new Command("create", "create data set"));
  ds_create_cmd->add_alias("cre");
  ds_create_cmd->add_positional_arg(DSN);

  // Data set creation attributes
  ds_create_cmd->add_keyword_arg("alcunit", make_aliases("--alcunit"), "Allocation unit", ArgType_Single, false);
  ds_create_cmd->add_keyword_arg("blksize", make_aliases("--blksize"), "Block size", ArgType_Single, false);
  ds_create_cmd->add_keyword_arg("dirblk", make_aliases("--dirblk"), "Directory blocks", ArgType_Single, false);
  ds_create_cmd->add_keyword_arg("dsorg", make_aliases("--dsorg"), "Data set organization", ArgType_Single, false);
  ds_create_cmd->add_keyword_arg("primary", make_aliases("--primary"), "Primary space", ArgType_Single, false);
  ds_create_cmd->add_keyword_arg("recfm", make_aliases("--recfm"), "Record format", ArgType_Single, false, ArgValue(std::string("FB")));
  ds_create_cmd->add_keyword_arg("lrecl", make_aliases("--lrecl"), "Record length", ArgType_Single, false);
  ds_create_cmd->add_keyword_arg("dataclass", make_aliases("--dataclass"), "Data class", ArgType_Single, false);
  ds_create_cmd->add_keyword_arg("unit", make_aliases("--unit"), "Device type", ArgType_Single, false);
  ds_create_cmd->add_keyword_arg("dsntype", make_aliases("--dsntype"), "Data set type", ArgType_Single, false);
  ds_create_cmd->add_keyword_arg("mgntclass", make_aliases("--mgntclass"), "Management class", ArgType_Single, false);
  ds_create_cmd->add_keyword_arg("dsname", make_aliases("--dsname"), "Data set name", ArgType_Single, false);
  ds_create_cmd->add_keyword_arg("avgblk", make_aliases("--avgblk"), "Average block length", ArgType_Single, false);
  ds_create_cmd->add_keyword_arg("secondary", make_aliases("--secondary"), "Secondary space", ArgType_Single, false);
  ds_create_cmd->add_keyword_arg("size", make_aliases("--size"), "Size", ArgType_Single, false);
  ds_create_cmd->add_keyword_arg("storclass", make_aliases("--storclass"), "Storage class", ArgType_Single, false);
  ds_create_cmd->add_keyword_arg("vol", make_aliases("--vol"), "Volume serial", ArgType_Single, false);
  ds_create_cmd->set_handler(create_with_attributes);
  data_set_cmd->add_command(ds_create_cmd);

  // Create-fb subcommand
  auto ds_create_fb_cmd = command_ptr(new Command("create-fb", "create FB data set using defaults: DSORG=PO, RECFM=FB, LRECL=80 "));
  ds_create_fb_cmd->add_alias("cre-fb");
  ds_create_fb_cmd->add_positional_arg(DSN);
  ds_create_fb_cmd->set_handler(handle_data_set_create_fb);
  data_set_cmd->add_command(ds_create_fb_cmd);

  // Create-vb subcommand
  auto ds_create_vb_cmd = command_ptr(new Command("create-vb", "create VB data set using defaults: DSORG=PO, RECFM=VB, LRECL=255"));
  ds_create_vb_cmd->add_alias("cre-vb");
  ds_create_vb_cmd->add_positional_arg(DSN);
  ds_create_vb_cmd->set_handler(handle_data_set_create_vb);
  data_set_cmd->add_command(ds_create_vb_cmd);

  // Create-adata subcommand
  auto ds_create_adata_cmd = command_ptr(new Command("create-adata", "create VB data set using defaults: DSORG=PO, RECFM=VB, LRECL=32756"));
  ds_create_adata_cmd->add_alias("cre-a");
  ds_create_adata_cmd->add_positional_arg(DSN);
  ds_create_adata_cmd->set_handler(handle_data_set_create_adata);
  data_set_cmd->add_command(ds_create_adata_cmd);

  // Create-loadlib subcommand
  auto ds_create_loadlib_cmd = command_ptr(new Command("create-loadlib", "create loadlib data set using defaults: DSORG=PO, RECFM=U, LRECL=0"));
  ds_create_loadlib_cmd->add_alias("cre-u");
  ds_create_loadlib_cmd->add_positional_arg(DSN);
  ds_create_loadlib_cmd->set_handler(handle_data_set_create_loadlib);
  data_set_cmd->add_command(ds_create_loadlib_cmd);

  // Create-member subcommand
  auto ds_create_member_cmd = command_ptr(new Command("create-member", "create member in data set"));
  ds_create_member_cmd->add_alias("cre-m");
  ds_create_member_cmd->add_positional_arg("dsn", "data set name with member specified", ArgType_Single, true);
  ds_create_member_cmd->set_handler(handle_data_set_create_member);
  data_set_cmd->add_command(ds_create_member_cmd);

  // View subcommand
  auto ds_view_cmd = command_ptr(new Command("view", "view data set"));
  ds_view_cmd->add_positional_arg(DSN);
  ds_view_cmd->add_keyword_arg(ENCODING);
  ds_view_cmd->add_keyword_arg(LOCAL_ENCODING);
  ds_view_cmd->add_keyword_arg(RESPONSE_FORMAT_BYTES);
  ds_view_cmd->add_keyword_arg(RETURN_ETAG);
  ds_view_cmd->add_keyword_arg(PIPE_PATH);
  ds_view_cmd->add_keyword_arg(VOLSER);
  ds_view_cmd->set_handler(handle_data_set_view);
  data_set_cmd->add_command(ds_view_cmd);

  // List subcommand
  auto ds_list_cmd = command_ptr(new Command("list", "list data sets"));
  ds_list_cmd->add_alias("ls");
  ds_list_cmd->add_positional_arg(DSN_PATTERN);
  ds_list_cmd->add_keyword_arg("attributes", make_aliases("--attributes", "-a"), "display data set attributes", ArgType_Flag, false, ArgValue(false));
  ds_list_cmd->add_keyword_arg(MAX_ENTRIES);
  ds_list_cmd->add_keyword_arg(WARN);
  ds_list_cmd->add_keyword_arg(RESPONSE_FORMAT_CSV);
  ds_list_cmd->set_handler(handle_data_set_list);
  ds_list_cmd->add_example("List SYS1.* with all attributes", "zowex ds ls 'sys1.*' -a");
  data_set_cmd->add_command(ds_list_cmd);

  // List-members subcommand
  auto ds_list_members_cmd = command_ptr(new Command("list-members", "list data set members"));
  ds_list_members_cmd->add_alias("lm");
  ds_list_members_cmd->add_positional_arg(DSN);
  ds_list_members_cmd->add_keyword_arg("attributes", make_aliases("--attributes", "-a"), "display data set attributes", ArgType_Flag, false, ArgValue(false));
  ds_list_members_cmd->add_keyword_arg(MAX_ENTRIES);
  ds_list_members_cmd->add_keyword_arg(
      "pattern",
      make_aliases("--pattern", "-p"),
      "filters results by the given member pattern",
      ArgType_Single,
      false,
      ArgValue());
  ds_list_members_cmd->add_keyword_arg(WARN);
  ds_list_members_cmd->add_keyword_arg(RESPONSE_FORMAT_CSV);
  ds_list_members_cmd->set_handler(handle_data_set_list_members);
  data_set_cmd->add_command(ds_list_members_cmd);

  // Write subcommand
  auto ds_write_cmd = command_ptr(new Command("write", "write to data set"));
  ds_write_cmd->add_positional_arg(DSN);
  ds_write_cmd->add_keyword_arg(ENCODING);
  ds_write_cmd->add_keyword_arg(LOCAL_ENCODING);
  ds_write_cmd->add_keyword_arg(ETAG);
  ds_write_cmd->add_keyword_arg(ETAG_ONLY);
  ds_write_cmd->add_keyword_arg(PIPE_PATH);
  ds_write_cmd->add_keyword_arg(VOLSER);
  ds_write_cmd->set_handler(handle_data_set_write);
  data_set_cmd->add_command(ds_write_cmd);

  // Delete subcommand
  auto ds_delete_cmd = command_ptr(new Command("delete", "delete data set"));
  ds_delete_cmd->add_alias("del");
  ds_delete_cmd->add_positional_arg(DSN);
  ds_delete_cmd->set_handler(handle_data_set_delete);
  data_set_cmd->add_command(ds_delete_cmd);

  // Restore subcommand
  auto ds_restore_cmd = command_ptr(new Command("restore", "restore/recall data set"));
  ds_restore_cmd->add_positional_arg(DSN);
  ds_restore_cmd->set_handler(handle_data_set_restore);
  data_set_cmd->add_command(ds_restore_cmd);

  // Rename subcommand
  auto ds_rename_cmd = command_ptr(new Command("rename", "rename data set"));
  ds_rename_cmd->add_alias("ren");
  ds_rename_cmd->add_positional_arg("dsname-before", "data set to rename", ArgType_Single, true);
  ds_rename_cmd->add_positional_arg("dsname-after", "new data set name", ArgType_Single, true);
  ds_rename_cmd->set_handler(handle_data_set_rename);
  data_set_cmd->add_command(ds_rename_cmd);

  // Rename member subcommand
  auto ds_rename_members_cmd = command_ptr(new Command("rename-member", "rename a member"));
  ds_rename_members_cmd->add_alias("ren-m");
  ds_rename_members_cmd->add_positional_arg(DSN);
  ds_rename_members_cmd->add_positional_arg("member-before", "member to rename", ArgType_Single, true);
  ds_rename_members_cmd->add_positional_arg("member-after", "new member name", ArgType_Single, true);
  ds_rename_members_cmd->set_handler(handle_rename_member);
  data_set_cmd->add_command(ds_rename_members_cmd);

  // Compress subcommand
  auto ds_compress_cmd = command_ptr(new Command("compress", "compress data set"));
  ds_compress_cmd->add_positional_arg("dsn", "data set to compress", ArgType_Single, true);
  ds_compress_cmd->set_handler(handle_data_set_compress);
  data_set_cmd->add_command(ds_compress_cmd);

  // Copy subcommand
  auto ds_copy_cmd = command_ptr(new Command("copy", "copy data set (RECFM=U not supported)"));
  ds_copy_cmd->add_positional_arg("source", "source data set to copy from", ArgType_Single, true);
  ds_copy_cmd->add_positional_arg("target", "target data set to copy to", ArgType_Single, true);
  ds_copy_cmd->add_keyword_arg("replace", make_aliases("--replace", "-r"),
                               "replace matching members in target PDS with source members (keeps non-matching target members)",
                               ArgType_Flag, false, ArgValue(false));
  ds_copy_cmd->add_keyword_arg("delete-target-members", make_aliases("--delete-target-members", "-d"),
                               "delete all members from target PDS before copying (PDS-to-PDS copy only, makes target match source exactly)",
                               ArgType_Flag, false, ArgValue(false));
  ds_copy_cmd->set_handler(handle_data_set_copy);
  data_set_cmd->add_command(ds_copy_cmd);

  root_command.add_command(data_set_cmd);
}
} // namespace ds

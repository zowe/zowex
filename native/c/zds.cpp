
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

#ifndef _OPEN_SYS_ITOA_EXT
#define _OPEN_SYS_ITOA_EXT
#include "zdbg.h"
#include "ztype.h"
#include <cctype>
#endif
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include <algorithm>
#include "zds.hpp"
#include "zdyn.h"
#include "zdstype.h"
#include "zut.hpp"
#include "iefzb4d2.h"
#include "zdsm.h"
#include <fcntl.h>
#include "zbase64.h"
#include "zdsm.h"
#include "zamtypes.h"

const size_t MAX_DS_LENGTH = 44u;
const size_t MAX_VOLSER_LENGTH = 6u;
// carriage return character, used for detecting CRLF line endings
const char CR_CHAR = '\x0D';
// form feed character, used for stripping off lines in ASA mode
const char FF_CHAR = '\x0C';

/**
 * Helper function to check if codepage encoding should be used.
 * Returns true if text mode is enabled and a codepage is specified.
 */
static inline bool zds_use_codepage(const ZDS *zds)
{
  return zds->encoding_opts.data_type == eDataTypeText && std::strlen(zds->encoding_opts.codepage) > 0;
}

/**
 * Get the effective LRECL for writing, accounting for RDW in variable records
 * and ASA control character in ASA-formatted data sets.
 */
static int get_effective_lrecl(IO_CTRL *ioc)
{
  int lrecl = ioc->dcb.dcblrecl;
  // For variable records, subtract RDW size (4 bytes)
  if (ioc->dcb.dcbrecfm & dcbrecv)
  {
    lrecl -= 4;
  }
  // For ASA control characters, subtract 1 byte for the control character
  if (ioc->dcb.dcbrecfm & dcbrecca)
  {
    lrecl -= 1;
  }
  return lrecl;
}

// https://www.ibm.com/docs/en/zos/2.5.0?topic=functions-fldata-retrieve-file-information#fldata__fldat
std::string zds_get_recfm(const fldata_t &file_info)
{
  std::string recfm = ZDS_RECFM_U;

  if (file_info.__recfmF)
  {
    recfm = ZDS_RECFM_F;
    if (file_info.__recfmBlk || file_info.__recfmB)
      recfm = file_info.__recfmS ? ZDS_RECFM_FBS : ZDS_RECFM_FB;
    if (file_info.__recfmASA)
      recfm += "A";
    if (file_info.__recfmM)
      recfm += "M";
  }
  else if (file_info.__recfmV)
  {
    recfm = ZDS_RECFM_V;
    if (file_info.__recfmBlk || file_info.__recfmB)
      recfm = file_info.__recfmS ? ZDS_RECFM_VBS : ZDS_RECFM_VB;
    if (file_info.__recfmASA)
      recfm += "A";
    if (file_info.__recfmM)
      recfm += "M";
  }
  else if (file_info.__recfmU)
  {
    recfm = ZDS_RECFM_U;
  }

  return recfm;
}

// Internal enum for data set type classification (used only by copy functions)
enum ZDS_TYPE
{
  ZDS_TYPE_UNKNOWN = 0,
  ZDS_TYPE_PS,     // Sequential
  ZDS_TYPE_PDS,    // Partitioned (including PDSE)
  ZDS_TYPE_MEMBER, // Member of a PDS
  ZDS_TYPE_VSAM    // VSAM
};

// Internal struct for data set type information (used only by copy functions)
struct ZDSTypeInfo
{
  bool exists;
  ZDS_TYPE type;
  std::string base_dsn;
  std::string member_name;
  ZDSEntry entry; // Basic attributes if it exists
};

static std::vector<std::string> get_member_names(const std::string &pds_dsn)
{
  std::vector<std::string> names;
  std::vector<ZDSMem> members;
  ZDS temp_zds{};
  zds_list_members(&temp_zds, pds_dsn, members);
  for (const auto &mem : members)
  {
    std::string name = mem.name;
    zut_trim(name);
    names.push_back(name);
  }
  return names;
}

static bool member_exists_in_pds(const std::string &pds_dsn, const std::string &member_name)
{
  std::vector<std::string> names = get_member_names(pds_dsn);
  for (auto it = names.begin(); it != names.end(); ++it)
  {
    if (*it == member_name)
      return true;
  }
  return false;
}

static int zds_get_type_info(const std::string &dsn, ZDSTypeInfo &info)
{
  info.exists = false;
  info.type = ZDS_TYPE_UNKNOWN;
  info.base_dsn = dsn;
  info.member_name = "";

  size_t member_idx = dsn.find('(');
  if (member_idx != std::string::npos)
  {
    info.base_dsn = dsn.substr(0, member_idx);
    size_t end_idx = dsn.find(')', member_idx);
    if (end_idx != std::string::npos && end_idx > member_idx + 1)
    {
      info.member_name = dsn.substr(member_idx + 1, end_idx - member_idx - 1);
    }
  }

  zut_trim(info.base_dsn);
  std::transform(info.base_dsn.begin(), info.base_dsn.end(), info.base_dsn.begin(), ::toupper);
  zut_trim(info.member_name);
  std::transform(info.member_name.begin(), info.member_name.end(), info.member_name.begin(), ::toupper);

  if (!zds_dataset_exists(info.base_dsn))
  {
    return RTNCD_SUCCESS;
  }

  ZDS zds{};
  std::vector<ZDSEntry> entries;
  int rc = zds_list_data_sets(&zds, info.base_dsn, entries, true);
  if (rc == RTNCD_SUCCESS && entries.size() == 1)
  {
    ZDSEntry &entry = entries[0];
    info.exists = true;
    info.entry = entry;

    if (entry.dsorg == ZDS_DSORG_PS)
    {
      info.type = ZDS_TYPE_PS;
    }
    else if (entry.dsorg.rfind(ZDS_DSORG_PO, 0) == 0)
    {
      if (!info.member_name.empty())
      {
        info.type = ZDS_TYPE_MEMBER;
        // For members, verify the member actually exists in the PDS
        info.exists = member_exists_in_pds(info.base_dsn, info.member_name);
      }
      else
      {
        info.type = ZDS_TYPE_PDS;
      }
    }
    else if (entry.dsorg == ZDS_DSORG_VSAM)
    {
      info.type = ZDS_TYPE_VSAM;
    }
  }

  return RTNCD_SUCCESS;
}

static int copy_sequential(ZDS *zds, const std::string &src_dsn, const std::string &dst_dsn);
static int zds_write_sequential_streamed(ZDS *zds, const std::string &dsn, const std::string &pipe, size_t *content_len, const DscbAttributes &attrs);
static int zds_write_member_bpam_streamed(ZDS *zds, const std::string &dsn, const std::string &pipe, size_t *content_len);

// PDS-to-PDS copy using member-by-member binary I/O.
// This approach provides granular control for --replace semantics (skip/overwrite individual members)
// and naturally supports --delete-target-members workflow.
static int copy_pds_to_pds(ZDS *zds, const ZDSTypeInfo &src, const ZDSTypeInfo &dst, bool replace)
{
  std::vector<std::string> src_members = get_member_names(src.base_dsn);
  for (size_t i = 0; i < src_members.size(); i++)
  {
    const std::string &member = src_members[i];
    if (!replace && member_exists_in_pds(dst.base_dsn, member))
      continue;
    std::string src_mem_dsn = src.base_dsn + "(" + member + ")";
    std::string dst_mem_dsn = dst.base_dsn + "(" + member + ")";
    int rc = copy_sequential(zds, src_mem_dsn, dst_mem_dsn);
    if (rc != RTNCD_SUCCESS)
      return rc;
  }
  return RTNCD_SUCCESS;
}

// Copy sequential data set or member using binary I/O
// Note: RECFM=U data sets are not explicitly checked here because fldata() returns
// unreliable RECFM information when files are opened in binary mode. The write
// operation will fail naturally if the target is truly RECFM=U.
static int copy_sequential(ZDS *zds, const std::string &src_dsn, const std::string &dst_dsn)
{
  std::string src_path = "//'" + src_dsn + "'";
  std::string dst_path = "//'" + dst_dsn + "'";

  FILE *fin = fopen(src_path.c_str(), "rb");
  if (!fin)
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Could not open source '%s'", src_dsn.c_str());
    return RTNCD_FAILURE;
  }

  FILE *fout = fopen(dst_path.c_str(), "wb");
  if (!fout)
  {
    fclose(fin);
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Could not open target '%s'", dst_dsn.c_str());
    return RTNCD_FAILURE;
  }

  char buffer[32760];
  size_t bytes_read;
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), fin)) > 0)
  {
    size_t bytes_written = fwrite(buffer, 1, bytes_read, fout);
    if (bytes_written != bytes_read)
    {
      fclose(fin);
      fclose(fout);
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Write error copying to '%s'", dst_dsn.c_str());
      return RTNCD_FAILURE;
    }
  }

  fclose(fin);
  fclose(fout);
  return RTNCD_SUCCESS;
}

// Delete all members from a PDS
static int delete_all_members(ZDS *zds, const std::string &pds_dsn)
{
  std::vector<std::string> members = get_member_names(pds_dsn);
  for (size_t i = 0; i < members.size(); i++)
  {
    std::string member_dsn = pds_dsn + "(" + members[i] + ")";
    int rc = zds_delete_dsn(zds, member_dsn);
    if (rc != RTNCD_SUCCESS)
    {
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Failed to delete member '%s'", member_dsn.c_str());
      return RTNCD_FAILURE;
    }
  }
  return RTNCD_SUCCESS;
}

int zds_copy_dsn(ZDS *zds, const std::string &dsn1, const std::string &dsn2, ZDSCopyOptions *options)
{
  int rc = 0;
  ZDSCopyOptions default_options;
  ZDSCopyOptions *opts = options ? options : &default_options;
  opts->target_created = false;
  opts->member_created = false;
  ZDSTypeInfo info1{};
  ZDSTypeInfo info2{};

  zds_get_type_info(dsn1, info1);
  zds_get_type_info(dsn2, info2);

  if (!info1.exists)
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Source data set '%s' not found", info1.base_dsn.c_str());
    return RTNCD_FAILURE;
  }

  // PDS -> Member is not allowed (can't copy entire PDS into a single member)
  if (info1.type == ZDS_TYPE_PDS && !info2.member_name.empty())
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg,
                                  "Cannot copy entire PDS to a member. "
                                  "Target must be a PDS.");
    return RTNCD_FAILURE;
  }

  // Member -> PS is not supported (cross-type copy)
  if (info1.type == ZDS_TYPE_MEMBER && info2.member_name.empty())
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg,
                                  "Cannot copy PDS member to a sequential data set. "
                                  "Target must specify a member name.");
    return RTNCD_FAILURE;
  }

  // PS -> Member is not supported (cross-type copy)
  if (info1.type == ZDS_TYPE_PS && !info2.member_name.empty())
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg,
                                  "Cannot copy sequential data set to a PDS member. "
                                  "Target must be a sequential data set.");
    return RTNCD_FAILURE;
  }

  // PDS -> PS is not supported (cannot copy PDS to existing sequential data set)
  if (info1.type == ZDS_TYPE_PDS && info2.type == ZDS_TYPE_PS)
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg,
                                  "Cannot copy PDS to a sequential data set. "
                                  "Target must be a PDS.");
    return RTNCD_FAILURE;
  }

  bool is_pds_full_copy = (info1.type == ZDS_TYPE_PDS && info2.member_name.empty());
  bool target_is_member = !info2.member_name.empty();
  bool target_base_exists = zds_dataset_exists(info2.base_dsn);

  if (!target_base_exists)
  {
    // Create the target data set
    ZDS create_zds{};
    std::string create_resp;
    DS_ATTRIBUTES attrs{};

    // Common attributes for all data set types
    attrs.recfm = info1.entry.recfm.c_str();
    attrs.lrecl = info1.entry.lrecl;
    attrs.blksize = info1.entry.blksize;

    // Preserve space unit (CYLINDERS or TRACKS)
    if (info1.entry.spacu == "CYLINDERS" || info1.entry.spacu == "CYL")
    {
      attrs.alcunit = "CYL";
    }
    else
    {
      attrs.alcunit = "TRACKS";
    }

    // Preserve primary/secondary allocation
    if (info1.entry.primary >= 0 && info1.entry.secondary >= 0)
    {
      attrs.primary = info1.entry.primary > INT_MAX ? INT_MAX : static_cast<int>(info1.entry.primary);
      attrs.secondary = info1.entry.secondary > INT_MAX ? INT_MAX : static_cast<int>(info1.entry.secondary);
    }
    else
    {
      attrs.primary = 1;
      attrs.secondary = 1;
    }

    if (info1.type == ZDS_TYPE_PDS || info1.type == ZDS_TYPE_MEMBER)
    {
      // PDS -> PDS or Member -> Member: create PDS with source attributes
      attrs.dsorg = "PO";
      attrs.dirblk = 5;
      attrs.dsntype = info1.entry.dsntype.c_str();
    }
    else
    {
      // PS -> PS: create sequential data set with source attributes
      attrs.dsorg = "PS";
    }

    rc = zds_create_dsn(&create_zds, info2.base_dsn, attrs, create_resp);
    if (rc != RTNCD_SUCCESS)
    {
      // Truncate detail message to avoid buffer overflow (leave room for prefix + dsn + ": ")
      const char *detail = create_zds.diag.e_msg_len > 0 ? create_zds.diag.e_msg : create_resp.c_str();
      char truncated_detail[128];
      strncpy(truncated_detail, detail, sizeof(truncated_detail) - 1);
      truncated_detail[sizeof(truncated_detail) - 1] = '\0';
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Failed to create target data set '%s': %s",
                                    info2.base_dsn.c_str(), truncated_detail);
      return RTNCD_FAILURE;
    }
    opts->target_created = true;
  }
  // Track if target member exists (for member_created reporting)
  bool target_member_exists = target_is_member && member_exists_in_pds(info2.base_dsn, info2.member_name);

  if (!opts->replace && !is_pds_full_copy)
  {
    // For member targets, check if the specific member exists
    // For non-member targets (sequential DS), check if the base data set existed before this call
    bool target_actually_exists = target_is_member ? target_member_exists : target_base_exists;

    if (target_actually_exists)
    {
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg,
                                    "Target '%s' already exists. Use --replace to overwrite.",
                                    dsn2.c_str());
      return RTNCD_FAILURE;
    }
  }

  // Check if source and target are in the same PDS - need special handling
  bool same_pds = (info1.type == ZDS_TYPE_MEMBER && target_is_member &&
                   info1.base_dsn == info2.base_dsn);

  // Delete all target members if requested (only for PDS-to-PDS copy)
  if (opts->delete_target_members && is_pds_full_copy && target_base_exists)
  {
    rc = delete_all_members(zds, info2.base_dsn);
    if (rc != RTNCD_SUCCESS)
    {
      return rc;
    }
  }

  if (is_pds_full_copy)
  {
    // When delete_target_members is used, always replace since target is now empty
    return copy_pds_to_pds(zds, info1, info2, opts->replace || opts->delete_target_members);
  }
  else
  {
    if (same_pds)
    {
      // Same PDS: copy member to member using binary I/O
      std::string src_mem_dsn = info1.base_dsn + "(" + info1.member_name + ")";
      std::string dst_mem_dsn = info2.base_dsn + "(" + info2.member_name + ")";
      rc = copy_sequential(zds, src_mem_dsn, dst_mem_dsn);
    }
    else
    {
      rc = copy_sequential(zds, dsn1, dsn2);
    }

    // Report if a new member was created
    if (rc == RTNCD_SUCCESS && target_is_member && !target_member_exists)
    {
      opts->member_created = true;
    }
    return rc;
  }
}

bool zds_dataset_exists(const std::string &dsn)
{
  const auto member_idx = dsn.find('(');
  const std::string dsn_without_member = member_idx == std::string::npos ? dsn : dsn.substr(0, member_idx);
  FILE *fp = fopen(("//'" + dsn_without_member + "'").c_str(), "r");
  if (fp)
  {
    fclose(fp);
    return true;
  }
  return false;
}

bool zds_member_exists(const std::string &dsn, const std::string &member_before)
{
  std::string mem_before = "//'" + dsn + "(" + member_before + ")'";
  FILE *fp = fopen(mem_before.c_str(), "r");
  if (fp)
  {
    fclose(fp);
    return true;
  }
  return false;
}

bool zds_is_valid_member_name(const std::string &name)
{
  if (name.length() > 8)
    return false;

  char first = name[0];
  if (!(isalpha(first) || first == '#' || first == '@' || first == '$'))
    return false;

  for (size_t i = 1; i < name.length(); ++i)
  {
    char c = name[i];
    if (!(isalnum(c) || c == '#' || c == '@' || c == '$'))
      return false;
  }

  return true;
}

/**
 * Helper function to get data set attributes (RECFM, LRECL) from DSCB.
 * Returns a DscbAttributes struct with recfm, lrecl, and is_asa flag.
 */
static DscbAttributes zds_get_dscb_attributes(const std::string &dsn)
{
  DscbAttributes attrs;
  const std::string base_dsn = dsn.find('(') != std::string::npos ? dsn.substr(0, dsn.find('(')) : dsn;

  ZDS zds{};
  std::vector<ZDSEntry> datasets;

  // Use catalog lookup with DSCB to get the actual attributes
  int rc = zds_list_data_sets(&zds, base_dsn, datasets, true);
  if (rc != RTNCD_SUCCESS || datasets.empty())
  {
    return attrs;
  }

  attrs.recfm = datasets[0].recfm;
  attrs.lrecl = datasets[0].lrecl;
  attrs.is_asa = attrs.recfm.find('A') != std::string::npos;

  return attrs;
}

/**
 * Get the effective LRECL for writing from DSCB attributes, accounting for RDW
 * in variable records and ASA control character in ASA-formatted data sets.
 */
static int get_effective_lrecl_from_attrs(const DscbAttributes &attrs)
{
  int lrecl = attrs.lrecl;
  // For variable records, subtract RDW size (4 bytes)
  if (attrs.recfm.find('V') != std::string::npos)
  {
    lrecl -= 4;
  }
  // For ASA control characters, subtract 1 byte
  if (attrs.is_asa)
  {
    lrecl -= 1;
  }
  return lrecl;
}

/**
 * Scan data for lines that exceed the maximum record length.
 * Populates the TruncationTracker with line numbers of truncated lines.
 *
 * @param data The data to scan
 * @param max_len The maximum allowed line length (effective LRECL)
 * @param newline_char The newline character to use for line splitting
 * @param truncation The TruncationTracker to populate
 */
static void scan_for_truncated_lines(const std::string &data, int max_len,
                                     char newline_char, TruncationTracker &truncation)
{
  int line_num = 0;
  size_t pos = 0;
  size_t newline_pos;

  while ((newline_pos = data.find(newline_char, pos)) != std::string::npos)
  {
    line_num++;
    size_t line_len = newline_pos - pos;

    // Account for CR if present (Windows line endings)
    if (line_len > 0 && data[newline_pos - 1] == CR_CHAR)
    {
      line_len--;
    }

    if (static_cast<int>(line_len) > max_len)
    {
      truncation.add_line(line_num);
    }
    pos = newline_pos + 1;
  }

  // Check final line (no trailing newline)
  if (pos < data.length())
  {
    line_num++;
    size_t line_len = data.length() - pos;
    if (line_len > 0 && data[data.length() - 1] == CR_CHAR)
    {
      line_len--;
    }
    if (static_cast<int>(line_len) > max_len)
    {
      truncation.add_line(line_num);
    }
  }

  truncation.flush_range();
}

static std::string zds_resolve_dsname(const ZDSReadOpts &opts)
{
  if (!opts.ddname.empty())
  {
    return "//DD:" + opts.ddname;
  }
  return "//'" + opts.dsname + "'";
}

static DscbAttributes zds_resolve_dscb(const ZDSReadOpts &opts)
{
  return opts.dsname.empty() ? DscbAttributes{} : zds_get_dscb_attributes(opts.dsname);
}

static std::string zds_resolve_write_target(const ZDSWriteOpts &opts)
{
  if (!opts.ddname.empty())
  {
    return "DD:" + opts.ddname;
  }
  return "//'" + opts.dsname + "'";
}

static DscbAttributes zds_resolve_write_dscb(const ZDSWriteOpts &opts)
{
  return opts.dsname.empty() ? DscbAttributes{} : zds_get_dscb_attributes(opts.dsname);
}

/**
 * Validates the write options and returns an error code if the options are invalid.
 *
 * @param opts write options to validate
 * @param caller name of the caller function
 * @throw std::invalid_argument if the opts.zds pointer is nullptr
 * @return RTNCD_SUCCESS if the options are valid, RTNCD_FAILURE otherwise
 */
static int zds_validate_write_opts(const ZDSWriteOpts &opts, const char *caller)
{
  if (opts.zds == nullptr)
  {
    throw std::invalid_argument(std::string(caller) + ": valid ZDS pointer is required in ZDSWriteOpts.zds");
  }
  if (opts.dsname.empty() && opts.ddname.empty())
  {
    opts.zds->diag.e_msg_len = sprintf(opts.zds->diag.e_msg, "Either a dsname or ddname must be provided");
    return RTNCD_FAILURE;
  }
  return RTNCD_SUCCESS;
}

/**
 * Validates the read options and returns an error code if the options are invalid.
 *
 * @param opts read options to validate
 * @param caller name of the caller function
 * @throw std::invalid_argument if the opts.zds pointer is nullptr
 * @return RTNCD_SUCCESS if the options are valid, RTNCD_FAILURE otherwise
 */
static int zds_validate_read_opts(const ZDSReadOpts &opts, const char *caller)
{
  if (opts.zds == nullptr)
  {
    throw std::invalid_argument(std::string(caller) + ": valid ZDS pointer is required in ZDSReadOpts.zds");
  }
  if (opts.dsname.empty() && opts.ddname.empty())
  {
    opts.zds->diag.e_msg_len = sprintf(opts.zds->diag.e_msg, "Either a dsname or ddname must be provided");
    return RTNCD_FAILURE;
  }
  return RTNCD_SUCCESS;
}

int zds_read(const ZDSReadOpts &opts, std::string &response)
{
  const int vrc = zds_validate_read_opts(opts, "zds_read");
  if (vrc != RTNCD_SUCCESS)
  {
    return vrc;
  }

  ZDS *zds = opts.zds;
  const std::string dsname = zds_resolve_dsname(opts);
  const auto is_dd = !opts.ddname.empty();

  // Check if ASA format - use record I/O to preserve ASA control characters
  const DscbAttributes attrs = zds_resolve_dscb(opts);
  const bool is_asa = attrs.is_asa && zds->encoding_opts.data_type != eDataTypeBinary;

  std::string fopen_flags;
  if (zds->encoding_opts.data_type == eDataTypeBinary)
  {
    fopen_flags = "rb";
  }
  else if (is_asa)
  {
    // Use record I/O to preserve ASA control characters (prevents runtime interpretation)
    fopen_flags = "rb,type=record";
  }
  else
  {
    fopen_flags = "r";
  }

  FileGuard fp(dsname.c_str(), fopen_flags.c_str());
  if (!fp)
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Could not open handle to %s '%s'", is_dd ? "DD" : "data set", dsname.c_str());
    return RTNCD_FAILURE;
  }

  size_t bytes_read = 0;
  size_t total_size = 0;

  if (is_asa)
  {
    // For ASA, read record by record and append newlines between records
    // This preserves the ASA control character as the first byte of each line
    const int lrecl = attrs.lrecl > 0 ? attrs.lrecl : 32760;
    std::vector<char> buffer(lrecl);
    bool first_record = true;

    while ((bytes_read = fread(&buffer[0], 1, lrecl, fp)) > 0)
    {
      // Add newline before each record (except the first)
      if (!first_record)
      {
        response.append(1, '\n');
        total_size += 1;
      }
      first_record = false;

      // Trim trailing spaces from the record (fixed-length records are padded)
      size_t actual_len = bytes_read;
      while (actual_len > 0 && buffer[actual_len - 1] == ' ')
      {
        actual_len--;
      }

      total_size += actual_len;
      response.append(&buffer[0], actual_len);
    }
    // Add trailing newline if we read any records
    if (!first_record)
    {
      response.append(1, '\n');
      total_size += 1;
    }
  }
  else
  {
    // Non-ASA: read in chunks as before
    char buffer[4096] = {};
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
      total_size += bytes_read;
      response.append(buffer, bytes_read);
    }
  }

  const auto encoding_provided = zds_use_codepage(zds);

  if (total_size > 0 && encoding_provided)
  {
    std::string temp = response;
    const auto source_encoding = std::strlen(zds->encoding_opts.source_codepage) > 0 ? std::string(zds->encoding_opts.source_codepage) : "UTF-8";
    try
    {
      const auto bytes_with_encoding = zut_encode(temp, std::string(zds->encoding_opts.codepage), source_encoding, zds->diag);
      temp = bytes_with_encoding;
    }
    catch (std::exception &e)
    {
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Failed to convert input data from %s to %s", source_encoding.c_str(), zds->encoding_opts.codepage);
      return RTNCD_FAILURE;
    }
    if (!temp.empty())
    {
      response = temp;
    }
  }

  return 0;
}

int zds_read_vsam(ZDS *zds, std::string ddname, std::string &response)
{
  int rc = 0;

  IO_CTRL *ioc = new IO_CTRL();

  rc = ZDSOIVSM(zds, &ioc, ddname.c_str());
  if (rc != RTNCD_SUCCESS)
  {
    return rc;
  }

  rc = ZDSPIVSM(zds, ioc);
  if (rc != RTNCD_SUCCESS)
  {
    ZDSCIVSM(zds, ioc);
    return rc;
  }

  int lines_read = 0;
  while (true)
  {
    rc = ZDSRIVSM(zds, ioc);
    if (rc == RTNCD_SUCCESS)
    {
      response.append(ioc->buffer, ioc->ifgacb.acblrecl);
      response.append(1, '\n');
      lines_read++;
    }
    else if (rc == RTNCD_WARNING)
    {
      break;
    }
    else
    {
      ZDSCIVSM(zds, ioc);
      return rc;
    }
    if (lines_read >= zds->max_lines)
    {
      break;
    }
  }

  rc = ZDSCIVSM(zds, ioc);
  if (rc != RTNCD_SUCCESS)
  {
    return rc;
  }

  return RTNCD_SUCCESS;
}

/**
 * Determine the newline character for a given encoding by converting '\n' from IBM-1047.
 * This uses zut_encode to handle the conversion properly for any encoding.
 * If encoding is empty, returns the native IBM-1047 newline character.
 */
static char get_newline_char(const std::string &encoding, ZDIAG &diag)
{
  // If no encoding specified, return native IBM-1047 newline
  if (encoding.empty())
  {
    return '\n'; // Native newline in IBM-1047
  }

  try
  {
    const std::string result = zut_encode("\n", "IBM-1047", encoding, diag);
    if (!result.empty())
    {
      return result[0];
    }
  }
  catch (...)
  {
    // Fall through to default
  }

  // Fallback: return native newline
  return '\n';
}

/**
 * Encoding setup context for write operations
 */
struct EncodingSetup
{
  bool has_encoding;
  std::string codepage;
  std::string source_encoding;
  char newline_char;

  EncodingSetup()
      : has_encoding(false), newline_char('\n')
  {
  }
};

/**
 * Initialize encoding context for write operations.
 * Consolidates encoding setup logic used across all write functions.
 *
 * @param zds ZDS context containing encoding options
 * @param setup EncodingSetup struct to populate
 * @return RTNCD_SUCCESS on success, RTNCD_FAILURE if iconv setup fails
 */
static int setup_encoding(ZDS *zds, EncodingSetup &setup)
{
  setup.has_encoding = zds_use_codepage(zds);
  setup.codepage = std::string(zds->encoding_opts.codepage);
  setup.source_encoding = std::strlen(zds->encoding_opts.source_codepage) > 0
                              ? std::string(zds->encoding_opts.source_codepage)
                              : "UTF-8";

  setup.newline_char = get_newline_char(setup.has_encoding ? setup.source_encoding : "", zds->diag);
  return RTNCD_SUCCESS;
}

/**
 * Validate write options and resolve target/attributes.
 * Consolidates validation and resolution logic used in write entry points.
 *
 * @param opts Write options to validate and resolve
 * @param caller Name of calling function for error messages
 * @param target Output parameter for resolved target string
 * @param attrs Output parameter for resolved DSCB attributes
 * @return RTNCD_SUCCESS on success, RTNCD_FAILURE on validation failure
 */
static int validate_and_resolve_write_opts(const ZDSWriteOpts &opts, const char *caller,
                                           std::string &target, DscbAttributes &attrs)
{
  const int vrc = zds_validate_write_opts(opts, caller);
  if (vrc != RTNCD_SUCCESS)
  {
    return vrc;
  }

  target = zds_resolve_write_target(opts);
  attrs = zds_resolve_write_dscb(opts);
  return RTNCD_SUCCESS;
}

/**
 * Handle truncation result and return appropriate code.
 * Consolidates truncation warning logic used across write functions.
 *
 * @param zds ZDS context for diagnostic messages
 * @param write_rc Return code from write operation
 * @param truncation Truncation tracker with line count and details
 * @return RTNCD_SUCCESS, RTNCD_WARNING, or original write_rc on failure
 */
static int handle_truncation_result(ZDS *zds, int write_rc, const TruncationTracker &truncation)
{
  if (write_rc != RTNCD_SUCCESS)
  {
    return write_rc;
  }

  if (truncation.count > 0)
  {
    const auto warning_msg = truncation.get_warning_message();
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "%s", warning_msg.c_str());
    zds->diag.detail_rc = ZDS_RSNCD_TRUNCATION_WARNING;
    return RTNCD_WARNING;
  }

  return RTNCD_SUCCESS;
}

/**
 * Initialize IconvGuard for encoding operations.
 * Consolidates IconvGuard setup and validation logic.
 * Note: IconvGuard must be created at the call site due to deleted assignment operator.
 *
 * @param setup Encoding setup context
 * @param diag Diagnostic context for error messages
 * @return RTNCD_SUCCESS on success, RTNCD_FAILURE on iconv setup failure
 */
static int validate_iconv_setup(const EncodingSetup &setup, const IconvGuard &iconv_guard, ZDIAG &diag)
{
  if (!setup.has_encoding)
  {
    return RTNCD_SUCCESS;
  }

  if (!iconv_guard.is_valid())
  {
    diag.e_msg_len = sprintf(diag.e_msg, "Cannot open converter from %s to %s",
                             setup.source_encoding.c_str(), setup.codepage.c_str());
    return RTNCD_FAILURE;
  }
  return RTNCD_SUCCESS;
}

/**
 * Encode a chunk of data using the provided encoding setup.
 * Consolidates chunk encoding logic used in streaming functions.
 *
 * @param chunk Pointer to chunk data (modified to point to encoded data)
 * @param chunk_len Length of chunk (modified to encoded length)
 * @param temp_encoded Buffer for encoded data (output parameter)
 * @param setup Encoding setup context
 * @param iconv_guard IconvGuard for encoding conversion
 * @param diag Diagnostic context for error messages
 * @return RTNCD_SUCCESS on success, RTNCD_FAILURE on encoding failure
 */
static int encode_chunk_if_needed(const char *&chunk, int &chunk_len, std::vector<char> &temp_encoded,
                                  const EncodingSetup &setup, IconvGuard &iconv_guard, ZDIAG &diag)
{
  if (!setup.has_encoding)
  {
    return RTNCD_SUCCESS;
  }

  try
  {
    temp_encoded = zut_encode(chunk, chunk_len, iconv_guard.get(), diag);
    chunk = &temp_encoded[0];
    chunk_len = temp_encoded.size();
    return RTNCD_SUCCESS;
  }
  catch (std::exception &e)
  {
    diag.e_msg_len = sprintf(diag.e_msg, "Failed to convert input data from %s to %s",
                             setup.source_encoding.c_str(), setup.codepage.c_str());
    return RTNCD_FAILURE;
  }
}

/**
 * Flush encoding state and handle flush buffer.
 * Consolidates encoding flush logic used across write functions.
 *
 * @param setup Encoding setup context
 * @param iconv_guard IconvGuard for encoding conversion
 * @param diag Diagnostic context for error messages
 * @param flush_buffer Output buffer for flush bytes
 * @return RTNCD_SUCCESS on success, RTNCD_FAILURE on flush failure
 */
static int flush_encoding_state(const EncodingSetup &setup, IconvGuard &iconv_guard, ZDIAG &diag, std::vector<char> &flush_buffer)
{
  if (!setup.has_encoding || !iconv_guard.is_valid())
  {
    return RTNCD_SUCCESS;
  }

  try
  {
    flush_buffer = zut_iconv_flush(iconv_guard.get(), diag);
    if (flush_buffer.empty() && diag.e_msg_len > 0)
    {
      return RTNCD_FAILURE;
    }
    return RTNCD_SUCCESS;
  }
  catch (std::exception &e)
  {
    diag.e_msg_len = sprintf(diag.e_msg, "Failed to flush encoding state");
    return RTNCD_FAILURE;
  }
}

/**
 * Encode a single line using the provided encoding setup.
 * Consolidates line encoding logic used in BPAM write functions.
 *
 * @param line Line to encode (modified in place)
 * @param setup Encoding setup context
 * @param iconv_guard IconvGuard for encoding conversion
 * @param diag Diagnostic context for error messages
 * @return RTNCD_SUCCESS on success, RTNCD_FAILURE on encoding failure
 */
static int encode_line_if_needed(std::string &line, const EncodingSetup &setup, IconvGuard &iconv_guard, ZDIAG &diag)
{
  if (!setup.has_encoding)
  {
    return RTNCD_SUCCESS;
  }

  try
  {
    line = zut_encode(line, iconv_guard.get(), diag);
    return RTNCD_SUCCESS;
  }
  catch (std::exception &e)
  {
    diag.e_msg_len = sprintf(diag.e_msg, "Failed to convert input data from %s to %s",
                             setup.source_encoding.c_str(), setup.codepage.c_str());
    return RTNCD_FAILURE;
  }
}

/**
 * Helper struct to track ASA conversion state across streaming chunks.
 * Tracks blank line counts to determine the appropriate ASA control character.
 */
struct AsaStreamState
{
  // ASA triple-space ('-') advances 3 lines, so we need 3 blank lines to trigger it
  static constexpr int ASA_TRIPLE_SPACE_LINES = 3;

  int blank_count;
  bool is_first_line;

  AsaStreamState()
      : blank_count(0), is_first_line(true)
  {
  }

private:
  /**
   * Check if there are overflow blank lines that need to be flushed as empty '-' records.
   * Call this AFTER process_line() returns non-'\0', BEFORE writing the actual line.
   * Each '-' record represents "advance 3 lines" and consumes 2 blank lines.
   * Returns true if an empty '-' record should be written.
   */
  bool has_overflow_blanks()
  {
    if (blank_count >= ASA_TRIPLE_SPACE_LINES)
    {
      blank_count -= ASA_TRIPLE_SPACE_LINES;
      return true;
    }
    return false;
  }

  /**
   * Get the ASA control character to prepend, and reset blank_count.
   * Call this AFTER flushing overflow blanks with has_overflow_blanks().
   * At this point blank_count should be 0, 1, or 2.
   * Returns the EBCDIC ASA character
   */
  char get_asa_char_and_reset(bool has_formfeed)
  {
    char asa_char = ' '; // EBCDIC 0x40: single-space (advance 1 line)
    if (has_formfeed)
    {
      asa_char = '1'; // EBCDIC 0xF1: form feed (advance to next page)
    }
    else if (blank_count == 1)
    {
      asa_char = '0'; // EBCDIC 0xF0: double-space (advance 2 lines)
    }
    else if (blank_count == 2)
    {
      asa_char = '-'; // EBCDIC 0x60: triple-space (advance 3 lines)
    }
    blank_count = 0;
    is_first_line = false;
    return asa_char;
  }

  /**
   * Process a line and check if it should be skipped (blank line).
   * For empty lines, returns '\0' to indicate the line should be skipped.
   * For non-empty lines, returns ' ' to indicate it should be written.
   * After this returns non-'\0', call has_overflow_blanks() then get_asa_char_and_reset().
   */
  char process_line_internal(std::string &line, bool &has_formfeed_out)
  {
    // Check for form feed at start of line
    has_formfeed_out = (!line.empty() && line[0] == FF_CHAR);
    if (has_formfeed_out)
    {
      line = line.substr(1); // remove form feed from content
    }

    // Empty lines (without form feed) increment blank count
    if (line.empty() && !has_formfeed_out)
    {
      blank_count++;
      return '\0'; // Signal to skip this line
    }

    // Return indicator that line should be written
    return has_formfeed_out ? '1' : ' ';
  }

public:
  /**
   * Result of preparing a line for ASA output.
   */
  struct PrepareResult
  {
    bool skip_line;       // If true, skip this line entirely (it's a blank that increments counter)
    int overflow_records; // Number of empty '-' records to write before this line
    char asa_char;        // ASA control character to prepend to the line (if !skip_line)
  };

  /**
   * Prepare a line for ASA output. This combines the process_line, has_overflow_blanks,
   * and get_asa_char_and_reset logic into a single call.
   *
   * @param line The line content (may be modified to strip formfeed character)
   * @return PrepareResult indicating whether to skip, overflow records to write, and ASA char
   */
  PrepareResult prepare_line(std::string &line)
  {
    PrepareResult result = {false, 0, '\0'};

    bool has_formfeed = false;
    char line_type = process_line_internal(line, has_formfeed);
    if (line_type == '\0')
    {
      result.skip_line = true;
      return result;
    }

    // Count overflow blank records (each '-' record consumes 3 blank lines)
    while (has_overflow_blanks())
    {
      result.overflow_records++;
    }

    result.asa_char = get_asa_char_and_reset(has_formfeed);
    return result;
  }
};

/**
 * Process trailing line content (without newline) for BPAM writes.
 * Handles CR stripping, ASA processing, encoding, and truncation tracking.
 * Returns RTNCD_SUCCESS on success or error code on failure.
 */
struct ProcessedLine
{
  std::string line;
  char asa_char;
  bool skip_line;
  int overflow_records;

  ProcessedLine()
      : asa_char('\0'), skip_line(false), overflow_records(0)
  {
  }
};

static int process_bpam_trailing_line(std::string &line_content, bool is_asa, AsaStreamState &asa_state,
                                      const EncodingSetup &encoding, IconvGuard &iconv_guard,
                                      int max_len, int &line_num, TruncationTracker &truncation,
                                      ProcessedLine &result, ZDIAG &diag)
{
  // Remove trailing carriage return if present
  if (!line_content.empty() && line_content[line_content.size() - 1] == CR_CHAR)
  {
    line_content.erase(line_content.size() - 1);
  }

  if (line_content.empty())
  {
    result.skip_line = true;
    return RTNCD_SUCCESS;
  }

  // Process ASA: determine control char
  result.asa_char = '\0';
  if (is_asa)
  {
    auto asa_result = asa_state.prepare_line(line_content);
    if (asa_result.skip_line)
    {
      result.skip_line = true;
      return RTNCD_SUCCESS;
    }

    result.overflow_records = asa_result.overflow_records;
    result.asa_char = asa_result.asa_char;
  }

  line_num++;

  // Encode line if needed
  int rc = encode_line_if_needed(line_content, encoding, iconv_guard, diag);
  if (rc != RTNCD_SUCCESS)
  {
    return rc;
  }

  // Check if line will be truncated (account for ASA char if present)
  if (static_cast<int>(line_content.length() + (is_asa && result.asa_char != '\0' ? 1 : 0)) > max_len)
  {
    truncation.add_line(line_num);
  }

  result.line = line_content;
  return RTNCD_SUCCESS;
}

/**
 * Helper function to check if a DSN contains a member name
 */
static bool zds_has_member(const std::string &dsn)
{
  const auto open_paren = dsn.find('(');
  const auto close_paren = dsn.find(')');
  return (open_paren != std::string::npos && close_paren != std::string::npos && close_paren > open_paren);
}

/**
 * Helper function to extract the base DSN (without member) from a fully qualified DSN
 */
static std::string zds_get_base_dsn(const std::string &dsn)
{
  const auto open_paren = dsn.find('(');
  if (open_paren != std::string::npos)
  {
    return dsn.substr(0, open_paren);
  }
  return dsn;
}

/**
 * Internal function to write to a sequential data set using fopen/fwrite
 */
static int zds_write_sequential(ZDS *zds, const std::string &dsn, const std::string &data, const DscbAttributes &attrs)
{
  EncodingSetup encoding;
  int rc = setup_encoding(zds, encoding);
  if (rc != RTNCD_SUCCESS)
  {
    return rc;
  }

  const bool isTextMode = zds->encoding_opts.data_type != eDataTypeBinary;
  std::string dsname = dsn;
  TruncationTracker truncation;
  // Build fopen flags with actual recfm from DSCB
  // Use text mode - the C runtime handles ASA control characters and record boundaries
  const std::string recfm_flag = attrs.recfm.empty() ? "*" : attrs.recfm;
  const std::string fopen_flags = zds->encoding_opts.data_type == eDataTypeBinary
                                      ? "wb"
                                      : "w,recfm=" + recfm_flag;

  {
    FileGuard fp(dsname.c_str(), fopen_flags.c_str());
    if (!fp)
    {
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Could not open dsn '%s'", dsn.c_str());
      return RTNCD_FAILURE;
    }

    if (!data.empty())
    {
      // Encode entire buffer and write - the C runtime handles ASA and record boundaries in text mode
      std::string temp = data;
      if (encoding.has_encoding)
      {
        try
        {
          temp = zut_encode(temp, encoding.source_encoding, encoding.codepage, zds->diag);
        }
        catch (std::exception &e)
        {
          zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Failed to convert input data from %s to %s", encoding.source_encoding.c_str(), encoding.codepage.c_str());
          return RTNCD_FAILURE;
        }
      }

      // Track truncated lines for text mode (post-encoding to handle multi-byte encodings correctly)
      if (isTextMode && attrs.lrecl > 0 && !temp.empty())
      {
        const int max_len = get_effective_lrecl_from_attrs(attrs);
        scan_for_truncated_lines(temp, max_len, encoding.newline_char, truncation);
      }

      size_t bytes_written = fwrite(temp.c_str(), 1u, temp.length(), fp);
      if (bytes_written != temp.length())
      {
        zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Failed to write all contents to '%s' (possibly out of space)", dsname.c_str());
        return RTNCD_FAILURE;
      }
    }
  }

  // Report truncation warning if lines were truncated and write succeeded
  return handle_truncation_result(zds, RTNCD_SUCCESS, truncation);
}

/**
 * Write ASA overflow records (empty '-' records) for handling 3+ consecutive blank lines.
 * Returns RTNCD_SUCCESS on success, or the error code on failure.
 */
static int write_asa_overflow_records(ZDS *zds, IO_CTRL *ioc, int overflow_count)
{
  for (int i = 0; i < overflow_count; i++)
  {
    std::string empty_record(1, '-');
    int rc = zds_write_output_bpam(zds, ioc, empty_record);
    if (rc != RTNCD_SUCCESS)
    {
      return rc;
    }
  }
  return RTNCD_SUCCESS;
}

/**
 * Internal function to write to a PDS/PDSE member using BPAM (updates ISPF stats)
 */
static int zds_write_member_bpam(ZDS *zds, const std::string &dsn, const std::string &data)
{
  int rc = 0;
  IO_CTRL *ioc = nullptr;

  // Open the member for BPAM output
  rc = zds_open_output_bpam(zds, dsn, ioc);
  if (rc != RTNCD_SUCCESS)
  {
    return rc;
  }

  // Check if ASA format from DCB after open
  const bool is_asa = (ioc->dcb.dcbrecfm & dcbrecca) != 0;

  EncodingSetup encoding;
  rc = setup_encoding(zds, encoding);
  if (rc != RTNCD_SUCCESS)
  {
    zds_close_output_bpam(zds, ioc);
    return rc;
  }

  // Track truncated lines
  TruncationTracker truncation;
  int line_num = 0;
  const int max_len = get_effective_lrecl(ioc);

  // ASA state tracking (only used when converting to ASA)
  AsaStreamState asa_state;

  // Open iconv descriptor once for all lines (for stateful encodings like IBM-939)
  IconvGuard iconv_guard(encoding.has_encoding ? encoding.codepage.c_str() : nullptr, encoding.has_encoding ? encoding.source_encoding.c_str() : nullptr);
  rc = validate_iconv_setup(encoding, iconv_guard, zds->diag);
  if (rc != RTNCD_SUCCESS)
  {
    zds_close_output_bpam(zds, ioc);
    return rc;
  }

  // Collect all lines first, then write them (so we can append flush bytes to the last line)
  std::vector<std::pair<std::string, char>> lines_to_write; // std::pair of (encoded line, asa_char)

  // Parse data line by line
  if (!data.empty())
  {
    size_t pos = 0;
    size_t newline_pos;

    while ((newline_pos = data.find(encoding.newline_char, pos)) != std::string::npos)
    {
      std::string line = data.substr(pos, newline_pos - pos);

      // Remove trailing CR if present (Windows line endings)
      if (!line.empty() && line[line.size() - 1] == CR_CHAR)
      {
        line.erase(line.size() - 1);
      }

      // Process ASA: determine control char, skip blank lines
      char asa_char = '\0';
      if (is_asa)
      {
        auto asa_result = asa_state.prepare_line(line);
        if (asa_result.skip_line)
        {
          pos = newline_pos + 1;
          continue;
        }

        // Add overflow blank lines as empty '-' records (for 3+ blank lines)
        for (int i = 0; i < asa_result.overflow_records; i++)
        {
          lines_to_write.emplace_back(std::make_pair(std::string(), '-'));
        }

        asa_char = asa_result.asa_char;
      }

      line_num++;

      // Encode line if needed
      rc = encode_line_if_needed(line, encoding, iconv_guard, zds->diag);
      if (rc != RTNCD_SUCCESS)
      {
        zds_close_output_bpam(zds, ioc);
        return rc;
      }

      // Check if line will be truncated (account for ASA char if present)
      if (static_cast<int>(line.length() + (is_asa ? 1 : 0)) > max_len)
      {
        truncation.add_line(line_num);
      }

      lines_to_write.emplace_back(std::make_pair(line, is_asa ? asa_char : '\0'));
      pos = newline_pos + 1;
    }

    // Handle remaining content after last newline
    if (pos < data.size())
    {
      std::string line = data.substr(pos);
      ProcessedLine result;

      rc = process_bpam_trailing_line(line, is_asa, asa_state, encoding, iconv_guard,
                                      max_len, line_num, truncation, result, zds->diag);
      if (rc != RTNCD_SUCCESS)
      {
        zds_close_output_bpam(zds, ioc);
        return rc;
      }

      if (!result.skip_line)
      {
        // Add overflow blank lines as empty '-' records
        for (int i = 0; i < result.overflow_records; i++)
        {
          lines_to_write.emplace_back(std::make_pair(std::string(), '-'));
        }

        lines_to_write.emplace_back(std::make_pair(result.line, result.asa_char));
      }
    }
  }

  // Flush encoding state and append to last line if needed
  std::vector<char> flush_buffer;
  rc = flush_encoding_state(encoding, iconv_guard, zds->diag, flush_buffer);
  if (rc != RTNCD_SUCCESS)
  {
    zds_close_output_bpam(zds, ioc);
    return rc;
  }

  // Append flush bytes to the last non-empty line
  if (!flush_buffer.empty() && !lines_to_write.empty())
  {
    for (auto it = lines_to_write.rbegin(); it != lines_to_write.rend(); ++it)
    {
      if (!it->first.empty())
      {
        it->first.append(flush_buffer.begin(), flush_buffer.end());
        break;
      }
    }
  }

  // Now write all the lines
  for (size_t i = 0; i < lines_to_write.size(); i++)
  {
    rc = zds_write_output_bpam(zds, ioc, lines_to_write[i].first, lines_to_write[i].second);
    if (rc != RTNCD_SUCCESS)
    {
      DiagMsgGuard guard(&zds->diag);
      zds_close_output_bpam(zds, ioc);
      return rc;
    }
  }

  // Finalize any pending range
  truncation.flush_range();

  // Close BPAM and handle truncation result
  rc = zds_close_output_bpam(zds, ioc);
  return handle_truncation_result(zds, rc, truncation);
}

/**
 * Checks if a record format is unsupported for writing.
 * @param recfm The record format to check.
 * @param include_standard Whether to include "S" record formats in the check (minor logic optimization for members as its only supported for sequential).
 * @return true if the record format is unsupported, false otherwise.
 */
static bool zds_write_recfm_unsupported(const std::string &recfm, const bool include_standard = false)
{
  return recfm == ZDS_RECFM_U || (include_standard && recfm.find(ZDS_RECFM_S) != std::string::npos);
}

int zds_validate_etag(ZDS *zds, const std::string &dsn, bool has_encoding)
{
  ZDS read_zds{};
  if (has_encoding)
  {
    memcpy(&read_zds.encoding_opts, &zds->encoding_opts, sizeof(ZEncode));
  }
  ZDSReadOpts read_opts{.zds = &read_zds, .dsname = dsn};
  std::string current_contents = "";
  const auto read_rc = zds_read(read_opts, current_contents);
  if (0 != read_rc)
  {
    char truncated_detail[128];
    strncpy(truncated_detail, read_zds.diag.e_msg, sizeof(truncated_detail) - 1);
    truncated_detail[sizeof(truncated_detail) - 1] = '\0';
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg,
                                  "Failed to read contents of data set for e-tag comparison: %s", truncated_detail);
    return RTNCD_FAILURE;
  }

  const auto given_etag = strtoul(zds->etag, nullptr, 16);
  const auto new_etag = zut_calc_adler32_checksum(current_contents);

  if (given_etag != new_etag)
  {
    std::ostringstream ss;
    ss << "Etag mismatch: expected ";
    ss << std::hex << given_etag << std::dec;
    ss << ", actual ";
    ss << std::hex << new_etag << std::dec;

    const auto error_msg = ss.str();
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "%s", error_msg.c_str());
    return RTNCD_FAILURE;
  }

  return RTNCD_SUCCESS;
}

/**
 * Validate dataset existence.
 * Consolidates dataset existence validation logic used in write entry points.
 *
 * @param dsn Dataset name to validate
 * @param diag Diagnostic context for error messages
 * @return RTNCD_SUCCESS on success, RTNCD_FAILURE if dataset doesn't exist
 */
static int validate_dataset_exists(const std::string &dsn, ZDIAG &diag)
{
  if (!zds_dataset_exists(dsn))
  {
    diag.e_msg_len = sprintf(diag.e_msg, "Could not access '%s'", dsn.c_str());
    return RTNCD_FAILURE;
  }
  return RTNCD_SUCCESS;
}

/**
 * Validate RECFM support for writing.
 * Consolidates RECFM validation logic used in write entry points.
 *
 * @param attrs DSCB attributes containing RECFM
 * @param is_member Whether this is a member write (affects standard format support)
 * @param diag Diagnostic context for error messages
 * @return RTNCD_SUCCESS on success, RTNCD_FAILURE on unsupported RECFM
 */
static int validate_write_recfm(const DscbAttributes &attrs, bool is_member, ZDIAG &diag)
{
  if (zds_write_recfm_unsupported(attrs.recfm, !is_member))
  {
    diag.e_msg_len = sprintf(diag.e_msg, "Writing to RECFM=%s data sets is not supported", attrs.recfm.c_str());
    diag.detail_rc = ZDS_RTNCD_UNSUPPORTED_RECFM;
    return RTNCD_FAILURE;
  }
  return RTNCD_SUCCESS;
}

/**
 * Validate etag if present.
 * Consolidates etag validation logic used in write entry points.
 *
 * @param zds ZDS context
 * @param dsn Dataset name
 * @param has_encoding Whether encoding is enabled
 * @return RTNCD_SUCCESS on success, RTNCD_FAILURE on validation failure
 */
static int validate_etag_if_present(ZDS *zds, const std::string &dsn, bool has_encoding)
{
  if (std::strlen(zds->etag) > 0 && 0 != zds_validate_etag(zds, dsn, has_encoding))
  {
    return RTNCD_FAILURE;
  }
  return RTNCD_SUCCESS;
}

int zds_write(const ZDSWriteOpts &opts, const std::string &data)
{
  const int vrc = zds_validate_write_opts(opts, "zds_write");
  if (vrc != RTNCD_SUCCESS)
  {
    return vrc;
  }

  ZDS *zds = opts.zds;
  const auto is_dd = !opts.ddname.empty();

  // DD writes: skip DSN validations, use sequential path, no etag
  if (is_dd)
  {
    const std::string target = zds_resolve_write_target(opts);
    const DscbAttributes empty_attrs{};
    return zds_write_sequential(zds, target, data, empty_attrs);
  }

  // DSN writes: full validation and logic from zds_write_to_dsn
  const std::string &dsn = opts.dsname;

  int rc = validate_dataset_exists(dsn, zds->diag);
  if (rc != RTNCD_SUCCESS)
  {
    return rc;
  }

  const DscbAttributes attrs = zds_get_dscb_attributes(dsn);
  const auto is_member = zds_has_member(dsn);

  rc = validate_write_recfm(attrs, is_member, zds->diag);
  if (rc != RTNCD_SUCCESS)
  {
    return rc;
  }

  const auto has_encoding = zds_use_codepage(zds);

  rc = validate_etag_if_present(zds, dsn, has_encoding);
  if (rc != RTNCD_SUCCESS)
  {
    return rc;
  }

  // Use BPAM path for PDS/PDSE members (updates ISPF stats), sequential for others
  if (is_member)
  {
    rc = zds_write_member_bpam(zds, dsn, data);
    // Clear ddname after BPAM close since the DD was freed
    memset(zds->ddname, 0, sizeof(zds->ddname));
  }
  else
  {
    rc = zds_write_sequential(zds, zds_resolve_write_target(opts), data, attrs);
  }

  if (rc == RTNCD_FAILURE)
  {
    return rc;
  }

  // Print new e-tag to stdout as response
  std::string saved_contents = "";
  ZDSReadOpts read_opts{.zds = zds, .dsname = dsn};
  const auto read_rc = zds_read(read_opts, saved_contents);
  if (0 != read_rc)
  {
    return RTNCD_FAILURE;
  }

  std::stringstream etag_stream;
  etag_stream << std::hex << zut_calc_adler32_checksum(saved_contents);
  strcpy(zds->etag, etag_stream.str().c_str());

  return rc;
}

int zds_write_streamed(const ZDSWriteOpts &opts, const std::string &pipe, size_t *content_len)
{
  const int vrc = zds_validate_write_opts(opts, "zds_write_streamed");
  if (vrc != RTNCD_SUCCESS)
  {
    return vrc;
  }

  ZDS *zds = opts.zds;

  if (content_len == nullptr)
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "content_len must be a valid size_t pointer");
    return RTNCD_FAILURE;
  }

  const auto is_dd = !opts.ddname.empty();

  // DD writes: skip DSN validations, use sequential streamed path
  if (is_dd)
  {
    const std::string target = zds_resolve_write_target(opts);
    const DscbAttributes empty_attrs{};
    return zds_write_sequential_streamed(zds, target, pipe, content_len, empty_attrs);
  }

  // DSN writes: full validation and logic from zds_write_to_dsn_streamed
  const std::string &dsn = opts.dsname;

  int rc = validate_dataset_exists(dsn, zds->diag);
  if (rc != RTNCD_SUCCESS)
  {
    return rc;
  }

  const DscbAttributes attrs = zds_get_dscb_attributes(dsn);
  const auto is_member = zds_has_member(dsn);

  rc = validate_write_recfm(attrs, is_member, zds->diag);
  if (rc != RTNCD_SUCCESS)
  {
    return rc;
  }

  const auto has_encoding = zds_use_codepage(zds);

  rc = validate_etag_if_present(zds, dsn, has_encoding);
  if (rc != RTNCD_SUCCESS)
  {
    return rc;
  }

  // Use BPAM path for PDS/PDSE members, sequential for others
  if (is_member)
  {
    rc = zds_write_member_bpam_streamed(zds, dsn, pipe, content_len);
    // Clear ddname after BPAM close since the DD was freed
    memset(zds->ddname, 0, sizeof(zds->ddname));
  }
  else
  {
    rc = zds_write_sequential_streamed(zds, zds_resolve_write_target(opts), pipe, content_len, attrs);
  }

  if (rc == RTNCD_FAILURE)
  {
    return rc;
  }

  // Print new e-tag to stdout as response
  std::string saved_contents = "";
  ZDSReadOpts read_opts{.zds = zds, .dsname = dsn};
  const auto read_rc = zds_read(read_opts, saved_contents);
  if (0 != read_rc)
  {
    return RTNCD_FAILURE;
  }

  std::stringstream etag_stream;
  etag_stream << std::hex << zut_calc_adler32_checksum(saved_contents);
  strcpy(zds->etag, etag_stream.str().c_str());

  return rc;
}

int zds_open_output_bpam(ZDS *zds, const std::string &dsname, IO_CTRL *&ioc)
{
  std::string alloc_cmd = "ALLOC DA('" + dsname + "') SHR"; // TODO(Kelosky): log this command
  unsigned int code = 0;
  std::string resp = "";
  std::string ddname = "";
  int rc = zut_bpxwdyn_rtdd(alloc_cmd, &code, resp, ddname);
  if (0 != rc)
  {
    strcpy(zds->diag.service_name, "BPXWDYN");
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Failed to allocate with code '%08X' on data set '%s': %s", code, dsname.c_str(), resp.c_str());
    zds->diag.detail_rc = ZDS_RTNCD_SERVICE_FAILURE;
    zds->diag.service_rc = code;
    return RTNCD_FAILURE;
  }

  zds->dynalloc = 1;
  zut_uppercase_pad_truncate(zds->ddname, ddname.c_str(), sizeof(zds->ddname));

  rc = ZDSOBPAM(zds, &ioc, zds->ddname);
  if (0 != rc)
  {
    return rc;
  }
  return RTNCD_SUCCESS;
}

int zds_write_output_bpam(ZDS *zds, IO_CTRL *ioc, std::string &data, char asa_char)
{
  int rc = 0;
  int length = 0;
  if (asa_char != '\0')
  {
    std::string asa_data;
    asa_data.reserve(data.length() + 1);
    asa_data = asa_char;
    asa_data += data;
    length = asa_data.length();
    rc = ZDSWBPAM(zds, ioc, asa_data.c_str(), &length);
  }
  else
  {
    length = data.length();
    rc = ZDSWBPAM(zds, ioc, data.c_str(), &length);
  }
  if (0 != rc)
  {
    if (0 == zds->diag.e_msg_len) // only set error if no error message was already set
    {
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Failed to write output to BPAM: %s... (%d bytes)", data.substr(0, 20).c_str(), length);
      return RTNCD_FAILURE;
    }
  }
  return rc;
}

int zds_close_output_bpam(ZDS *zds, IO_CTRL *ioc)
{
  int rc = 0;
  unsigned int code = 0;
  std::string resp = "";
  if (ioc == nullptr)
  {
    zds->diag.detail_rc = RTNCD_WARNING;
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "IO_CTRL is NULL");
    rc = RTNCD_WARNING;
  }
  else
  {
    rc = ZDSCBPAM(zds, ioc);
  }

  if (0 == zds->dynalloc)
  {
    if (0 == zds->diag.e_msg_len) // only set error if no error message was already set
    {
      zds->diag.detail_rc = RTNCD_WARNING;
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Data set was not dynamically allocated");
    }
  }
  else
  {
    std::string free_cmd = "FREE DD(" + std::string(zds->ddname, sizeof(zds->ddname)) + ")";
    int rc = zut_bpxwdyn(free_cmd, &code, resp);
    if (0 != rc)
    {
      if (0 == zds->diag.e_msg_len) // only set error if no error message was already set
      {
        zds->diag.detail_rc = ZDS_RTNCD_SERVICE_FAILURE;
        zds->diag.service_rc = code;
        zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Failed to free with code '%08X' on data set '%s': %s", code, zds->ddname, resp.c_str());
        rc = RTNCD_FAILURE;
      }
    }
  }

  return rc;
}

// https://www.ibm.com/docs/en/zos/3.1.0?topic=examples-listing-partitioned-data-set-members
#define RECLEN 254

typedef struct
{
  unsigned short int count;
  char rest[RECLEN];
} RECORD;

typedef struct
{
  char name[8];
  unsigned char ttr[3];
  unsigned char info;
} RECORD_ENTRY;

int alloc_and_free(const std::string &alloc_dd, const std::string &dsn, unsigned int *code, std::string &resp)
{
  int rc = zut_bpxwdyn(alloc_dd, code, resp);
  if (RTNCD_SUCCESS == rc)
  {
    rc = zut_bpxwdyn("FREE DA('" + dsn + "')", code, resp);
  }
  return rc;
}

// TODO(Kelosky): add attributues to ZDS and have other functions populate it
int zds_create_dsn(ZDS *, const std::string &dsn, DS_ATTRIBUTES attributes, std::string &response)
{
  unsigned int code = 0;
  std::string parm = "ALLOC DA('" + dsn + "')";
  std::transform(attributes.alcunit.begin(), attributes.alcunit.end(), attributes.alcunit.begin(), ::toupper);
  if (attributes.alcunit.empty() || attributes.alcunit == "TRACKS" || attributes.alcunit == "TRK")
  {
    attributes.alcunit = "TRACKS"; // Allocation Unit
  }
  else if (attributes.alcunit == "CYLINDERS" || attributes.alcunit == "CYL")
  {
    attributes.alcunit = "CYL"; // Allocation Unit
  }
  else
  {
    response = "Invalid allocation unit '" + attributes.alcunit + "'";
    return RTNCD_FAILURE;
  }
  // Only default blksize when unset (negative); preserve 0 for RECFM U
  if (attributes.blksize < 0)
  {
    attributes.blksize = (attributes.lrecl > 0) ? attributes.lrecl : 80; // Block Size (avoid system default when unset)
  }
  if (attributes.primary == 0)
  {
    attributes.primary = 1; // Primary Space
  }
  // Only default lrecl when unset (negative); preserve 0 for RECFM U
  if (attributes.lrecl < 0)
  {
    attributes.lrecl = 80; // Record Length
  }

  // Required options, default to PS if not specified
  if (!attributes.dsorg.empty())
    parm += " DSORG(" + attributes.dsorg + ")";
  else
  {
    parm += " DSORG(PS)";
  }

  if (attributes.primary > 0)
  {
    parm += " SPACE(" + std::to_string(attributes.primary);

    if (attributes.secondary > 0)
    {
      parm += "," + std::to_string(attributes.secondary);
    }

    parm += ") " + attributes.alcunit;
  }

  if (!attributes.recfm.empty())
    parm += " RECFM(" + attributes.recfm + ")";

  // For RECFM=U, LRECL should be 0 or omitted; specifying non-zero LRECL can cause
  // the system to override RECFM to FB
  bool is_recfm_u = (attributes.recfm == "U" || attributes.recfm == ZDS_RECFM_U);
  if (attributes.lrecl > 0 && !is_recfm_u)
  {
    parm += " LRECL(" + std::to_string(attributes.lrecl) + ")";
  }

  if (attributes.dirblk > 0)
  {
    parm += " DIR(" + std::to_string(attributes.dirblk) + ")";
  }

  parm += " NEW CATALOG";

  if (!attributes.dsntype.empty())
    parm += " DSNTYPE(" + attributes.dsntype + ")";
  if (!attributes.storclass.empty())
    parm += " STORCLAS(" + attributes.storclass + ")";
  if (!attributes.dataclass.empty())
    parm += " DATACLAS(" + attributes.dataclass + ")";
  if (!attributes.mgntclass.empty())
    parm += " MGMTCLAS(" + attributes.mgntclass + ")";
  if (!attributes.vol.empty())
    parm += " VOL(" + attributes.vol + ")";
  if (!attributes.unit.empty())
    parm += " UNIT(" + attributes.unit + ")";

  // For RECFM=U, BLKSIZE handling is system-dependent; some systems need it, others don't
  // Skip BLKSIZE for RECFM=U to let the system determine the appropriate value
  if (attributes.blksize >= 0 && !is_recfm_u)
  {
    parm += " BLKSIZE(" + std::to_string(attributes.blksize) + ")";
  }

  return alloc_and_free(parm, dsn, &code, response);
}

int zds_create_dsn_fb(ZDS *zds, const std::string &dsn, std::string &response)
{
  DS_ATTRIBUTES attributes{};
  attributes.dsorg = "PO";
  attributes.primary = 5;
  attributes.secondary = 5;
  attributes.alcunit = "CYL";
  attributes.lrecl = 80;
  attributes.recfm = "F,B";
  attributes.dirblk = 5;
  attributes.dsntype = ZDS_DSNTYPE_LIBRARY;
  return zds_create_dsn(zds, dsn, attributes, response);
}

int zds_create_dsn_vb(ZDS *zds, const std::string &dsn, std::string &response)
{
  DS_ATTRIBUTES attributes{};
  attributes.dsorg = "PO";
  attributes.primary = 5;
  attributes.secondary = 5;
  attributes.alcunit = "CYL";
  attributes.lrecl = 255;
  attributes.recfm = "V,B";
  attributes.dirblk = 5;
  attributes.dsntype = ZDS_DSNTYPE_LIBRARY;
  return zds_create_dsn(zds, dsn, attributes, response);
}

int zds_create_dsn_adata(ZDS *zds, const std::string &dsn, std::string &response)
{
  DS_ATTRIBUTES attributes{};
  attributes.dsorg = "PO";
  attributes.primary = 5;
  attributes.secondary = 5;
  attributes.alcunit = "CYL";
  attributes.lrecl = 32756;
  attributes.blksize = 32760;
  attributes.recfm = "V,B";
  attributes.dirblk = 5;
  attributes.dsntype = ZDS_DSNTYPE_LIBRARY;
  return zds_create_dsn(zds, dsn, attributes, response);
}

int zds_create_dsn_loadlib(ZDS *zds, const std::string &dsn, std::string &response)
{
  DS_ATTRIBUTES attributes{};
  attributes.dsorg = "PO";
  attributes.primary = 5;
  attributes.secondary = 5;
  attributes.alcunit = "CYL";
  attributes.lrecl = 0;
  attributes.blksize = 32760;
  attributes.recfm = "U";
  attributes.dirblk = 5;
  attributes.dsntype = ZDS_DSNTYPE_LIBRARY;
  return zds_create_dsn(zds, dsn, attributes, response);
}

#define NUM_DELETE_TEXT_UNITS 2
int zds_delete_dsn(ZDS *zds, std::string dsn)
{
  int rc = 0;

  dsn = "//'" + dsn + "'";

  rc = remove(dsn.c_str());

  if (0 != rc)
  {
    strcpy(zds->diag.service_name, "remove");
    zds->diag.service_rc = rc;
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Could not delete data set '%s', rc: '%d'", dsn.c_str(), rc);
    zds->diag.detail_rc = ZDS_RTNCD_SERVICE_FAILURE;
    return RTNCD_FAILURE;
  }

  return 0;
}

int zds_rename_dsn(ZDS *zds, std::string dsn_before, std::string dsn_after)
{
  int rc = 0;
  if (dsn_before.empty() || dsn_after.empty())
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Data set names must be valid");
    return RTNCD_FAILURE;
  }
  if (dsn_after.length() > 44)
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Target data set name exceeds max character length of 44");
    return RTNCD_FAILURE;
  }
  if (!zds_dataset_exists(dsn_before))
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Source data set does not exist '%s'", dsn_before.c_str());
    return RTNCD_FAILURE;
  }
  if (zds_dataset_exists(dsn_after))
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Target data set name already exists '%s'", dsn_after.c_str());
    return RTNCD_FAILURE;
  }

  dsn_before = "//'" + dsn_before + "'";
  dsn_after = "//'" + dsn_after + "'";

  errno = 0;
  rc = rename(dsn_before.c_str(), dsn_after.c_str());

  if (rc != 0)
  {
    int err = errno;
    strcpy(zds->diag.service_name, "rename");
    zds->diag.service_rc = rc;
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Could not rename data set '%s', errno: '%d'", dsn_before.c_str(), err);
    zds->diag.detail_rc = ZDS_RTNCD_SERVICE_FAILURE;
    return RTNCD_FAILURE;
  }

  return 0;
}

int zds_rename_members(ZDS *zds, const std::string &dsname, const std::string &member_before, const std::string &member_after)
{
  int rc = 0;
  if (dsname.empty())
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Data set name must be valid");
    return RTNCD_FAILURE;
  }
  if (member_before.empty() || member_after.empty())
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Member name cannot be empty");
    return RTNCD_FAILURE;
  }
  if (!zds_dataset_exists(dsname))
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Data set does not exist");
    return RTNCD_FAILURE;
  }
  if (!zds_member_exists(dsname, member_before))
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Source member does not exist");
    return RTNCD_FAILURE;
  }
  if (zds_member_exists(dsname, member_after))
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Target member already exists");
    return RTNCD_FAILURE;
  }
  if (!zds_is_valid_member_name(member_after) || !zds_is_valid_member_name(member_before))
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Member name must start with A-Z,#,@,$ and contain only A-Z,0-9,#,@,$ (max 8 chars)");
    return RTNCD_FAILURE;
  }

  std::string source_member = "//'" + dsname + "(" + member_before + ")'";
  std::string target_member = "//'" + dsname + "(" + member_after + ")'";
  errno = 0;
  rc = rename(source_member.c_str(), target_member.c_str());

  if (rc != 0)
  {
    int err = errno;
    strcpy(zds->diag.service_name, "rename_members");
    zds->diag.service_rc = rc;
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Could not rename member '%s', errno: '%d'", source_member.c_str(), err);
    zds->diag.detail_rc = ZDS_RTNCD_SERVICE_FAILURE;
    return RTNCD_FAILURE;
  }

  return 0;
}

void parse_packed_time(
    const unsigned char hours_byte,
    const unsigned char minutes_byte,
    const unsigned char seconds_byte,
    std::string *time_out)
{
  if (hours_byte == 0 && minutes_byte == 0 && seconds_byte == 0)
  {
    *time_out = "";
    return;
  }

  // Bit-shifting is safe from Compiler Backend crashes
  int hh = ((hours_byte >> 4) & 0x0F) * 10 + (hours_byte & 0x0F);
  int mm = ((minutes_byte >> 4) & 0x0F) * 10 + (minutes_byte & 0x0F);
  int ss = ((seconds_byte >> 4) & 0x0F) * 10 + (seconds_byte & 0x0F);

  if (hh > 23 || mm > 59 || ss > 59)
  {
    *time_out = "";
    return;
  }

  char buffer[9];
  sprintf(buffer, "%02d:%02d:%02d", hh, mm, ss);
  *time_out = buffer;
}

bool is_match(const char *s, const char *p)
{
  const char *star_p = nullptr; // Tracks the last '*' seen in the pattern
  const char *star_s = nullptr; // Tracks where we were in the string when we saw the '*'

  while (*s != '\0')
  {
    // Exact match or '?' wildcard: advance both pointers
    if (*p == '?' || *p == *s)
    {
      s++;
      p++;
    }
    // '*' wildcard found: record positions and advance the pattern
    else if (*p == '*')
    {
      star_p = p;
      star_s = s;
      p++;
    }
    // Mismatch, but we previously saw a '*': backtrack to the star
    else if (star_p != nullptr)
    {
      p = star_p + 1; // Reset pattern pointer to right after the star
      star_s++;       // Force the star to consume one more character of the string
      s = star_s;     // Reset string pointer to this new position
    }
    // Mismatch and no '*' to save us: immediate failure
    else
    {
      return false;
    }
  }

  // Consume any trailing '*' at the end of the pattern
  while (*p == '*')
  {
    p++;
  }

  // If we've reached the end of the pattern, it's a full match
  return *p == '\0';
}

int zds_list_members(ZDS *zds, std::string dsn, std::vector<ZDSMem> &members, const std::string &pattern, bool show_attributes)
{
  // PO
  // PO-E (PDS)
  const auto formatted_dsn = "//'" + dsn + "'";

  int total_entries = 0;

  if (0 == zds->max_entries)
    zds->max_entries = ZDS_DEFAULT_MAX_MEMBER_ENTRIES;

  members.reserve(zds->max_entries);

  std::string upper_pattern = pattern;
  std::transform(upper_pattern.begin(), upper_pattern.end(), upper_pattern.begin(), ::toupper);

  RECORD rec{};
  // https://www.ibm.com/docs/en/zos/3.1.0?topic=pds-reading-directory-sequentially
  // https://www.ibm.com/docs/en/zos/3.1.0?topic=pdse-reading-directory - long alias names omitted, use DESERV for those
  // https://www.ibm.com/docs/en/zos/3.1.0?topic=pds-directory
  FILE *fp = fopen(formatted_dsn.c_str(), "rb,recfm=u");

  if (!fp)
  {
    constexpr auto EABEND = 92;
    if (errno == EABEND)
    {
      __amrc_type save_amrc = *__amrc;
      const auto abend_code = save_amrc.__code.__abend.__syscode;
      if (abend_code == 0x913)
      {
        // Insufficient permissions for this data set
        zds->diag.e_msg_len = snprintf(zds->diag.e_msg, sizeof(zds->diag.e_msg), "Insufficient permissions for opening data set '%s' (S913 abend)", dsn.c_str());
      }
      else
      {
        zds->diag.e_msg_len = snprintf(zds->diag.e_msg, sizeof(zds->diag.e_msg), "Could not list members for '%s': %s (errno %d) abend code S%03X", dsn.c_str(), strerror(errno), errno, save_amrc.__code.__abend.__syscode);
      }
    }
    else
    {
      zds->diag.e_msg_len = snprintf(zds->diag.e_msg, sizeof(zds->diag.e_msg), "Could not list members for '%s': %s (errno %d)", dsn.c_str(), strerror(errno), errno);
    }
    return RTNCD_FAILURE;
  }

  while (fread(&rec, sizeof(rec), 1, fp))
  {
    unsigned char *data = nullptr;
    data = (unsigned char *)&rec;
    data += sizeof(rec.count); // increment past halfword length
    int len = sizeof(RECORD_ENTRY);
    for (int i = 0; i < rec.count; i = i + len)
    {
      RECORD_ENTRY entry{};
      memcpy(&entry, data, sizeof(entry));
      long long int end = 0xFFFFFFFFFFFFFFFF; // indicates end of entries
      if (memcmp(entry.name, &end, sizeof(end)) == 0)
      {
        break;
      }
      else
      {
        unsigned char info = entry.info;
        unsigned char pointer_count = entry.info;

        if (info & 0x80) // bit 0 indicates alias
        {
          // TODO(Kelosky): // member name is an alias
        }

        // TODO(Trae): Consider removing this? It's not used anywhere.
        // pointer_count &= 0x60; // bits 1-2 contain number of user data TTRNs
        // pointer_count >>= 5;   // adjust to byte boundary

        info &= 0x1F; // bits 3-7 contain the number of half words of user data

        char name[9] = {};
        memcpy(name, entry.name, sizeof(entry.name));

        for (int j = 8; j >= 0; j--)
        {
          if (name[j] == ' ')
          {
            name[j] = 0x00;
          }
        }

        if (pattern.empty() || is_match(name, upper_pattern.c_str()))
        {
          total_entries++;

          if (total_entries > zds->max_entries)
          {
            zds->diag.e_msg_len = snprintf(zds->diag.e_msg, sizeof(zds->diag.e_msg), "Reached maximum returned members requested %d", zds->max_entries);
            zds->diag.detail_rc = ZDS_RSNCD_MAXED_ENTRIES_REACHED;
            fclose(fp);
            return RTNCD_WARNING;
          }

          ZDSMem mem{};
          mem.name = std::string(name);
          int user_data_len = info * 2;

          if (show_attributes && user_data_len >= sizeof(ISPF_STATS))
          {
            const ISPF_STATS *stats = reinterpret_cast<const ISPF_STATS *>(data + sizeof(entry));
            mem.vers = stats->version;
            mem.mod = stats->level;

            mem.sclm = (stats->flags & 0x80) != 0;

            int rc = zut_convert_date(&stats->created_date_century, mem.c4date);

            // Convert Modified Date
            rc = zut_convert_date(&stats->modified_date_century, mem.m4date);

            parse_packed_time(
                stats->modified_time_hours,
                stats->modified_time_minutes,
                stats->modified_time_seconds,
                &mem.mtime);

            mem.cnorc = stats->current_number_of_lines;
            mem.inorc = stats->initial_number_of_lines;
            mem.mnorc = stats->modified_number_of_lines;

            char user[9] = {0};
            memcpy(user, stats->userid, 8);
            mem.user = std::string(user);
          }
          members.push_back(mem);
        }

        data += sizeof(entry) + info * 2;
        len = sizeof(entry) + info * 2;

        int remainder = rec.count - (i + len);
        if (remainder < sizeof(entry))
          break;
      }
    }
  }

  fclose(fp);
  return 0;
}

ZNP_PACK_ON

// https://www.ibm.com/docs/en/zos/3.1.0?topic=format-work-area-table
// https://www.ibm.com/docs/en/zos/3.1.0?topic=format-work-area-picture
typedef struct
{
  int total_size;
  int min_size;
  int used_size;
  short number_fields;
} ZDS_CSI_HEADER;

typedef struct
{
  char modid[2];
  unsigned char rsn;
  unsigned char rc;
} ZDS_CSI_ERROR_INFO;

typedef struct
{
  unsigned char flag;
#define NOT_SUPPORTED 0x80         // CSINTICF
#define NO_ENTRY 0x40              // CSINOENT
#define DATA_NOT_COMPLETE 0x20     // CSINTCMP
#define PROCESS_ERROR 0x10         // CSICERR
#define PARTIAL_PROCESS_ERROR 0x08 // CSICERRP
#define ERROR_CONDITION 0XF8       // all flags
  unsigned char type;
#define CATALOG_TYPE 0xF0
  char name[MAX_DS_LENGTH];
  ZDS_CSI_ERROR_INFO error_info;
} ZDS_CSI_CATALOG;

// This count should match number of catalog fields requested from CSI
#define CSI_FIELD_COUNT 5
#define CSI_FIELD_LEN 8

typedef struct
{
  int total_len;
  unsigned int reserved;
  int field_lens[CSI_FIELD_COUNT]; // data after field_lens
} ZDS_CSI_FIELD;

typedef struct
{
  unsigned char flag;
#define PRIMARY_ENTRY 0x80 // CSIPMENT
#define ERROR 0x40         // CSIENTER
#define DATA 0x20          // CSIEDATA
  unsigned char type;
#define NOT_FOUND 0x00
#define NON_VSAM_DATA_SET 'A'
#define GENERATION_DATA_GROUP 'B'
#define CLUSTER 'C'
#define DATA_COMPONENT 'D'
#define ALTERNATE_INDEX 'G'
#define GENERATION_DATA_SET 'H'
#define INDEX_COMPONENT 'I'
#define ATL_LIBRARY_ENTRY 'L'
#define PATH 'R'
#define USER_CATALOG_CONNECTOR_ENTRY 'U'
#define ATL_VOLUME_ENTRY 'W'
#define ALIAS 'X'
  char name[MAX_DS_LENGTH];

  union
  {
    ZDS_CSI_ERROR_INFO error_info; // if CSIENTER=1
    ZDS_CSI_FIELD field;
  } response;
} ZDS_CSI_ENTRY;

typedef struct
{
  ZDS_CSI_HEADER header;
  ZDS_CSI_CATALOG catalog;
  ZDS_CSI_ENTRY entry;
} ZDS_CSI_WORK_AREA;

ZNP_PACK_OFF

#define DS1DSGPS_MASK 0x4000 // PS: Bit 2 is set
#define DS1DSGDA_MASK 0x2000 // DA: Bit 3 is set
#define DS1DSGPO_MASK 0x0200 // PO: Bit 7 is set
#define DS1DSGU_MASK 0x0100  // Unmovable: Bit 8 is set
#define DS1ACBM_MASK 0x0008  // VSAM: Bit 13 is set
#define DS1PDSE_MASK 0x10    // PDSE: Bit 4 in ds1smsfg
#define DS1ENCRP_MASK 0x04   // Encrypted: Bit 5 in ds1flag1

void load_dsorg_from_dscb(const DSCBFormat1 *dscb, std::string *dsorg)
{
  // Bitmasks translated from binary to hex from "DFSMSdfp advanced services" PDF, Chapter 1 page 7 (PDF page 39)
  if (dscb->ds1dsorg & DS1DSGPS_MASK)
  {
    *dsorg = ZDS_DSORG_PS;
  }
  else if (dscb->ds1dsorg & DS1DSGDA_MASK)
  {
    *dsorg = ZDS_DSORG_DA;
  }
  else if (dscb->ds1dsorg & DS1DSGPO_MASK)
  {
    *dsorg = ZDS_DSORG_PO;
  }
  else if (dscb->ds1dsorg & DS1ACBM_MASK)
  {
    *dsorg = ZDS_DSORG_VSAM;
  }

  // Unmovable: Last bit of first half is set
  if (dscb->ds1dsorg & DS1DSGU_MASK)
  {
    *dsorg += 'U';
  }

  if (dsorg->empty())
  {
    *dsorg = ZDS_DSORG_UNKNOWN;
  }
}

void load_recfm_from_dscb(const DSCBFormat1 *dscb, std::string *recfm)
{
  // Bitmasks translated from binary to hex from "DFSMSdfp advanced services" PDF, Chapter 1 page 7 (PDF page 39)
  // Fixed: First bit is set
  if ((dscb->ds1recfm & 0xC0) == 0x80)
  {
    *recfm = ZDS_RECFM_F;
  }
  // Variable: Second bit is set
  else if ((dscb->ds1recfm & 0xC0) == 0x40)
  {
    *recfm = ZDS_RECFM_V;
  }
  // Undefined: First and second bits are set
  else if ((dscb->ds1recfm & 0xC0) == 0xC0)
  {
    *recfm = ZDS_RECFM_U;
  }

  // Blocked records: Fourth bit is set
  if ((dscb->ds1recfm & 0x10) > 0)
  {
    *recfm += 'B';
  }

  // Sequential: Fifth bit is set
  if ((dscb->ds1recfm & 0x08) > 0 && recfm[0] != ZDS_RECFM_U)
  {
    *recfm += 'S';
  }

  // ANSI control characters/ASA: Sixth bit is set
  if ((dscb->ds1recfm & 0x04) > 0)
  {
    *recfm += 'A';
  }

  // Machine-control characters: Seventh bit is set
  if ((dscb->ds1recfm & 0x02) > 0)
  {
    *recfm += 'M';
  }

  if (recfm->empty())
  {
    *recfm = ZDS_RECFM_U;
  }
}

void load_date_from_dscb(const char *date_in, std::string *date_out, bool is_expiration_date = false)
{
  // Date is in 'YDD' format (3 bytes): Year offset and day of year
  // If all zeros, date is not maintained
  unsigned char year_offset = static_cast<unsigned char>(date_in[0]);
  unsigned char day_high = static_cast<unsigned char>(date_in[1]);
  unsigned char day_low = static_cast<unsigned char>(date_in[2]);

  // Check if date is zero (not maintained)
  if (year_offset == 0 && day_high == 0 && day_low == 0)
  {
    *date_out = "";
    return;
  }

  // Parse year: add 1900 to the year offset
  int year = 1900 + year_offset;

  // Parse day of year from 2-byte value (big-endian)
  int day_of_year = (day_high << 8) | day_low;

  // Check for sentinel date 1999/12/31 (year_offset=99, day=365)
  // This is used to indicate "no expiration" for expiration dates only
  if (is_expiration_date && year == 1999 && day_of_year == 365)
  {
    *date_out = "";
    return;
  }

  // Convert day of year to month/day
  static const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  // Check for leap year
  bool is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);

  int month = 1;
  int day = day_of_year;

  for (int i = 0; i < 12 && day > days_in_month[i]; i++)
  {
    int days_this_month = days_in_month[i];
    if (i == 1 && is_leap) // February in leap year
      days_this_month = 29;

    day -= days_this_month;
    month++;
  }

  // Format as "YYYY/MM/DD"
  char buffer[11];
  sprintf(buffer, "%04d/%02d/%02d", year, month, day);
  *date_out = buffer;
}

// Map UCB device type last byte to device type number
static inline uint16_t ucb_to_devtype(uint8_t ucb_byte)
{
  // Support only the DASD unit types supported by DFSMShsm
  // https://www.ibm.com/docs/en/zos/2.5.0?topic=command-unit-specifying-type-device
  switch (ucb_byte)
  {
  case ZDS_DEVTYPE_3380:
    return 0x3380;
  case ZDS_DEVTYPE_3390:
    return 0x3390;
  case ZDS_DEVTYPE_9345:
    return 0x9345;
  default:
    return 0x0000; // Unsupported device type
  }
}

// Parse CCHH (Cylinder-Cylinder-Head-Head) format to get cylinder number
static inline uint32_t parse_cchh_cylinder(const char *cchh)
{
  uint16_t cyl_low = (static_cast<unsigned char>(cchh[0]) << 8) |
                     static_cast<unsigned char>(cchh[1]);
  uint16_t cyl_high = ((static_cast<unsigned char>(cchh[2]) << 4) |
                       (static_cast<unsigned char>(cchh[3]) >> 4)) &
                      0x0FFF;
  return (cyl_high << 16) | cyl_low;
}

// Get bytes per track for a given device type
static inline int get_bytes_per_track(const uint16_t &devtype)
{
  switch (devtype)
  {
  case 0x3380:
    return 47476;
  case 0x9345:
    return 46456;
  case 0x3390:
  default:
    return 56664;
  }
}

// Calculate tracks in an extent given lower and upper CCHH values
static inline int calculate_extent_tracks(const char *extent, const uint16_t &devtype)
{
  uint32_t lower_cyl = parse_cchh_cylinder(extent + 2);
  uint16_t lower_head = static_cast<unsigned char>(extent[5]) & 0x0F;
  uint32_t upper_cyl = parse_cchh_cylinder(extent + 6);
  uint16_t upper_head = static_cast<unsigned char>(extent[9]) & 0x0F;

  if (upper_cyl < lower_cyl)
    return 0;

  const auto tracks_per_cyl = 15;
  return ((upper_cyl - lower_cyl) * tracks_per_cyl) + (upper_head - lower_head) + 1;
}

// Load space unit (spacu) from DSCB
void load_spacu_from_dscb(const DSCBFormat1 *dscb, ZDSEntry &entry)
{
  uint8_t scal1 = static_cast<uint8_t>(dscb->ds1scalo[0]);
  uint8_t dspac = (scal1 & 0xC0) >> 6; // Top 2 bits: space request type

  if (dspac == 0x03) // 11 = DS1CYL - Cylinders
  {
    entry.spacu = "CYLINDERS";
  }
  else if (dspac == 0x02) // 10 = DS1TRK - Tracks
  {
    // Check bit 4 (0x10) - DS1CONTG (contiguous request)
    bool is_contiguous = (scal1 & 0x10) != 0;
    if (is_contiguous)
    {
      entry.spacu = "BYTES"; // Contiguous track allocation displayed as BYTES
    }
    else
    {
      entry.spacu = "TRACKS"; // Non-contiguous track allocation displayed as TRACKS
    }
  }
  else if (dspac == 0x01) // 01 = DS1AVR - Average block length (blocks)
  {
    entry.spacu = "BLOCKS";
  }
  else if ((scal1 & 0x20) != 0) // Bit 2 set: DS1EXT - Extension to secondary space
  {
    // Fall back to ds1scext[0] (ds1scxtf) for extended units
    uint8_t scxtf = static_cast<uint8_t>(dscb->ds1scext[0]);

    if (scxtf & 0x40) // DS1SCMB - megabytes
    {
      entry.spacu = "MEGABYTES";
    }
    else if (scxtf & 0x20) // DS1SCKB - kilobytes
    {
      entry.spacu = "KILOBYTES";
    }
    else if (scxtf & 0x10) // DS1SCUB - bytes
    {
      entry.spacu = "BYTES";
    }
    else
    {
      entry.spacu = "TRACKS"; // Default fallback
    }
  }
  else
  {
    // dspac == 0x00 - Absolute track request (DS1DSABS)
    entry.spacu = "BYTES";
  }
}

// Load primary and secondary allocation quantities from DSCB
void load_primary_secondary_from_dscb(const DSCBFormat1 *dscb, ZDSEntry &entry)
{
  uint8_t scal1 = static_cast<uint8_t>(dscb->ds1scalo[0]);
  uint8_t dspac = (scal1 & 0xC0) >> 6;

  // Parse DS1SCAL3 (ds1scalo[1-3]) - secondary allocation quantity (3 bytes)
  uint32_t scal3 = (static_cast<unsigned char>(dscb->ds1scalo[1]) << 16) |
                   (static_cast<unsigned char>(dscb->ds1scalo[2]) << 8) |
                   static_cast<unsigned char>(dscb->ds1scalo[3]);

  const char *extent_data = dscb->ds1exnts;

  // Primary: First extent size
  // Read allocx directly from DSCB to avoid dependency on entry.allocx being set
  uint8_t allocx = dscb->ds1noepv;
  if (allocx > 0)
  {
    const char *first_extent = extent_data;

    if (dspac == 0x03) // Cylinders
    {
      uint32_t lower_cyl = parse_cchh_cylinder(first_extent + 2);
      uint32_t upper_cyl = parse_cchh_cylinder(first_extent + 6);
      entry.primary = (upper_cyl - lower_cyl + 1);
    }
    else // Tracks or Blocks
    {
      entry.primary = calculate_extent_tracks(first_extent, entry.devtype);
    }
  }
  else
  {
    entry.primary = 0;
  }

  // Secondary: Allocation quantity from DS1SCEXT or DS1SCAL3
  // For BYTES space unit, read from DS1SCEXT (contiguous/absolute allocations)
  if (entry.spacu == "BYTES")
  {
    // Parse DS1SCEXT: ds1scext[0] = DS1SCXTF (flags), ds1scext[1-2] = DS1SCXTV (value)
    uint8_t scxtf = static_cast<uint8_t>(dscb->ds1scext[0]);
    uint16_t scxtv = (static_cast<unsigned char>(dscb->ds1scext[1]) << 8) |
                     static_cast<unsigned char>(dscb->ds1scext[2]);

    // Check if value is compacted
    if (scxtf & 0x08) // DS1SCCP1 - compacted by 256
    {
      entry.secondary = scxtv * 256LL;
    }
    else if (scxtf & 0x04) // DS1SCCP2 - compacted by 65,536
    {
      entry.secondary = scxtv * 65536LL;
    }
    else
    {
      entry.secondary = scxtv;
    }
  }
  else
  {
    entry.secondary = scal3;
  }
}

// Load attributes categorized by ISPF as General Data (dsorg, recfm, etc)
void load_general_attrs_from_dscb(const DSCBFormat1 *dscb, ZDSEntry &entry)
{
  load_dsorg_from_dscb(dscb, &entry.dsorg);

  // Determine space unit first (needed for other calculations)
  load_spacu_from_dscb(dscb, entry);

  // Check if this is a VSAM dataset
  bool is_vsam = (dscb->ds1dsorg & DS1ACBM_MASK) != 0;

  if (is_vsam)
  {
    // VSAM datasets: blksize, lrecl, and recfm are not meaningful
    entry.blksize = -1;
    entry.lrecl = -1;
    entry.recfm = "";
  }
  else
  {
    load_recfm_from_dscb(dscb, &entry.recfm);
    entry.blksize = (static_cast<unsigned char>(dscb->ds1blkl[0]) << 8) |
                    static_cast<unsigned char>(dscb->ds1blkl[1]);
    entry.lrecl = (static_cast<unsigned char>(dscb->ds1lrecl[0]) << 8) |
                  static_cast<unsigned char>(dscb->ds1lrecl[1]);
  }

  // Determine dsntype: PDS or LIBRARY (PDSE)
  // DS1DSGPO (0x0200) in ds1dsorg indicates partitioned organization
  bool is_partitioned = (dscb->ds1dsorg & DS1DSGPO_MASK) != 0;
  // DS1PDSE (0x10) in ds1smsfg indicates PDSE
  bool is_pdse = (dscb->ds1smsfg & DS1PDSE_MASK) != 0;

  if (is_partitioned && is_pdse)
  {
    entry.dsntype = ZDS_DSNTYPE_LIBRARY;
  }
  else if (is_partitioned)
  {
    entry.dsntype = ZDS_DSNTYPE_PDS;
  }
  else
  {
    entry.dsntype = "";
  }

  // Check if dataset is encrypted
  // DS1ENCRP (0x04) in ds1flag1 indicates access method encrypted dataset
  entry.encrypted = (dscb->ds1flag1 & DS1ENCRP_MASK) != 0;

  // Load primary and secondary allocations (part of General Data in ISPF)
  load_primary_secondary_from_dscb(dscb, entry);
}

// Load Current Allocation attributes (allocated units/tracks/bytes, allocated extents)
void load_alloc_attrs_from_dscb(const DSCBFormat1 *dscb, ZDSEntry &entry)
{
  uint8_t scal1 = static_cast<uint8_t>(dscb->ds1scalo[0]);
  uint8_t dspac = (scal1 & 0xC0) >> 6;

  // allocx: Number of extents allocated on this volume
  entry.allocx = dscb->ds1noepv;

  // Parse the extent information from ds1exnts (3 extents, 10 bytes each)
  const char *extent_data = dscb->ds1exnts;
  int total_cylinders = 0;
  int total_tracks = 0;
  bool use_cylinders = (dspac == 0x03);

  for (int i = 0; i < 3; i++)
  {
    const char *extent = extent_data + (i * 10);
    uint8_t extent_type = static_cast<uint8_t>(extent[0]);

    if (extent_type == 0x00)
      break;

    uint32_t lower_cyl = parse_cchh_cylinder(extent + 2);
    uint32_t upper_cyl = parse_cchh_cylinder(extent + 6);

    if (upper_cyl >= lower_cyl)
    {
      int cylinders_in_extent = upper_cyl - lower_cyl + 1;
      int tracks_in_extent = calculate_extent_tracks(extent, entry.devtype);
      total_cylinders += cylinders_in_extent;
      total_tracks += tracks_in_extent;
    }
  }

  entry.alloc = use_cylinders ? total_cylinders : total_tracks;

  // Convert to bytes if spacu is BYTES
  if (entry.spacu == "BYTES" && entry.blksize > 0)
  {
    int bytes_per_track = get_bytes_per_track(entry.devtype);
    int blocks_per_track = bytes_per_track / entry.blksize;
    if (blocks_per_track <= 0)
      blocks_per_track = 1;
    long long base_bytes_per_track = blocks_per_track * entry.blksize;

    if (entry.alloc > 0)
      entry.alloc *= base_bytes_per_track;
    if (entry.primary > 0)
      entry.primary *= base_bytes_per_track;

    // Secondary already in bytes from DS1SCEXT for contiguous allocations
  }
}

// Load Used Space attributes (used space percentage, used extents)
void load_used_attrs_from_dscb(const DSCBFormat1 *dscb, ZDSEntry &entry)
{
  uint8_t scal1 = static_cast<uint8_t>(dscb->ds1scalo[0]);
  uint8_t dspac = (scal1 & 0xC0) >> 6;
  bool use_cylinders = (dspac == 0x03);

  // Check if this is a PDSE
  bool is_pdse = (dscb->ds1smsfg & DS1PDSE_MASK) != 0;

  if ((dscb->ds1dsorg & DS1DSGDA_MASK) || (dscb->ds1dsorg & DS1ACBM_MASK) || is_pdse)
  {
    // DA (Direct Access/BDAM), VSAM, or PDSE: DS1LSTAR not meaningful
    entry.usedp = -1;
    entry.usedx = -1;
    return;
  }

  // Parse DS1LSTAR (last used track in TTR format)
  uint8_t lstar_tt_high = static_cast<unsigned char>(dscb->ds1lstar[0]);
  uint8_t lstar_tt_low = static_cast<unsigned char>(dscb->ds1lstar[1]);
  uint32_t last_used_track = (lstar_tt_high << 8) | lstar_tt_low;

  // Check if this is a large format dataset that uses DS1TTHI
  bool is_large = (dscb->ds1flag1 & 0x10) != 0;
  if (is_large)
  {
    uint8_t lstar_tt_highest = static_cast<unsigned char>(dscb->ds1ttthi);
    last_used_track |= (lstar_tt_highest << 16);
  }

  // Special case: DS1LSTAR = 0x000000 for blksize=0 means unused
  bool is_blksize_zero = (entry.blksize == 0);
  if (last_used_track == 0 && is_blksize_zero)
  {
    entry.usedp = 0;
    entry.usedx = 0;
    return;
  }

  // Store used space temporarily in tracks/cylinders (will convert to percentage later)
  if (use_cylinders)
  {
    const auto tracks_per_cyl = 15;
    entry.usedp = (last_used_track + 1 + tracks_per_cyl - 1) / tracks_per_cyl; // Round up to cylinders
  }
  else
  {
    entry.usedp = last_used_track + 1; // Tracks (0-based, so +1)
  }

  // Calculate how many extents contain data (count of used extents)
  if (entry.usedp > 0)
  {
    const char *extent_data = dscb->ds1exnts;
    int cumulative_tracks = 0;
    entry.usedx = 0;

    for (int i = 0; i < 3; i++)
    {
      const char *extent = extent_data + (i * 10);
      uint8_t extent_type = static_cast<uint8_t>(extent[0]);

      if (extent_type == 0x00)
        break;

      int tracks_in_extent = calculate_extent_tracks(extent, entry.devtype);
      cumulative_tracks += tracks_in_extent;

      // Count this extent as used if it contains data up to the last used track
      if (last_used_track <= cumulative_tracks)
      {
        entry.usedx = i + 1;
        break;
      }
    }
  }

  // Calculate used percentage
  // Note: DS1TRBAL contains bytes remaining calculated by IBM TRKCALC,
  // but since we don't have access to TRKCALC's exact capacity formula,
  // we approximate by using simple track-based percentage calculation.

  if (entry.usedp > 0 && entry.alloc > 0)
  {
    int used_value = entry.usedp;
    long long alloc_value = entry.alloc;

    // For BYTES space unit, convert to track-based calculation for percentage
    if (entry.spacu == "BYTES" && entry.blksize > 0)
    {
      int bytes_per_track = get_bytes_per_track(entry.devtype);
      int blocks_per_track = bytes_per_track / entry.blksize;
      if (blocks_per_track <= 0)
        blocks_per_track = 1;
      long long base_bytes_per_track = blocks_per_track * entry.blksize;

      // Convert alloc from bytes to tracks for percentage calculation
      alloc_value = entry.alloc / base_bytes_per_track;
    }

    // Calculate used percentage and round down to nearest int to match z/OSMF
    double percentage = (100.0 * used_value) / alloc_value;
    entry.usedp = (int)percentage;

    if (entry.usedp > 100)
      entry.usedp = 100;
  }
  else if (entry.usedp == 0)
  {
    entry.usedp = 0;
  }
  else
  {
    entry.usedp = -1;
  }
}

// Load Date attributes (creation date, expiration date, referenced date)
void load_date_attrs_from_dscb(const DSCBFormat1 *dscb, ZDSEntry &entry)
{
  load_date_from_dscb(dscb->ds1credt, &entry.cdate, false);
  load_date_from_dscb(dscb->ds1expdt, &entry.edate, true); // expiration date can have sentinel
  load_date_from_dscb(dscb->ds1refd, &entry.rdate, false);
}

// Load storage management attributes from catalog fields
void load_volsers_from_catalog(const unsigned char *&data, const int field_len, int &csi_offset, ZDSEntry &entry)
{
  int num_volumes = field_len / MAX_VOLSER_LENGTH;
  entry.multivolume = num_volumes > 1;
  entry.volsers.reserve(num_volumes);

  char buffer[MAX_VOLSER_LENGTH + 1] = {};
  for (int i = 0; i < num_volumes; i++)
  {
    memset(buffer, 0x00, sizeof(buffer)); // clear buffer
    memcpy(buffer, data + csi_offset + (i * MAX_VOLSER_LENGTH), MAX_VOLSER_LENGTH);
    auto vol = std::string(buffer, MAX_VOLSER_LENGTH);
    zut_rtrim(vol);
    if (!vol.empty())
      entry.volsers.push_back(vol);
  }

  // Use the first volume as the primary, or set unknown if empty
  entry.volser = entry.volsers.empty() ? ZDS_VOLSER_UNKNOWN : entry.volsers[0];

#define IPL_VOLUME "******"
#define IPL_VOLUME_SYMBOL "&SYSR1" // https://www.ibm.com/docs/en/zos/3.1.0?topic=symbols-static-system

  if (0 == strcmp(IPL_VOLUME, entry.volser.c_str()))
  {
    std::string symbol(IPL_VOLUME_SYMBOL);
    std::string value;
    int rc = zut_substitute_symbol(symbol, value);
    if (0 == rc)
    {
      entry.volser = value;
    }
  }

  csi_offset += field_len;
}

void load_devtype_from_catalog(const unsigned char *&data, const int field_len, int &csi_offset, uint16_t &attr)
{
  if (field_len >= 4)
  {
    const uint8_t *bytes = &data[csi_offset];
    uint32_t devtyp = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];

    // Extract the device type code (last byte of UCB) and map to device type
    // If devtyp is all zeros, default to 3390 (most common modern DASD)
    attr = devtyp != 0x00000000 ? ucb_to_devtype(bytes[3]) : 0x3390;
  }

  csi_offset += field_len;
}

void load_storage_attr_from_catalog(const unsigned char *&data, const int field_len, int &csi_offset, std::string &attr)
{
  attr = "";

  if (field_len > 2)
  {
    // First 2 bytes are a length prefix, extract actual length
    uint16_t actual_len = (static_cast<unsigned char>(data[csi_offset]) << 8) |
                          static_cast<unsigned char>(data[csi_offset + 1]);
    if (actual_len > 0 && actual_len <= (field_len - 2))
    {
      attr = std::string((char *)(data + csi_offset + 2), actual_len);
    }
  }

  csi_offset += field_len;
}

void zds_get_attrs_from_dscb(ZDS *zds, ZDSEntry &entry)
{
  auto *dscb = (DSCBFormat1 *)__malloc31(sizeof(DSCBFormat1));
  if (dscb == nullptr)
  {
    return;
  }

  memset(dscb, 0x00, sizeof(DSCBFormat1));
  const auto rc = ZDSDSCB1(zds, entry.name.c_str(), entry.volser.c_str(), dscb);

  if (rc == RTNCD_SUCCESS)
  {
    // Load attributes in logical order
    load_general_attrs_from_dscb(dscb, entry);
    load_alloc_attrs_from_dscb(dscb, entry);
    load_used_attrs_from_dscb(dscb, entry);
    load_date_attrs_from_dscb(dscb, entry);
  }
  else
  {
    entry.alloc = -1;
    entry.allocx = -1;
    entry.blksize = -1;
    entry.lrecl = -1;
    entry.primary = -1;
    entry.secondary = -1;
    entry.usedp = -1;
    entry.usedx = -1;
  }

  free(dscb);
}

int zds_list_data_sets(ZDS *zds, std::string dsn, std::vector<ZDSEntry> &datasets, bool show_attributes)
{
  int rc = 0;

  zds->csi = nullptr;

  // https://www.ibm.com/docs/en/zos/3.1.0?topic=directory-catalog-field-names
  std::string fields_long[CSI_FIELD_COUNT][CSI_FIELD_LEN] = {{"VOLSER"}, {"DEVTYP"}, {"DATACLAS"}, {"MGMTCLAS"}, {"STORCLAS"}};

  std::string(*fields)[CSI_FIELD_LEN] = nullptr;
  int number_of_fields = 0;

  if (show_attributes)
  {
    fields = fields_long;
    number_of_fields = sizeof(fields_long) / sizeof(fields_long[0]);
  }

  int internal_used_buffer_size = sizeof(CSIFIELD) + number_of_fields * CSI_FIELD_LEN;

  if (0 == zds->buffer_size)
    zds->buffer_size = ZDS_DEFAULT_BUFFER_SIZE;
  if (0 == zds->max_entries)
    zds->max_entries = ZDS_DEFAULT_MAX_ENTRIES;

#define MIN_BUFFER_FIXED 1024

  int min_buffer_size = MIN_BUFFER_FIXED + internal_used_buffer_size;

  if (zds->buffer_size < min_buffer_size)
  {
    zds->diag.detail_rc = ZDS_RTNCD_INSUFFICIENT_BUFFER;
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Minimum buffer size required is %d but %d was provided", min_buffer_size, zds->buffer_size);
    return RTNCD_FAILURE;
  }

  auto *area = (unsigned char *)__malloc31(zds->buffer_size);
  if (area == nullptr)
  {
    zds->diag.detail_rc = ZDS_RTNCD_INSUFFICIENT_BUFFER;
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Failed to allocate 31-bit buffer for workarea to list %s", dsn.c_str());
    return RTNCD_FAILURE;
  }
  memset(area, 0x00, zds->buffer_size);

  CSIFIELD *selection_criteria = (CSIFIELD *)area;
  char *csi_fields = (char *)(selection_criteria->csifldnm);
  ZDS_CSI_WORK_AREA *csi_work_area = (ZDS_CSI_WORK_AREA *)(area + sizeof(CSIFIELD) + number_of_fields * CSI_FIELD_LEN);

  // set work area
  csi_work_area->header.total_size = zds->buffer_size - min_buffer_size;

#define FOUR_BYTE_RETURN 'F'

  // init blanks in query and set input DSN name
  memset(selection_criteria->csifiltk, ' ', sizeof(selection_criteria->csifiltk));
  std::transform(dsn.begin(), dsn.end(), dsn.begin(), ::toupper); // upper case
  memcpy(selection_criteria->csifiltk, dsn.c_str(), dsn.size());
  memset(&selection_criteria->csicldi, 'Y', sizeof(selection_criteria->csicldi));
  memset(&selection_criteria->csiresum, ' ', sizeof(selection_criteria->csiresum));
  memset(&selection_criteria->csicatnm, ' ', sizeof(selection_criteria->csicatnm));
  memset(&selection_criteria->csis1cat, 'Y', sizeof(selection_criteria->csis1cat)); // do not search master catalog if alias is found
  memset(&selection_criteria->csioptns, FOUR_BYTE_RETURN, sizeof(selection_criteria->csioptns));
  memset(selection_criteria->csidtyps, ' ', sizeof(selection_criteria->csidtyps));

  for (int i = 0; i < number_of_fields; i++)
  {
    memset(csi_fields, ' ', CSI_FIELD_LEN);
    memcpy(csi_fields, fields[i][0].c_str(), fields[i][0].size());
    csi_fields += CSI_FIELD_LEN;
  }

  selection_criteria->csinumen = number_of_fields;

  do
  {
    rc = ZDSCSI00(zds, selection_criteria, csi_work_area);

    if (0 != rc)
    {
      free(area);
      ZDSDEL(zds);
      strcpy(zds->diag.service_name, "ZDSCSI00");
      zds->diag.service_rc = rc;
      zds->diag.detail_rc = ZDS_RTNCD_SERVICE_FAILURE;
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "ZDSCSI00 failed with rc %d", rc);
      return RTNCD_FAILURE;
    }

    int number_fields = csi_work_area->header.number_fields - 1;

    if (number_fields != number_of_fields)
    {
      free(area);
      ZDSDEL(zds);
      zds->diag.detail_rc = ZDS_RTNCD_UNEXPECTED_ERROR;
      zds->diag.e_msg_len =
          sprintf(zds->diag.e_msg,
                  "Unexpected work area field response preset len %d and "
                  "return len %d are not equal",
                  number_fields, number_of_fields);
      return RTNCD_FAILURE;
    }

    if (ERROR_CONDITION == csi_work_area->catalog.flag)
    {
      free(area);
      ZDSDEL(zds);
      zds->diag.detail_rc = ZDS_RTNCD_PARSING_ERROR;
      zds->diag.service_rc = ZDS_RTNCD_CATALOG_ERROR;
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Unexpected catalog flag '%x' ", csi_work_area->catalog.flag);
      return RTNCD_FAILURE;
    }

    if (DATA_NOT_COMPLETE & csi_work_area->catalog.flag)
    {
      free(area);
      ZDSDEL(zds);
      zds->diag.detail_rc = ZDS_RTNCD_PARSING_ERROR;
      zds->diag.service_rc = ZDS_RTNCD_CATALOG_ERROR;
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Unexpected catalog flag '%x' ", csi_work_area->catalog.flag);
      return RTNCD_FAILURE;
    }

    if (NO_ENTRY & csi_work_area->catalog.flag)
    {
      free(area);
      ZDSDEL(zds);
      zds->diag.detail_rc = ZDS_RSNCD_NOT_FOUND;
      zds->diag.service_rc = ZDS_RTNCD_CATALOG_ERROR;
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Not found in catalog, flag '%x' ", csi_work_area->catalog.flag);
      return RTNCD_WARNING;
    }

    if (CATALOG_TYPE != csi_work_area->catalog.type)
    {
      free(area);
      ZDSDEL(zds);
      zds->diag.detail_rc = ZDS_RTNCD_PARSING_ERROR;
      zds->diag.service_rc = ZDS_RTNCD_CATALOG_ERROR;
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Unexpected type '%x' ", csi_work_area->catalog.type);
      return RTNCD_FAILURE;
    }

    int work_area_total = csi_work_area->header.used_size;
    unsigned char *p = (unsigned char *)&csi_work_area->entry;
    ZDS_CSI_ENTRY *f = nullptr;

    work_area_total -= sizeof(ZDS_CSI_HEADER);
    work_area_total -= sizeof(ZDS_CSI_CATALOG);

    char buffer[sizeof(f->name) + 1] = {};

    while (work_area_total > 0)
    {
      ZDSEntry entry{};
      f = (ZDS_CSI_ENTRY *)p;

      if (ERROR == f->flag)
      {
        free(area);
        ZDSDEL(zds);
        zds->diag.detail_rc = ZDS_RTNCD_SERVICE_FAILURE;
        zds->diag.service_rc = ZDS_RTNCD_ENTRY_ERROR;
        zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Unexpected entry flag '%x' ", f->flag);
        return RTNCD_FAILURE;
      }

      if (NOT_FOUND == f->type)
      {
        free(area);
        ZDSDEL(zds);
        zds->diag.detail_rc = ZDS_RTNCD_NOT_FOUND;
        zds->diag.service_rc = ZDS_RTNCD_ENTRY_ERROR;
        zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "No entry found '%x' ", f->type);
        return RTNCD_FAILURE;
      }

      // NOTE(Kelosky): rejecting entities we haven't tested... this can be removed as we verify the logic works for all return types
      if (
          NON_VSAM_DATA_SET != f->type &&
          CLUSTER != f->type &&
          DATA_COMPONENT != f->type &&
          INDEX_COMPONENT != f->type &&
          GENERATION_DATA_GROUP != f->type &&
          GENERATION_DATA_SET != f->type &&
          ALIAS != f->type &&
          PATH != f->type &&
          ALTERNATE_INDEX != f->type)
      {
        free(area);
        ZDSDEL(zds);
        zds->diag.detail_rc = ZDS_RTNCD_SERVICE_FAILURE;
        zds->diag.service_rc = ZDS_RTNCD_UNSUPPORTED_ERROR;
        zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Unsupported entry type '%x' ", f->type);
        return RTNCD_FAILURE;
      }

      memset(buffer, 0x00, sizeof(buffer));     // clear buffer
      memcpy(buffer, f->name, sizeof(f->name)); // copy all & leave a null
      entry.name = std::string(buffer);

      // Only process catalog fields and DSCB if show_attributes is true
      if (show_attributes)
      {
        const int *field_lens = f->response.field.field_lens;
        const unsigned char *data = (unsigned char *)&f->response.field.field_lens + sizeof(f->response.field.field_lens);
        int csi_offset = 0;

        // Load volume serials from catalog and check if migrated
        load_volsers_from_catalog(data, field_lens[0], csi_offset, entry);

#define MIGRAT_VOLUME "MIGRAT"
#define ARCIVE_VOLUME "ARCIVE"

        if (entry.volser == MIGRAT_VOLUME || entry.volser == ARCIVE_VOLUME)
        {
          entry.migrated = true;
          entry.recfm = ZDS_RECFM_U;
        }
        else
        {
          entry.migrated = false;
        }

        // Parse storage management attributes (devtype, dataclass, mgmtclass, storclass)
        load_devtype_from_catalog(data, field_lens[1], csi_offset, entry.devtype);
        load_storage_attr_from_catalog(data, field_lens[2], csi_offset, entry.dataclass);
        load_storage_attr_from_catalog(data, field_lens[3], csi_offset, entry.mgmtclass);
        load_storage_attr_from_catalog(data, field_lens[4], csi_offset, entry.storclass);

        // Load detailed attributes from DSCB if not migrated
        // This needs to happen before the switch statement as it sets entry.dsorg
        if (!entry.migrated)
        {
          zds_get_attrs_from_dscb(zds, entry);
        }

        std::string old_volser = entry.volser;
        switch (f->type)
        {
        case NON_VSAM_DATA_SET:
          // DSORG and PDSE/PDS determination is done via DSCB parsing
          break;
        case GENERATION_DATA_GROUP:
          entry.volser = ZDS_VOLSER_GDG;
          break;
        case CLUSTER:
          entry.dsorg = ZDS_DSORG_VSAM;
          entry.volser = ZDS_VOLSER_VSAM;
          break;
        case DATA_COMPONENT:
        case INDEX_COMPONENT:
          entry.dsorg = ZDS_DSORG_VSAM;
          break;
        case ALTERNATE_INDEX:
          entry.volser = ZDS_VOLSER_AIX;
          break;
        case PATH:
          entry.volser = ZDS_VOLSER_PATH;
          break;
        case GENERATION_DATA_SET:
          break;
        case ALIAS:
          entry.dsorg = ZDS_DSORG_UNKNOWN;
          entry.volser = ZDS_VOLSER_ALIAS;
          break;
        case ATL_LIBRARY_ENTRY:
        case USER_CATALOG_CONNECTOR_ENTRY:
        case ATL_VOLUME_ENTRY:
        default:
          free(area);
          ZDSDEL(zds);
          zds->diag.detail_rc = ZDS_RTNCD_SERVICE_FAILURE;
          zds->diag.service_rc = ZDS_RTNCD_UNSUPPORTED_ERROR;
          zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Unsupported entry type '%x' ", f->type);
          return RTNCD_FAILURE;
        };

        if (old_volser != entry.volser)
        {
          entry.volsers.clear();
          entry.volsers.push_back(entry.volser);
        }
      }

      if (datasets.size() + 1 > zds->max_entries)
      {
        free(area);
        ZDSDEL(zds);
        zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Reached maximum returned records requested %d", zds->max_entries);
        zds->diag.detail_rc = ZDS_RSNCD_MAXED_ENTRIES_REACHED;
        return RTNCD_WARNING;
      }

      datasets.push_back(entry);

      work_area_total -= ((sizeof(ZDS_CSI_ENTRY) - sizeof(ZDS_CSI_FIELD) + f->response.field.total_len));
      p = p + ((sizeof(ZDS_CSI_ENTRY) - sizeof(ZDS_CSI_FIELD) + f->response.field.total_len)); // next entry
    }
  } while ('Y' == selection_criteria->csiresum);

  free(area);
  ZDSDEL(zds);

  return RTNCD_SUCCESS;
}

/**
 * Reads data from a data set in streaming mode.
 *
 * @param opts read options containing ZDS state and either a dsname or ddname
 * @param pipe name of the output pipe
 * @param content_len pointer where the length of the data set contents will be stored
 *
 * @return RTNCD_SUCCESS on success, RTNCD_FAILURE on failure
 */
int zds_read_streamed(const ZDSReadOpts &opts, const std::string &pipe, size_t *content_len)
{
  const int vrc = zds_validate_read_opts(opts, "zds_read_streamed");
  if (vrc != RTNCD_SUCCESS)
  {
    return vrc;
  }

  ZDS *zds = opts.zds;

  if (content_len == nullptr)
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "content_len must be a valid size_t pointer");
    return RTNCD_FAILURE;
  }

  const std::string dsname = zds_resolve_dsname(opts);
  const auto is_dd = !opts.ddname.empty();

  // Check if ASA format - use record I/O to preserve ASA control characters
  const DscbAttributes attrs = zds_resolve_dscb(opts);
  const bool is_asa = attrs.is_asa && zds->encoding_opts.data_type != eDataTypeBinary;

  std::string fopen_flags;
  if (zds->encoding_opts.data_type == eDataTypeBinary)
  {
    fopen_flags = "rb";
  }
  else if (is_asa)
  {
    // Use record I/O to preserve ASA control characters (prevents runtime interpretation)
    fopen_flags = "rb,type=record";
  }
  else
  {
    fopen_flags = "r";
  }

  FileGuard fin(dsname.c_str(), fopen_flags.c_str());
  if (!fin)
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Could not open handle to %s '%s'", is_dd ? "DD" : "data set", dsname.c_str());
    return RTNCD_FAILURE;
  }

  int fifo_fd = open(pipe.c_str(), O_WRONLY);
  FileGuard fout(fifo_fd, "w");
  if (!fout)
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Could not open output pipe '%s'", pipe.c_str());
    close(fifo_fd);
    return RTNCD_FAILURE;
  }

  const auto has_encoding = zds_use_codepage(zds);
  const auto codepage = std::string(zds->encoding_opts.codepage);

  std::vector<char> temp_encoded;
  std::vector<char> left_over;

  // Open iconv descriptor once for all chunks (for stateful encodings like IBM-939)
  const std::string source_encoding = has_encoding && std::strlen(zds->encoding_opts.source_codepage) > 0 ? std::string(zds->encoding_opts.source_codepage) : "UTF-8";
  IconvGuard iconv_guard(has_encoding ? source_encoding.c_str() : nullptr, has_encoding ? codepage.c_str() : nullptr);
  if (has_encoding && !iconv_guard.is_valid())
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Cannot open converter from %s to %s", codepage.c_str(), source_encoding.c_str());
    return RTNCD_FAILURE;
  }

  if (is_asa)
  {
    // For ASA, read record by record and append newlines between records
    // This preserves the ASA control character as the first byte of each line
    const int lrecl = attrs.lrecl > 0 ? attrs.lrecl : 32760;
    std::vector<char> buf(lrecl);
    size_t bytes_read;
    bool first_record = true;

    while ((bytes_read = fread(&buf[0], 1, lrecl, fin)) > 0)
    {
      // Add newline before each record (except the first)
      std::string record_data;
      if (!first_record)
      {
        record_data.append(1, '\n');
      }
      first_record = false;

      // Trim trailing spaces from the record (fixed-length records are padded)
      size_t actual_len = bytes_read;
      while (actual_len > 0 && buf[actual_len - 1] == ' ')
      {
        actual_len--;
      }

      record_data.append(&buf[0], actual_len);

      const char *chunk = record_data.c_str();
      int chunk_len = record_data.length();

      if (has_encoding)
      {
        try
        {
          temp_encoded = zut_encode(chunk, chunk_len, iconv_guard.get(), zds->diag);
          chunk = &temp_encoded[0];
          chunk_len = temp_encoded.size();
        }
        catch (std::exception &e)
        {
          zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Failed to convert input data from %s to %s", codepage.c_str(), source_encoding.c_str());
          return RTNCD_FAILURE;
        }
      }

      *content_len += chunk_len;
      temp_encoded = zbase64::encode(chunk, chunk_len, &left_over);
      fwrite(&temp_encoded[0], 1, temp_encoded.size(), fout);
    }

    // Add trailing newline if we read any records
    if (!first_record)
    {
      const char newline = '\n';
      const char *chunk = &newline;
      int chunk_len = 1;

      if (has_encoding)
      {
        try
        {
          temp_encoded = zut_encode(chunk, chunk_len, iconv_guard.get(), zds->diag);
          chunk = &temp_encoded[0];
          chunk_len = temp_encoded.size();
        }
        catch (std::exception &e)
        {
          zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Failed to convert input data from %s to %s", codepage.c_str(), source_encoding.c_str());
          return RTNCD_FAILURE;
        }
      }

      *content_len += chunk_len;
      temp_encoded = zbase64::encode(chunk, chunk_len, &left_over);
      fwrite(&temp_encoded[0], 1, temp_encoded.size(), fout);
    }
  }
  else
  {
    // Non-ASA: read in chunks
    const size_t chunk_size = FIFO_CHUNK_SIZE * 3 / 4;
    std::vector<char> buf(chunk_size);
    size_t bytes_read;

    while ((bytes_read = fread(&buf[0], 1, chunk_size, fin)) > 0)
    {
      int chunk_len = bytes_read;
      const char *chunk = &buf[0];

      if (has_encoding)
      {
        try
        {
          temp_encoded = zut_encode(chunk, chunk_len, iconv_guard.get(), zds->diag);
          chunk = &temp_encoded[0];
          chunk_len = temp_encoded.size();
        }
        catch (std::exception &e)
        {
          zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Failed to convert input data from %s to %s", codepage.c_str(), source_encoding.c_str());
          return RTNCD_FAILURE;
        }
      }

      *content_len += chunk_len;
      temp_encoded = zbase64::encode(chunk, chunk_len, &left_over);
      fwrite(&temp_encoded[0], 1, temp_encoded.size(), fout);
    }
  }

  // Flush the shift state for stateful encodings after all chunks are processed
  if (has_encoding && iconv_guard.is_valid())
  {
    try
    {
      std::vector<char> flush_buffer = zut_iconv_flush(iconv_guard.get(), zds->diag);
      if (flush_buffer.empty() && zds->diag.e_msg_len > 0)
      {
        return RTNCD_FAILURE;
      }

      // Write any shift sequence bytes that were generated
      if (!flush_buffer.empty())
      {
        *content_len += flush_buffer.size();
        temp_encoded = zbase64::encode(&flush_buffer[0], flush_buffer.size(), &left_over);
        fwrite(&temp_encoded[0], 1, temp_encoded.size(), fout);
      }
    }
    catch (std::exception &e)
    {
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Failed to flush encoding state");
      return RTNCD_FAILURE;
    }
  }

  if (!left_over.empty())
  {
    temp_encoded = zbase64::encode(&left_over[0], left_over.size());
    fwrite(&temp_encoded[0], 1, temp_encoded.size(), fout);
  }

  fflush(fout);

  return RTNCD_SUCCESS;
}

/**
 * Internal function to write to a sequential data set in streaming mode using fopen/fwrite
 */
static int zds_write_sequential_streamed(ZDS *zds, const std::string &dsn, const std::string &pipe, size_t *content_len, const DscbAttributes &attrs)
{
  std::string dsname = dsn;

  EncodingSetup encoding;
  int rc = setup_encoding(zds, encoding);
  if (rc != RTNCD_SUCCESS)
  {
    return rc;
  }

  const bool isTextMode = zds->encoding_opts.data_type != eDataTypeBinary;
  const int max_len = get_effective_lrecl_from_attrs(attrs);

  // Build fopen flags with actual recfm from DSCB
  // Use text mode - the C runtime handles ASA control characters and record boundaries
  const std::string recfm_flag = attrs.recfm.empty() ? "*" : attrs.recfm;
  const std::string fopen_flags = zds->encoding_opts.data_type == eDataTypeBinary
                                      ? "wb"
                                      : "w,recfm=" + recfm_flag;

  // Track truncated lines for text mode
  TruncationTracker truncation;
  int line_num = 0;
  std::string line_buffer;

  {
    FileGuard fout(dsname.c_str(), fopen_flags.c_str());
    if (!fout)
    {
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Could not open dsn '%s'", dsn.c_str());
      return RTNCD_FAILURE;
    }

    int fifo_fd = open(pipe.c_str(), O_RDONLY);
    if (fifo_fd == -1)
    {
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "open() failed on input pipe '%s', errno %d", pipe.c_str(), errno);
      return RTNCD_FAILURE;
    }

    FileGuard fin(fifo_fd, "r");
    if (!fin)
    {
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Could not open input pipe '%s'", pipe.c_str());
      close(fifo_fd);
      return RTNCD_FAILURE;
    }

    std::vector<char> buf(FIFO_CHUNK_SIZE);
    size_t bytes_read;
    std::vector<char> temp_encoded;
    std::vector<char> left_over;
    bool write_failed = false;

    // Open iconv descriptor once for all chunks (for stateful encodings like IBM-939)
    IconvGuard iconv_guard(encoding.has_encoding ? encoding.codepage.c_str() : nullptr, encoding.has_encoding ? encoding.source_encoding.c_str() : nullptr);
    int iconv_rc = validate_iconv_setup(encoding, iconv_guard, zds->diag);
    if (iconv_rc != RTNCD_SUCCESS)
    {
      return iconv_rc;
    }

    // Write chunks directly - the C runtime handles ASA and record boundaries in text mode
    while ((bytes_read = fread(&buf[0], 1, FIFO_CHUNK_SIZE, fin)) > 0)
    {
      temp_encoded = zbase64::decode(&buf[0], bytes_read, &left_over);
      const char *chunk = &temp_encoded[0];
      int chunk_len = temp_encoded.size();
      *content_len += chunk_len;

      rc = encode_chunk_if_needed(chunk, chunk_len, temp_encoded, encoding, iconv_guard, zds->diag);
      if (rc != RTNCD_SUCCESS)
      {
        return rc;
      }

      // Check for truncated lines in text mode (post-encoding to handle multi-byte encodings correctly)
      if (isTextMode && attrs.lrecl > 0)
      {
        line_buffer.append(chunk, chunk_len);

        size_t pos = 0;
        size_t newline_pos;
        while ((newline_pos = line_buffer.find(encoding.newline_char, pos)) != std::string::npos)
        {
          line_num++;
          size_t line_len = newline_pos - pos;

          // Account for CR if present (Windows line endings become CR+NL in EBCDIC)
          if (line_len > 0 && line_buffer[newline_pos - 1] == CR_CHAR)
          {
            line_len--;
          }

          if (static_cast<int>(line_len) > max_len)
          {
            truncation.add_line(line_num);
          }
          pos = newline_pos + 1;
        }

        // Keep remaining partial line in buffer
        line_buffer = line_buffer.substr(pos);
      }

      size_t bytes_written = fwrite(chunk, 1, chunk_len, fout);
      if (bytes_written != chunk_len)
      {
        write_failed = true;
        break;
      }
      temp_encoded.clear();
    }

    // Check any remaining partial line (no trailing newline)
    if (isTextMode && attrs.lrecl > 0 && !line_buffer.empty())
    {
      line_num++;
      size_t line_len = line_buffer.length();
      if (line_len > 0 && line_buffer[line_len - 1] == CR_CHAR)
      {
        line_len--;
      }
      if (static_cast<int>(line_len) > max_len)
      {
        truncation.add_line(line_num);
      }
    }

    truncation.flush_range();

    // Flush the shift state for stateful encodings after all chunks are processed
    std::vector<char> flush_buffer;
    rc = flush_encoding_state(encoding, iconv_guard, zds->diag, flush_buffer);
    if (rc != RTNCD_SUCCESS)
    {
      return rc;
    }

    // Write any shift sequence bytes that were generated
    if (!flush_buffer.empty())
    {
      size_t bytes_written = fwrite(&flush_buffer[0], 1, flush_buffer.size(), fout);
      if (bytes_written != flush_buffer.size())
      {
        write_failed = true;
      }
    }

    const int flush_rc = fflush(fout);
    if (write_failed || flush_rc != 0)
    {
      zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Failed to write to '%44.44s' (possibly out of space)", dsname.c_str());
      return RTNCD_FAILURE;
    }
  }

  // Handle truncation result
  return handle_truncation_result(zds, RTNCD_SUCCESS, truncation);
}

/**
 * Internal function to write to a PDS/PDSE member in streaming mode using BPAM (updates ISPF stats)
 */
static int zds_write_member_bpam_streamed(ZDS *zds, const std::string &dsn, const std::string &pipe, size_t *content_len)
{
  int rc = 0;
  IO_CTRL *ioc = nullptr;

  // Open the member for BPAM output
  rc = zds_open_output_bpam(zds, dsn, ioc);
  if (rc != RTNCD_SUCCESS)
  {
    return rc;
  }

  // Check if ASA format from DCB after open
  const bool is_asa = (ioc->dcb.dcbrecfm & dcbrecca) != 0;

  // Open the input pipe
  int fifo_fd = open(pipe.c_str(), O_RDONLY);
  if (fifo_fd == -1)
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "open() failed on input pipe '%s', errno %d", pipe.c_str(), errno);
    zds_close_output_bpam(zds, ioc);
    return RTNCD_FAILURE;
  }

  FileGuard fin(fifo_fd, "r");
  if (!fin)
  {
    zds->diag.e_msg_len = sprintf(zds->diag.e_msg, "Could not open input pipe '%s'", pipe.c_str());
    close(fifo_fd);
    zds_close_output_bpam(zds, ioc);
    return RTNCD_FAILURE;
  }

  EncodingSetup encoding;
  rc = setup_encoding(zds, encoding);
  if (rc != RTNCD_SUCCESS)
  {
    zds_close_output_bpam(zds, ioc);
    return rc;
  }

  // Track truncated lines
  TruncationTracker truncation;
  int line_num = 0;
  const int max_len = get_effective_lrecl(ioc);

  // ASA state tracking for streaming
  AsaStreamState asa_state;

  // Open iconv descriptor once for all lines (for stateful encodings like IBM-939)
  IconvGuard iconv_guard(encoding.has_encoding ? encoding.codepage.c_str() : nullptr, encoding.has_encoding ? encoding.source_encoding.c_str() : nullptr);
  rc = validate_iconv_setup(encoding, iconv_guard, zds->diag);
  if (rc != RTNCD_SUCCESS)
  {
    zds_close_output_bpam(zds, ioc);
    return rc;
  }

  std::vector<char> buf(FIFO_CHUNK_SIZE);
  size_t bytes_read;
  std::vector<char> temp_encoded;
  std::vector<char> left_over;
  std::string line_buffer; // Buffer for accumulating partial lines across chunks

  // Buffer the previous line so we can append flush bytes to the last one
  std::string prev_line;
  char prev_asa_char = '\0';
  bool has_prev_line = false;

  while ((bytes_read = fread(&buf[0], 1, FIFO_CHUNK_SIZE, fin)) > 0)
  {
    temp_encoded = zbase64::decode(&buf[0], bytes_read, &left_over);
    *content_len += temp_encoded.size();

    // Append chunk to line buffer (before encoding for ASA processing)
    line_buffer.append(temp_encoded.begin(), temp_encoded.end());

    size_t pos = 0;
    size_t newline_pos;
    while ((newline_pos = line_buffer.find(encoding.newline_char, pos)) != std::string::npos)
    {
      std::string line = line_buffer.substr(pos, newline_pos - pos);

      // Remove trailing carriage return if present (Windows line endings)
      if (!line.empty() && line[line.size() - 1] == CR_CHAR)
      {
        line.erase(line.size() - 1);
      }

      // Process ASA: determine control char, skip blank lines
      char asa_char = '\0';
      if (is_asa)
      {
        auto asa_result = asa_state.prepare_line(line);
        if (asa_result.skip_line)
        {
          pos = newline_pos + 1;
          continue;
        }

        // Flush overflow blank lines as empty '-' records (for 3+ blank lines)
        rc = write_asa_overflow_records(zds, ioc, asa_result.overflow_records);
        if (rc != RTNCD_SUCCESS)
        {
          zds_close_output_bpam(zds, ioc);
          return rc;
        }

        asa_char = asa_result.asa_char;
      }

      line_num++;

      // Encode line content
      rc = encode_line_if_needed(line, encoding, iconv_guard, zds->diag);
      if (rc != RTNCD_SUCCESS)
      {
        zds_close_output_bpam(zds, ioc);
        return rc;
      }

      // Check if line will be truncated (account for ASA char if present)
      if (static_cast<int>(line.length() + (is_asa ? 1 : 0)) > max_len)
      {
        truncation.add_line(line_num);
      }

      // Write previous line (not the current one - we buffer it in case it's the last)
      if (has_prev_line)
      {
        rc = zds_write_output_bpam(zds, ioc, prev_line, prev_asa_char);
        if (rc != RTNCD_SUCCESS)
        {
          DiagMsgGuard guard(&zds->diag);
          zds_close_output_bpam(zds, ioc);
          return rc;
        }
      }

      prev_line = line;
      prev_asa_char = is_asa ? asa_char : '\0';
      has_prev_line = true;

      pos = newline_pos + 1;
    }

    // Keep any remaining partial line in the buffer
    line_buffer = line_buffer.substr(pos);
  }

  // Handle remaining content that didn't end with a newline
  if (!line_buffer.empty())
  {
    ProcessedLine result;

    rc = process_bpam_trailing_line(line_buffer, is_asa, asa_state, encoding, iconv_guard,
                                    max_len, line_num, truncation, result, zds->diag);
    if (rc != RTNCD_SUCCESS)
    {
      zds_close_output_bpam(zds, ioc);
      return rc;
    }

    if (!result.skip_line)
    {
      // Flush overflow blank lines as empty '-' records
      rc = write_asa_overflow_records(zds, ioc, result.overflow_records);
      if (rc != RTNCD_SUCCESS)
      {
        zds_close_output_bpam(zds, ioc);
        return rc;
      }

      // Write previous line first
      if (has_prev_line)
      {
        rc = zds_write_output_bpam(zds, ioc, prev_line, prev_asa_char);
        if (rc != RTNCD_SUCCESS)
        {
          DiagMsgGuard guard(&zds->diag);
          zds_close_output_bpam(zds, ioc);
          return rc;
        }
      }

      prev_line = result.line;
      prev_asa_char = result.asa_char;
      has_prev_line = true;
    }
  }

  // Flush encoding state and append to the last line
  std::vector<char> flush_buffer;
  rc = flush_encoding_state(encoding, iconv_guard, zds->diag, flush_buffer);
  if (rc != RTNCD_SUCCESS)
  {
    zds_close_output_bpam(zds, ioc);
    return rc;
  }

  // Append flush bytes to the last line
  if (!flush_buffer.empty() && has_prev_line)
  {
    prev_line.append(flush_buffer.begin(), flush_buffer.end());
  }

  // Write the last buffered line
  if (has_prev_line)
  {
    rc = zds_write_output_bpam(zds, ioc, prev_line, prev_asa_char);
    if (rc != RTNCD_SUCCESS)
    {
      DiagMsgGuard guard(&zds->diag);
      zds_close_output_bpam(zds, ioc);
      return rc;
    }
  }

  // Finalize any pending range
  truncation.flush_range();

  // Close BPAM and handle truncation result
  rc = zds_close_output_bpam(zds, ioc);
  return handle_truncation_result(zds, rc, truncation);
}

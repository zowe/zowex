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

#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "../zbase64.h"
#include <vector>
#include <thread>
#include <chrono>

#include "ztest.hpp"
#include "zds.hpp"
#include "zjb.hpp"
#include "zut.hpp"
#include "zutils.hpp"
// #include "zstorage.metal.test.h"

using namespace ztst;

// Base for test contexts that create data sets and register them for cleanup
struct DataSetTestContextBase
{
  std::vector<std::string> &cleanup_list;

  explicit DataSetTestContextBase(std::vector<std::string> &list)
      : cleanup_list(list)
  {
  }

  void create_pds_at(const std::string &dsn)
  {
    ZDS zds = {0};
    create_pds(&zds, dsn);
  }

  void create_pdse_at(const std::string &dsn)
  {
    ZDS zds = {0};
    create_pdse(&zds, dsn);
  }

  void create_seq_at(const std::string &dsn)
  {
    ZDS zds = {0};
    create_seq(&zds, dsn);
  }
};

struct ListMembersTestContext : DataSetTestContextBase
{
  std::string pds_dsn;

  explicit ListMembersTestContext(std::vector<std::string> &list)
      : DataSetTestContextBase(list)
  {
    pds_dsn = get_random_ds(3);
    cleanup_list.push_back(pds_dsn);
  }

  void setup_pds()
  {
    create_pds_at(pds_dsn);
    write_to_dsn(pds_dsn + "(ABC1)", "data 1");
    write_to_dsn(pds_dsn + "(ABC2)", "data 2");
    write_to_dsn(pds_dsn + "(XYZ1)", "data 3");
    write_to_dsn(pds_dsn + "(TEST)", "data 4");
  }
};

// Test context for DD allocation and management
struct DDTestContext : DataSetTestContextBase
{
  std::vector<std::string> allocated_dds;

  explicit DDTestContext(std::vector<std::string> &list)
      : DataSetTestContextBase(list)
  {
  }

  void allocate_fb_dd(const std::string &ddname, int lrecl, int blksize = 0)
  {
    if (blksize == 0)
      blksize = lrecl;
    std::string alloc_cmd = "alloc dd(" + ddname + ") lrecl(" + std::to_string(lrecl) +
                            ") recfm(f,b) blksize(" + std::to_string(blksize) + ")";

    std::vector<std::string> single_dd;
    single_dd.push_back(alloc_cmd);
    allocated_dds.push_back(alloc_cmd);

    ZDIAG diag{};
    int rc = zut_loop_dynalloc(diag, single_dd);
    if (rc != 0)
    {
      throw std::runtime_error("Failed to allocate DD " + ddname + ": " + std::string(diag.e_msg));
    }
  }

  void allocate_vb_dd(const std::string &ddname, int lrecl)
  {
    std::string alloc_cmd = "alloc dd(" + ddname + ") lrecl(" + std::to_string(lrecl) +
                            ") recfm(v,b) blksize(" + std::to_string(lrecl + 4) + ")";

    std::vector<std::string> single_dd;
    single_dd.push_back(alloc_cmd);
    allocated_dds.push_back(alloc_cmd);

    ZDIAG diag{};
    int rc = zut_loop_dynalloc(diag, single_dd);
    if (rc != 0)
    {
      throw std::runtime_error("Failed to allocate VB DD " + ddname + ": " + std::string(diag.e_msg));
    }
  }

  void allocate_sysprint_dd()
  {
    allocate_fb_dd("SYSPRINT", 80);
  }

  bool verify_dd_content(const std::string &ddname, const std::string &expected)
  {
    ZDS read_zds{};
    ZDSReadOpts read_opts{.zds = &read_zds, .ddname = ddname};
    std::string content;
    int rc = zds_read(read_opts, content);
    return (rc == 0) && (content.find(expected) != std::string::npos);
  }

  ~DDTestContext()
  {
    if (!allocated_dds.empty())
    {
      ZDIAG diag{};
      zut_free_dynalloc_dds(diag, allocated_dds);
    }
  }
};

// Test context for copy operations
struct CopyTestContext : DataSetTestContextBase
{
  std::string source_dsn;
  std::string target_dsn;

  explicit CopyTestContext(std::vector<std::string> &list)
      : DataSetTestContextBase(list)
  {
    source_dsn = get_random_ds(3);
    target_dsn = get_random_ds(3);
    cleanup_list.push_back(source_dsn);
    cleanup_list.push_back(target_dsn);
  }

  void create_source_pds()
  {
    create_pds_at(source_dsn);
  }
  void create_source_pdse()
  {
    create_pdse_at(source_dsn);
  }
  void create_source_seq()
  {
    create_seq_at(source_dsn);
  }
  void create_target_pds()
  {
    create_pds_at(target_dsn);
  }
  void create_target_seq()
  {
    create_seq_at(target_dsn);
  }

  void write_source_member(const std::string &member, const std::string &data)
  {
    write_to_dsn(source_dsn + "(" + member + ")", data);
  }

  void write_source(const std::string &data)
  {
    write_to_dsn(source_dsn, data);
  }

  void write_target_member(const std::string &member, const std::string &data)
  {
    write_to_dsn(target_dsn + "(" + member + ")", data);
  }

  void write_target(const std::string &data)
  {
    write_to_dsn(target_dsn, data);
  }

  int copy(bool replace = false, bool delete_target_members = false)
  {
    ZDS zds = {0};
    ZDSCopyOptions options;
    options.replace = replace;
    options.delete_target_members = delete_target_members;
    return zds_copy_dsn(&zds, source_dsn, target_dsn, &options);
  }

  int copy_member(const std::string &src_mem, const std::string &tgt_mem, bool replace = false, bool delete_target_members = false)
  {
    ZDS zds = {0};
    ZDSCopyOptions options;
    options.replace = replace;
    options.delete_target_members = delete_target_members;
    return zds_copy_dsn(&zds, source_dsn + "(" + src_mem + ")", target_dsn + "(" + tgt_mem + ")", &options);
  }

  bool target_has_member(const std::string &member)
  {
    std::vector<ZDSMem> members;
    ZDS zds = {0};
    zds_list_members(&zds, target_dsn, members);
    for (const auto &mem : members)
    {
      std::string name = mem.name;
      zut_trim(name);
      if (name == member)
        return true;
    }
    return false;
  }

  bool source_has_member(const std::string &member)
  {
    std::vector<ZDSMem> members;
    ZDS zds = {0};
    zds_list_members(&zds, source_dsn, members);
    for (const auto &mem : members)
    {
      std::string name = mem.name;
      zut_trim(name);
      if (name == member)
        return true;
    }
    return false;
  }
};

void zds_tests()
{
  std::vector<std::string> created_dsns;

  describe("zds",
           [&]() -> void
           {
             TEST_OPTIONS cleanup_opts = {false, 30};
             afterAll([&]() -> void
                      {
                          for (const auto &dsn : created_dsns)
                          {
                            try
                            {
                              ZDS zds = {0};
                              zds_delete_dsn(&zds, dsn);
                            }
                            catch (...)
                            {
                            }
                          }
                          created_dsns.clear(); },
                      cleanup_opts);

             describe("list",
                      []() -> void
                      {
                        it("should list data sets with a given DSN",
                           []() -> void
                           {
                             int rc = 0;
                             ZDS zds = {0};
                             std::vector<ZDSEntry> entries;
                             std::string dsn = "SYS1.MACLIB";
                             rc = zds_list_data_sets(&zds, dsn, entries);
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             std::string found = "";
                             for (const auto &e : entries)
                             {
                               std::string trimmed_name = e.name;
                               zut_rtrim(trimmed_name);
                               if (trimmed_name == dsn)
                               {
                                 found = trimmed_name;
                               }
                             }
                             Expect(found).ToBe(dsn);
                           });

                        it("should list data sets with a given DSN and show attributes",
                           []() -> void
                           {
                             int rc = 0;
                             ZDS zds = {0};
                             std::vector<ZDSEntry> entries;
                             std::string dsn = "SYS1.MACLIB";
                             rc = zds_list_data_sets(&zds, dsn, entries, true);
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             ZDSEntry *found = nullptr;
                             for (auto &e : entries)
                             {
                               std::string trimmed_name = e.name;
                               zut_rtrim(trimmed_name);
                               if (trimmed_name == dsn)
                               {
                                 found = &e;
                               }
                             }
                             Expect(found != nullptr).ToBe(true);
                             Expect(zut_rtrim(found->name)).ToBe(dsn);
                             Expect(found->volser.length()).ToBeGreaterThan(0);
                           });

                        it("should find dsn (SYS1.MACLIB) based on a pattern: (SYS1.*)",
                           []() -> void
                           {
                             int rc = 0;
                             ZDS zds = {0};
                             std::vector<ZDSEntry> entries;
                             std::string dsn = "SYS1.MACLIB";
                             std::string pattern = "SYS1.*";
                             rc = zds_list_data_sets(&zds, pattern, entries);
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             std::string found = "";
                             for (const auto &e : entries)
                             {
                               std::string trimmed_name = e.name;
                               zut_rtrim(trimmed_name);
                               if (trimmed_name == dsn)
                               {
                                 found = trimmed_name;
                               }
                             }
                             Expect(found).ToBe(dsn);
                           });
                      });

             describe("list members",
                      [&]() -> void
                      {
                        it("should list all members when pattern is empty",
                           [&]() -> void
                           {
                             ListMembersTestContext tc(created_dsns);
                             tc.setup_pds();

                             ZDS zds = {0};
                             std::vector<ZDSMem> members;
                             int rc = zds_list_members(&zds, tc.pds_dsn, members, "");
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             Expect(members.size()).ToBe(4); // ABC1, ABC2, XYZ1, TEST
                           });

                        it("should list specific member by exact name",
                           [&]() -> void
                           {
                             ListMembersTestContext tc(created_dsns);
                             tc.setup_pds();

                             ZDS zds = {0};
                             std::vector<ZDSMem> members;
                             int rc = zds_list_members(&zds, tc.pds_dsn, members, "XYZ1");
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             Expect(members.size()).ToBe(1);
                             Expect(members[0].name).ToBe("XYZ1");
                           });

                        it("should list members matching wildcard *",
                           [&]() -> void
                           {
                             ListMembersTestContext tc(created_dsns);
                             tc.setup_pds();

                             ZDS zds = {0};
                             std::vector<ZDSMem> members;
                             int rc = zds_list_members(&zds, tc.pds_dsn, members, "ABC*");
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             Expect(members.size()).ToBe(2); // ABC1, ABC2

                             bool has_abc1 = members[0].name == "ABC1" || members[1].name == "ABC1";
                             bool has_abc2 = members[0].name == "ABC2" || members[1].name == "ABC2";
                             Expect(has_abc1).ToBe(true);
                             Expect(has_abc2).ToBe(true);
                           });

                        it("should list members matching wildcard ?",
                           [&]() -> void
                           {
                             ListMembersTestContext tc(created_dsns);
                             tc.setup_pds();

                             ZDS zds = {0};
                             std::vector<ZDSMem> members;
                             // Match exactly 4 characters starting with ABC
                             int rc = zds_list_members(&zds, tc.pds_dsn, members, "ABC?");
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             Expect(members.size()).ToBe(2); // ABC1, ABC2
                           });

                        it("should return empty list when no members match",
                           [&]() -> void
                           {
                             ListMembersTestContext tc(created_dsns);
                             tc.setup_pds();

                             ZDS zds = {0};
                             std::vector<ZDSMem> members;
                             int rc = zds_list_members(&zds, tc.pds_dsn, members, "NOMATCH*");
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             Expect(members.size()).ToBe(0);
                           });
                        it("should list members matching wildcard * at the beginning",
                           [&]() -> void
                           {
                             ListMembersTestContext tc(created_dsns);
                             tc.setup_pds(); // Creates ABC1, ABC2, XYZ1, TEST

                             ZDS zds = {0};
                             std::vector<ZDSMem> members;
                             int rc = zds_list_members(&zds, tc.pds_dsn, members, "*1");
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             Expect(members.size()).ToBe(2); // ABC1, XYZ1

                             bool has_abc1 = members[0].name == "ABC1" || members[1].name == "ABC1";
                             bool has_xyz1 = members[0].name == "XYZ1" || members[1].name == "XYZ1";
                             Expect(has_abc1).ToBe(true);
                             Expect(has_xyz1).ToBe(true);
                           });

                        it("should list members matching wildcard * in the middle",
                           [&]() -> void
                           {
                             ListMembersTestContext tc(created_dsns);
                             tc.setup_pds();

                             ZDS zds = {0};
                             std::vector<ZDSMem> members;
                             int rc = zds_list_members(&zds, tc.pds_dsn, members, "A*2");
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             Expect(members.size()).ToBe(1); // ABC2
                             Expect(members[0].name).ToBe("ABC2");
                           });

                        it("should list members matching a combination of ? and *",
                           [&]() -> void
                           {
                             ListMembersTestContext tc(created_dsns);
                             tc.setup_pds();

                             ZDS zds = {0};
                             std::vector<ZDSMem> members;
                             // Starts with any char, followed by B, then any trailing chars
                             int rc = zds_list_members(&zds, tc.pds_dsn, members, "?B*");
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             Expect(members.size()).ToBe(2); // ABC1, ABC2

                             bool has_abc1 = members[0].name == "ABC1" || members[1].name == "ABC1";
                             bool has_abc2 = members[0].name == "ABC2" || members[1].name == "ABC2";
                             Expect(has_abc1).ToBe(true);
                             Expect(has_abc2).ToBe(true);
                           });

                        it("should list members matching multiple ? wildcards for exact length",
                           [&]() -> void
                           {
                             ListMembersTestContext tc(created_dsns);
                             tc.setup_pds();

                             ZDS zds = {0};
                             std::vector<ZDSMem> members;
                             // Matches exactly 4 characters
                             int rc = zds_list_members(&zds, tc.pds_dsn, members, "????");
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             Expect(members.size()).ToBe(4); // ABC1, ABC2, XYZ1, TEST
                           });

                        it("should list members matching an embedded ? and trailing *",
                           [&]() -> void
                           {
                             ListMembersTestContext tc(created_dsns);
                             tc.setup_pds();

                             ZDS zds = {0};
                             std::vector<ZDSMem> members;
                             // 'X?Z*' matches XYZ1
                             int rc = zds_list_members(&zds, tc.pds_dsn, members, "X?Z*");
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             Expect(members.size()).ToBe(1);
                             Expect(members[0].name).ToBe("XYZ1");
                           });
                        it("should list members matching a lowercase pattern by converting it to uppercase",
                           [&]() -> void
                           {
                             ListMembersTestContext tc(created_dsns);
                             tc.setup_pds();

                             ZDS zds = {0};
                             std::vector<ZDSMem> members;

                             int rc = zds_list_members(&zds, tc.pds_dsn, members, "xyz1");
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             Expect(members.size()).ToBe(1);
                             Expect(members[0].name).ToBe("XYZ1");

                             members.clear();
                             rc = zds_list_members(&zds, tc.pds_dsn, members, "abc*");
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             Expect(members.size()).ToBe(2);

                             Expect(members[0].name == "ABC1" || members[1].name == "ABC1").ToBe(true);
                             Expect(members[0].name == "ABC2" || members[1].name == "ABC2").ToBe(true);
                           });
                        it("should list members with correct ISPF statistics attributes",
                           [&]() -> void
                           {
                             ListMembersTestContext tc(created_dsns);
                             tc.setup_pds();

                             ZDS zds = {0};
                             std::vector<ZDSMem> members;

                             int rc = zds_list_members(&zds, tc.pds_dsn, members, "ABC1", true);

                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             Expect(members.size()).ToBe(1);

                             ZDSMem &mem = members[0];
                             Expect(mem.name).ToBe("ABC1");
                             Expect(mem.user.empty()).ToBe(false);

                             Expect(mem.c4date.empty()).ToBe(false);
                             Expect(mem.m4date.empty()).ToBe(false);

                             Expect(mem.mtime.empty()).ToBe(false);

                             Expect(mem.c4date[2]).ToBe('/');
                             Expect(mem.c4date[5]).ToBe('/');

                             Expect(mem.sclm).ToBe(false);
                           });
                      });

             describe("source encoding tests",
                      []() -> void
                      {
                        it("should use default source encoding (UTF-8) when not specified",
                           []() -> void
                           {
                             ZDS zds = {0};
                             // Set target encoding but no source encoding
                             strcpy(zds.encoding_opts.codepage, "IBM-1047");
                             zds.encoding_opts.data_type = eDataTypeText;
                             // source_codepage should be empty/null

                             // The actual encoding conversion should use UTF-8 as source when source_codepage is empty
                             // Since we can't easily test the actual file operations without a real data set,
                             // we'll verify the struct is properly initialized
                             Expect(std::strlen(zds.encoding_opts.source_codepage)).ToBe(0);
                             Expect(std::strlen(zds.encoding_opts.codepage)).ToBe(8); // "IBM-1047"
                           });

                        it("should use specified source encoding when provided",
                           []() -> void
                           {
                             ZDS zds = {0};
                             // Set both target and source encoding
                             strcpy(zds.encoding_opts.codepage, "IBM-1047");
                             strcpy(zds.encoding_opts.source_codepage, "IBM-037");
                             zds.encoding_opts.data_type = eDataTypeText;

                             // Verify both encodings are set correctly
                             Expect(std::string(zds.encoding_opts.codepage)).ToBe("IBM-1047");
                             Expect(std::string(zds.encoding_opts.source_codepage)).ToBe("IBM-037");
                             Expect(zds.encoding_opts.data_type).ToBe(eDataTypeText);
                           });

                        it("should handle binary data type correctly with source encoding",
                           []() -> void
                           {
                             ZDS zds = {0};
                             strcpy(zds.encoding_opts.codepage, "binary");
                             strcpy(zds.encoding_opts.source_codepage, "UTF-8");
                             zds.encoding_opts.data_type = eDataTypeBinary;

                             // For binary data, encoding should not be used for conversion
                             Expect(std::string(zds.encoding_opts.codepage)).ToBe("binary");
                             Expect(std::string(zds.encoding_opts.source_codepage)).ToBe("UTF-8");
                             Expect(zds.encoding_opts.data_type).ToBe(eDataTypeBinary);
                           });

                        it("should handle empty source encoding gracefully",
                           []() -> void
                           {
                             ZDS zds = {0};
                             strcpy(zds.encoding_opts.codepage, "IBM-1047");
                             // Explicitly set source_codepage to empty
                             memset(zds.encoding_opts.source_codepage, 0, sizeof(zds.encoding_opts.source_codepage));
                             zds.encoding_opts.data_type = eDataTypeText;

                             // Should handle empty source encoding (will default to UTF-8 in actual conversion)
                             Expect(std::strlen(zds.encoding_opts.source_codepage)).ToBe(0);
                             Expect(std::string(zds.encoding_opts.codepage)).ToBe("IBM-1047");
                           });

                        it("should handle maximum length encoding names",
                           []() -> void
                           {
                             ZDS zds = {0};
                             // Test with maximum length encoding names (15 chars + null terminator)
                             std::string long_target = "IBM-1234567890A"; // 15 characters
                             std::string long_source = "UTF-1234567890B"; // 15 characters

                             strncpy(zds.encoding_opts.codepage, long_target.c_str(), sizeof(zds.encoding_opts.codepage) - 1);
                             strncpy(zds.encoding_opts.source_codepage, long_source.c_str(), sizeof(zds.encoding_opts.source_codepage) - 1);

                             // Ensure null termination
                             zds.encoding_opts.codepage[sizeof(zds.encoding_opts.codepage) - 1] = '\0';
                             zds.encoding_opts.source_codepage[sizeof(zds.encoding_opts.source_codepage) - 1] = '\0';

                             Expect(std::string(zds.encoding_opts.codepage)).ToBe(long_target);
                             Expect(std::string(zds.encoding_opts.source_codepage)).ToBe(long_source);
                           });

                        it("should preserve encoding settings through struct copy",
                           []() -> void
                           {
                             ZDS zds1 = {0};
                             strcpy(zds1.encoding_opts.codepage, "IBM-1047");
                             strcpy(zds1.encoding_opts.source_codepage, "IBM-037");
                             zds1.encoding_opts.data_type = eDataTypeText;
                             ZDS zds2 = zds1;
                             Expect(std::string(zds2.encoding_opts.codepage)).ToBe("IBM-1047");
                             Expect(std::string(zds2.encoding_opts.source_codepage)).ToBe("IBM-037");
                             Expect(zds2.encoding_opts.data_type).ToBe(eDataTypeText);
                           });

                        it("should validate encoding struct size assumptions",
                           []() -> void
                           {
                             // Verify the struct size is as expected for both fields
                             Expect(sizeof(((ZEncode *)0)->codepage)).ToBe(16);
                             Expect(sizeof(((ZEncode *)0)->source_codepage)).ToBe(16);

                             // Verify the fields are at expected offsets
                             ZEncode encode = {0};
                             strcpy(encode.codepage, "test1");
                             strcpy(encode.source_codepage, "test2");

                             Expect(std::string(encode.codepage)).ToBe("test1");
                             Expect(std::string(encode.source_codepage)).ToBe("test2");
                           });

                        it("should handle common encoding combinations",
                           []() -> void
                           {
                             ZDS zds = {0};
                             zds.encoding_opts.data_type = eDataTypeText;

                             // Test common mainframe encoding conversions
                             struct EncodingPair
                             {
                               const char *source;
                               const char *target;
                             };

                             EncodingPair pairs[] = {
                                 {"UTF-8", "IBM-1047"},     // UTF-8 to EBCDIC
                                 {"IBM-037", "UTF-8"},      // EBCDIC to UTF-8
                                 {"IBM-1047", "IBM-037"},   // EBCDIC to EBCDIC
                                 {"ISO8859-1", "IBM-1047"}, // ASCII to EBCDIC
                                 {"UTF-8", "binary"}        // Text to binary
                             };

                             for (const auto &pair : pairs)
                             {
                               memset(&zds.encoding_opts, 0, sizeof(zds.encoding_opts));
                               strcpy(zds.encoding_opts.source_codepage, pair.source);
                               strcpy(zds.encoding_opts.codepage, pair.target);
                               Expect(std::string(zds.encoding_opts.source_codepage)).ToBe(std::string(pair.source));
                               Expect(std::string(zds.encoding_opts.codepage)).ToBe(std::string(pair.target));
                             }
                           });
                      });
             describe("copy",
                      [&]() -> void
                      {
                        it("should copy PDS to PDS",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             tc.write_source_member("MEMBER", "Test data");
                             Expect(tc.copy()).ToBe(0);
                           });

                        it("should copy PDSE to nonexisting PDS (creates target)",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pdse();
                             tc.write_source_member("MEMBER", "Test data");
                             Expect(tc.copy()).ToBe(0);
                           });

                        it("should fail to copy PDS member to sequential data set",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             tc.write_source_member("MEMBER", "Member content");

                             ZDS zds = {0};
                             int rc = zds_copy_dsn(&zds, tc.source_dsn + "(MEMBER)", tc.target_dsn, nullptr);
                             Expect(rc).Not().ToBe(0);
                             Expect(std::string(zds.diag.e_msg)).ToContain("must specify a member name");
                           });

                        it("should copy sequential data set to sequential data set",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_seq();
                             tc.write_source("Sequential data");
                             Expect(tc.copy()).ToBe(0);
                           });

                        it("should copy member to member",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             tc.create_target_pds();
                             tc.write_source_member("SRCMEM", "Source member data");
                             Expect(tc.copy_member("SRCMEM", "TGTMEM")).ToBe(0);
                           });

                        it("should fail to copy sequential data set to PDS member",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_seq();
                             tc.write_source("Sequential data");

                             ZDS zds = {0};
                             int rc = zds_copy_dsn(&zds, tc.source_dsn, tc.target_dsn + "(MEMBER)", nullptr);
                             Expect(rc).Not().ToBe(0);
                             Expect(std::string(zds.diag.e_msg)).ToContain("must be a sequential data set");
                           });

                        it("should fail to copy PDS to existing sequential data set",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             tc.create_target_seq();
                             tc.write_source_member("MEMBER", "PDS member content");
                             tc.write_target("Sequential content");

                             ZDS zds = {0};
                             int rc = zds_copy_dsn(&zds, tc.source_dsn, tc.target_dsn, nullptr);
                             Expect(rc).Not().ToBe(0);
                             Expect(std::string(zds.diag.e_msg)).ToContain("must be a PDS");
                           });

                        it("should copy PDS with multiple members",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             for (int i = 1; i <= 3; i++)
                             {
                               tc.write_source_member("MEM" + std::to_string(i), "Data " + std::to_string(i));
                             }
                             Expect(tc.copy()).ToBe(0);
                           });

                        it("should fail when copying from nonexistent source",
                           [&]() -> void
                           {
                             ZDS zds = {0};
                             std::string source_dsn = "NONEXISTENT.DATASET.NAME";
                             std::string target_dsn = get_random_ds(3);
                             created_dsns.push_back(target_dsn);
                             int rc = zds_copy_dsn(&zds, source_dsn, target_dsn, nullptr);
                             Expect(rc).Not().ToBe(0);
                             Expect(std::string(zds.diag.e_msg)).ToContain("not found");
                           });

                        it("should preserve data set attributes when copying",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_seq();
                             tc.write_source("Test data");
                             Expect(tc.copy()).ToBe(0);

                             std::vector<ZDSEntry> source_entries;
                             std::vector<ZDSEntry> target_entries;
                             ZDS zds_list = {0};
                             zds_list_data_sets(&zds_list, tc.source_dsn, source_entries, true);
                             zds_list_data_sets(&zds_list, tc.target_dsn, target_entries, true);
                             Expect(source_entries.empty()).ToBe(false);
                             Expect(target_entries.empty()).ToBe(false);

                             const ZDSEntry &src = source_entries[0];
                             const ZDSEntry &tgt = target_entries[0];
                             Expect(tgt.recfm).ToBe(src.recfm);
                             Expect(tgt.lrecl).ToBe(src.lrecl);
                             Expect(tgt.blksize).ToBe(src.blksize);
                             Expect(tgt.spacu).ToBe(src.spacu);
                             Expect(tgt.primary).ToBe(src.primary);
                             Expect(tgt.secondary).ToBe(src.secondary);

                             ZDS read_zds{};
                             ZDSReadOpts read_opts{.zds = &read_zds, .dsname = tc.target_dsn};
                             std::string content;
                             zds_read(read_opts, content);
                             Expect(content.find("Test data") != std::string::npos).ToBe(true);
                           });

                        it("should fail to overwrite existing sequential data set without replace flag",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_seq();
                             tc.create_target_seq();
                             tc.write_source("Source data");
                             tc.write_target("Old target data");

                             ZDS zds = {0};
                             ZDSCopyOptions options;
                             options.replace = false;
                             int rc = zds_copy_dsn(&zds, tc.source_dsn, tc.target_dsn, &options);
                             Expect(rc).Not().ToBe(0);
                             Expect(std::string(zds.diag.e_msg)).ToContain("already exists");
                           });

                        it("should overwrite existing sequential data set with replace flag",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_seq();
                             tc.create_target_seq();
                             tc.write_source("Source data");
                             tc.write_target("Old target data");
                             Expect(tc.copy(true)).ToBe(0);
                           });

                        it("should skip existing members in PDS without replace flag",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             tc.create_target_pds();
                             tc.write_source_member("MEM1", "Source member");
                             tc.write_source_member("MEM2", "Source member 2");
                             tc.write_target_member("MEM1", "Old target member");

                             Expect(tc.copy(false)).ToBe(0);
                             Expect(tc.target_has_member("MEM2")).ToBe(true);
                           });

                        it("should replace existing members in PDS with replace flag",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             tc.create_target_pds();
                             tc.write_source_member("MEM1", "Source member");
                             tc.write_target_member("MEM1", "Old target member");
                             Expect(tc.copy(true)).ToBe(0);
                           });

                        it("should overwrite entire PDS with delete_target_members flag",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             tc.create_target_pds();
                             tc.write_source_member("MEM1", "Source member");
                             tc.write_target_member("MEM2", "Target member to be deleted");

                             ZDS zds = {0};
                             ZDSCopyOptions options;
                             options.delete_target_members = true;
                             int rc = zds_copy_dsn(&zds, tc.source_dsn, tc.target_dsn, &options);
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);

                             Expect(tc.target_has_member("MEM1")).ToBe(true);
                             Expect(tc.target_has_member("MEM2")).ToBe(false);
                           });

                        it("should fail to overwrite existing member without replace flag",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             tc.create_target_pds();
                             tc.write_source_member("MEM", "New source data");
                             tc.write_target_member("MEM", "Old target data");

                             ZDS zds = {0};
                             ZDSCopyOptions options;
                             options.replace = false;
                             int rc = zds_copy_dsn(&zds, tc.source_dsn + "(MEM)", tc.target_dsn + "(MEM)", &options);
                             Expect(rc).Not().ToBe(0);
                             Expect(std::string(zds.diag.e_msg)).ToContain("already exists");
                           });

                        it("should overwrite existing member with replace flag",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             tc.create_target_pds();
                             tc.write_source_member("MEM", "New source data");
                             tc.write_target_member("MEM", "Old target data");
                             Expect(tc.copy_member("MEM", "MEM", true)).ToBe(0);
                           });

                        it("should copy member to another member in the same PDS",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             tc.write_source_member("SRC", "Source member data");

                             // Copy within same PDS
                             ZDS zds = {0};
                             ZDSCopyOptions options;
                             options.replace = false;
                             int rc = zds_copy_dsn(&zds, tc.source_dsn + "(SRC)", tc.source_dsn + "(DST)", &options);
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);

                             // Verify both members exist
                             Expect(tc.source_has_member("SRC")).ToBe(true);
                             Expect(tc.source_has_member("DST")).ToBe(true);
                           });

                        it("should copy and rename member in the same PDS with replace",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             tc.write_source_member("ORIG", "Original data");
                             tc.write_source_member("COPY", "Old copy data");

                             // Copy and replace within same PDS
                             ZDS zds = {0};
                             ZDSCopyOptions options;
                             options.replace = true;
                             int rc = zds_copy_dsn(&zds, tc.source_dsn + "(ORIG)", tc.source_dsn + "(COPY)", &options);
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                           });

                        it("should copy empty PDS",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             Expect(tc.copy()).ToBe(0);
                           });

                        it("should copy empty sequential data set",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_seq();
                             Expect(tc.copy()).ToBe(0);
                           });

                        it("should copy PDSE to PDSE",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pdse();
                             tc.write_source_member("MEMBER", "PDSE data");
                             Expect(tc.copy()).ToBe(0);
                           });

                        it("should copy PDS to new target without replace flag",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             tc.write_source_member("MEMBER", "Test data");
                             Expect(tc.copy(false)).ToBe(0);
                           });

                        it("should copy sequential to new target without replace flag",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_seq();
                             tc.write_source("Sequential data");
                             Expect(tc.copy(false)).ToBe(0);
                           });

                        it("should add new members to existing PDS without replace flag",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             tc.create_target_pds();
                             tc.write_source_member("MEM1", "Source data");
                             tc.write_target_member("MEM2", "Target data");

                             Expect(tc.copy(false)).ToBe(0);
                             Expect(tc.target_has_member("MEM1")).ToBe(true);
                             Expect(tc.target_has_member("MEM2")).ToBe(true);
                           });

                        it("should fail to copy sequential to PDS member even with replace",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_seq();
                             tc.create_target_pds();
                             tc.write_source("Sequential source data");
                             tc.write_target_member("EXISTING", "Old member data");

                             ZDS zds = {0};
                             ZDSCopyOptions options;
                             options.replace = true;
                             int rc = zds_copy_dsn(&zds, tc.source_dsn, tc.target_dsn + "(EXISTING)", &options);
                             Expect(rc).Not().ToBe(0);
                             Expect(std::string(zds.diag.e_msg)).ToContain("must be a sequential data set");
                           });

                        it("should set member_created when copying to new member",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             tc.create_target_pds();
                             tc.write_source_member("SRC", "source content");

                             ZDS zds = {0};
                             ZDSCopyOptions options;
                             options.replace = false;
                             options.delete_target_members = false;
                             int rc = zds_copy_dsn(&zds, tc.source_dsn + "(SRC)", tc.target_dsn + "(NEW)", &options);
                             Expect(rc).ToBe(0);
                             Expect(options.target_created).ToBe(false); // PDS already existed
                             Expect(options.member_created).ToBe(true);  // Member was newly created
                           });

                        it("should not set member_created when overwriting existing member",
                           [&]() -> void
                           {
                             CopyTestContext tc(created_dsns);
                             tc.create_source_pds();
                             tc.create_target_pds();
                             tc.write_source_member("SRC", "source content");
                             tc.write_target_member("EXIST", "existing content");

                             ZDS zds = {0};
                             ZDSCopyOptions options;
                             options.replace = true;
                             options.delete_target_members = false;
                             int rc = zds_copy_dsn(&zds, tc.source_dsn + "(SRC)", tc.target_dsn + "(EXIST)", &options);
                             Expect(rc).ToBe(0);
                             Expect(options.target_created).ToBe(false);
                             Expect(options.member_created).ToBe(false); // Member already existed
                           });
                      });

             describe("rename data sets",
                      [&]() -> void
                      {
                        it("should fail if source or target data sets are empty",
                           []() -> void
                           {
                             ZDS zds = {0};
                             std::string target = get_random_ds(3);
                             int rc = zds_rename_dsn(&zds, "", target);
                             Expect(rc).ToBe(RTNCD_FAILURE);
                             Expect(std::string(zds.diag.e_msg)).ToContain("Data set names must be valid");

                             ZDS zds2 = {0};
                             rc = zds_rename_dsn(&zds2, "USER.TEST", "");
                             Expect(rc).ToBe(RTNCD_FAILURE);
                             Expect(std::string(zds2.diag.e_msg)).ToContain("Data set names must be valid");
                           });

                        it("should fail if target data set name exceeds max length",
                           []() -> void
                           {
                             ZDS zds = {0};
                             std::string longName = "USER.TEST.TEST.TEST.TEST.TEST.TEST.TEST.TEST.TEST.TEST.TEST.TEST";
                             std::string source = get_random_ds(3);
                             int rc = zds_rename_dsn(&zds, source, longName);
                             Expect(rc).ToBe(RTNCD_FAILURE);
                             Expect(std::string(zds.diag.e_msg)).ToContain("Target data set name exceeds max character length of 44");
                           });

                        it("should fail if source data set does not exist",
                           []() -> void
                           {
                             ZDS zds = {0};
                             std::string target = get_random_ds(3);
                             std::string source = get_random_ds(3);
                             int rc = zds_rename_dsn(&zds, source, target);
                             Expect(rc).ToBe(RTNCD_FAILURE);
                             Expect(std::string(zds.diag.e_msg)).ToContain("Source data set does not exist");
                           });

                        it("should fail if target data set already exists",
                           [&]() -> void
                           {
                             ZDS zds = {0};
                             DS_ATTRIBUTES attr{};

                             attr.dsorg = "PS";
                             attr.recfm = "FB";
                             attr.lrecl = 80;
                             attr.blksize = 0;
                             attr.alcunit = "TRACKS";
                             attr.primary = 1;
                             attr.secondary = 1;
                             attr.dirblk = 0;

                             std::string source = get_random_ds(3);
                             std::string target = get_random_ds(3);
                             created_dsns.push_back(source);
                             created_dsns.push_back(target);

                             create_seq(&zds, source);
                             create_seq(&zds, target);
                             int rc = zds_rename_dsn(&zds, source, target);
                             Expect(rc).ToBe(RTNCD_FAILURE);
                             Expect(std::string(zds.diag.e_msg)).ToContain("Target data set name already exists");
                           });

                        it("should rename dataset successfully when valid",
                           [&]() -> void
                           {
                             ZDS zds = {0};
                             DS_ATTRIBUTES attr{};

                             attr.dsorg = "PS";
                             attr.recfm = "FB";
                             attr.lrecl = 80;
                             attr.blksize = 0;
                             attr.alcunit = "TRACKS";
                             attr.primary = 1;
                             attr.secondary = 1;
                             attr.dirblk = 0;
                             std::string before = get_random_ds(3);
                             std::string after = get_random_ds(3);
                             created_dsns.push_back(after); // before is renamed to after; clean up final name

                             create_seq(&zds, before);
                             int rc = zds_rename_dsn(&zds, before, after);
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                           });
                      });
             describe("rename members",
                      [&]() -> void
                      {
                        const std::string M1 = "M1";
                        const std::string M2 = "M2";
                        DS_ATTRIBUTES attr{};

                        attr.dsorg = "PO";
                        attr.recfm = "FB";
                        attr.lrecl = 80;
                        attr.blksize = 6160;
                        attr.alcunit = "TRACKS";
                        attr.primary = 1;
                        attr.secondary = 1;
                        attr.dirblk = 10;
                        std::string response;

                        it("should fail if data set name is empty",
                           [&]() -> void
                           {
                             ZDS zds = {0};
                             std::string ds = get_random_ds(3);
                             int rc = zds_rename_members(&zds, "", M1, M2);
                             Expect(rc).ToBe(RTNCD_FAILURE);
                             Expect(std::string(zds.diag.e_msg)).ToContain("Data set name must be valid");
                           });

                        it("should fail if member names are empty",
                           [&]() -> void
                           {
                             ZDS zds = {0};
                             std::string ds = get_random_ds(3);
                             created_dsns.push_back(ds);
                             int rc = zds_create_dsn(&zds, ds, attr, response);
                             Expect(rc).ToBe(0);

                             rc = zds_rename_members(&zds, ds, "", M2);
                             Expect(rc).ToBe(RTNCD_FAILURE);
                             Expect(std::string(zds.diag.e_msg)).ToContain("Member name cannot be empty");

                             ZDS zds2 = {0};
                             rc = zds_rename_members(&zds2, ds, M1, "");
                             Expect(rc).ToBe(RTNCD_FAILURE);
                             Expect(std::string(zds.diag.e_msg)).ToContain("Member name cannot be empty");
                           });
                        it("should fail if member name is too long",
                           [&]() -> void
                           {
                             ZDS zds = {0};
                             std::string longName = "USER.TEST.TEST.TEST";
                             std::string ds = get_random_ds(3);
                             created_dsns.push_back(ds);
                             int rc = zds_create_dsn(&zds, ds, attr, response);
                             std::string empty = "";
                             ZDSWriteOpts write_opts{.zds = &zds, .dsname = ds + "(M1)"};
                             rc = zds_write(write_opts, empty);
                             Expect(rc).ToBe(0);
                             rc = zds_rename_members(&zds, ds, M1, longName);
                             Expect(rc).ToBe(RTNCD_FAILURE);
                             Expect(std::string(zds.diag.e_msg)).ToContain("Member name must start with A-Z,#,@,$ and contain only A-Z,0-9,#,@,$ (max 8 chars)");
                           });

                        it("should fail if data set does not exist",
                           [&]() -> void
                           {
                             ZDS zds = {0};
                             std::string ds = get_random_ds(3);
                             int rc = zds_rename_members(&zds, ds, M1, M2);
                             Expect(rc).ToBe(RTNCD_FAILURE);
                             Expect(std::string(zds.diag.e_msg)).ToContain("Data set does not exist");
                           });

                        it("should fail if source member does not exist",
                           [&]() -> void
                           {
                             ZDS zds = {0};
                             std::string ds = get_random_ds(3);
                             created_dsns.push_back(ds);
                             int rc = zds_create_dsn(&zds, ds, attr, response);
                             rc = zds_rename_members(&zds, ds, M1, "M3");
                             Expect(rc).ToBe(RTNCD_FAILURE);
                             Expect(std::string(zds.diag.e_msg)).ToContain("Source member does not exist");
                           });

                        it("should fail if target member already exists",
                           [&]() -> void
                           {
                             ZDS zds = {};

                             std::string ds = get_random_ds(3);
                             created_dsns.push_back(ds);
                             int rc = zds_create_dsn(&zds, ds, attr, response);

                             std::string empty = "";
                             ZDSWriteOpts write_opts{.zds = &zds, .dsname = ds + "(M1)"};
                             rc = zds_write(write_opts, empty);
                             Expect(rc).ToBe(0);
                             memset(zds.etag, 0, 8);
                             ZDSWriteOpts write_opts2{.zds = &zds, .dsname = ds + "(M2)"};
                             rc = zds_write(write_opts2, empty);
                             Expect(rc).ToBe(0);

                             rc = zds_rename_members(&zds, ds, M1, M2);
                             Expect(rc).ToBe(RTNCD_FAILURE);
                             Expect(std::string(zds.diag.e_msg)).ToContain("Target member already exists");
                           });

                        it("should rename dataset successfully when valid",
                           [&]() -> void
                           {
                             ZDS zds = {};
                             std::string ds = get_random_ds(3);
                             created_dsns.push_back(ds);
                             int rc = zds_create_dsn(&zds, ds, attr, response);

                             std::string empty = "";
                             ZDSWriteOpts write_opts{.zds = &zds, .dsname = ds + "(M1)"};
                             rc = zds_write(write_opts, empty);
                             Expect(rc).ToBe(0);

                             rc = zds_rename_members(&zds, ds, M1, "M3");
                             Expect(rc).ToBe(0);
                             rc = zds_delete_dsn(&zds, ds);
                           });

                        it("should fail if member name begins with a digit",
                           [&]() -> void
                           {
                             ZDS zds = {};
                             std::string ds = get_random_ds(3);
                             created_dsns.push_back(ds);
                             int rc = zds_create_dsn(&zds, ds, attr, response);

                             std::string empty = "";
                             ZDSWriteOpts write_opts{.zds = &zds, .dsname = ds + "(M1)"};
                             rc = zds_write(write_opts, empty);
                             Expect(rc).ToBe(0);

                             rc = zds_rename_members(&zds, ds, M1, "123");
                             Expect(rc).ToBe(RTNCD_FAILURE);
                             Expect(std::string(zds.diag.e_msg)).ToContain("Member name must start with A-Z,#,@,$ and contain only A-Z,0-9,#,@,$ (max 8 chars)");
                           });
                      });

             describe("list VSAM",
                      [&]() -> void
                      {
                        std::string user = get_user();
                        std::string base = get_random_ds(2, user);
                        std::string ksds_dsn = base + ".VSAM.KSDS";
                        std::string ksds_aix_dsn = base + ".VSAM.KAIX";
                        std::string ksds_path_dsn = base + ".VSAM.KPTH";
                        std::string esds_dsn = base + ".VSAM.ESDS";
                        std::string esds_aix_dsn = base + ".VSAM.EAIX";
                        std::string esds_path_dsn = base + ".VSAM.EPTH";

                        auto submit_and_wait = [](const std::string &jcl, int max_polls = 600, bool check_rc = false)
                        {
                          ZJB zjb = {0};
                          std::string jobid;
                          int rc = zjb_submit(&zjb, jcl, jobid);
                          if (rc != 0)
                            throw std::runtime_error("Failed to submit JCL: " + std::string(zjb.diag.e_msg));
                          TestLog("Submitted job " + jobid + " (" + std::to_string(jcl.size()) + " bytes)");
                          std::string correlator(zjb.correlator, sizeof(zjb.correlator));
                          ZJob final_job = {};
                          for (int i = 0; i < max_polls; ++i)
                          {
                            ZJB zjb_v = {0};
                            final_job = {};
                            int vrc = zjb_view(&zjb_v, correlator, final_job);
                            if (vrc != 0)
                              throw std::runtime_error("Failed to view job: " + std::string(zjb_v.diag.e_msg));
                            if (!final_job.retcode.empty())
                              break;
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                          }
                          TestLog("Job " + jobid + " retcode='" + final_job.retcode + "'");
                          if (check_rc && final_job.retcode.find("CC 0000") == std::string::npos)
                            throw std::runtime_error("Job " + jobid + " failed: retcode='" + final_job.retcode + "'");
                        };

                        auto find_entry = [](std::vector<ZDSEntry> &entries, const std::string &dsn) -> ZDSEntry *
                        {
                          for (auto &e : entries)
                          {
                            std::string trimmed = e.name;
                            zut_rtrim(trimmed);
                            if (trimmed == dsn)
                              return &e;
                          }
                          return nullptr;
                        };

                        TEST_OPTIONS vsam_opts = {false, 60};

                        beforeAll([&]() -> void
                                  {
                                    std::string cleanup_jcl;
                                    cleanup_jcl += "//CLEANUP$ JOB IZUACCT\n";
                                    cleanup_jcl += "//STEP1    EXEC PGM=IDCAMS\n";
                                    cleanup_jcl += "//SYSPRINT DD SYSOUT=*\n";
                                    cleanup_jcl += "//SYSIN    DD *\n";
                                    cleanup_jcl += "  DELETE " + ksds_path_dsn + " PURGE\n";
                                    cleanup_jcl += "  DELETE " + ksds_aix_dsn + " PURGE\n";
                                    cleanup_jcl += "  DELETE " + ksds_dsn + " CLUSTER PURGE\n";
                                    cleanup_jcl += "  DELETE " + esds_path_dsn + " PURGE\n";
                                    cleanup_jcl += "  DELETE " + esds_aix_dsn + " PURGE\n";
                                    cleanup_jcl += "  DELETE " + esds_dsn + " CLUSTER PURGE\n";
                                    cleanup_jcl += "  SET MAXCC = 0\n";
                                    cleanup_jcl += "/*\n";
                                    submit_and_wait(cleanup_jcl, 200);

                                    std::string setup_jcl;
                                    setup_jcl += "//VSAMSET$ JOB IZUACCT\n";
                                    setup_jcl += "//STEP1    EXEC PGM=IDCAMS\n";
                                    setup_jcl += "//SYSPRINT DD SYSOUT=*\n";
                                    setup_jcl += "//SYSIN    DD *\n";
                                    setup_jcl += "  DEFINE CLUSTER ( -\n";
                                    setup_jcl += "    NAME(" + ksds_dsn + ") -\n";
                                    setup_jcl += "    INDEXED -\n";
                                    setup_jcl += "    KEYS(8 0) -\n";
                                    setup_jcl += "    RECORDSIZE(80 80) -\n";
                                    setup_jcl += "    TRACKS(5 5) -\n";
                                    setup_jcl += "    SHAREOPTIONS(2 3) )\n";
                                    setup_jcl += "  DEFINE AIX ( -\n";
                                    setup_jcl += "    NAME(" + ksds_aix_dsn + ") -\n";
                                    setup_jcl += "    RELATE(" + ksds_dsn + ") -\n";
                                    setup_jcl += "    KEYS(8 32) -\n";
                                    setup_jcl += "    RECORDSIZE(80 80) -\n";
                                    setup_jcl += "    TRACKS(5 5) -\n";
                                    setup_jcl += "    SHAREOPTIONS(2 3) -\n";
                                    setup_jcl += "    UPGRADE )\n";
                                    setup_jcl += "  DEFINE PATH ( -\n";
                                    setup_jcl += "    NAME(" + ksds_path_dsn + ") -\n";
                                    setup_jcl += "    PATHENTRY(" + ksds_aix_dsn + ") -\n";
                                    setup_jcl += "    UPDATE )\n";
                                    setup_jcl += "  DEFINE CLUSTER ( -\n";
                                    setup_jcl += "    NAME(" + esds_dsn + ") -\n";
                                    setup_jcl += "    NONINDEXED -\n";
                                    setup_jcl += "    RECORDSIZE(80 80) -\n";
                                    setup_jcl += "    TRACKS(5 5) -\n";
                                    setup_jcl += "    SHAREOPTIONS(2 3) )\n";
                                    setup_jcl += "  DEFINE AIX ( -\n";
                                    setup_jcl += "    NAME(" + esds_aix_dsn + ") -\n";
                                    setup_jcl += "    RELATE(" + esds_dsn + ") -\n";
                                    setup_jcl += "    KEYS(8 0) -\n";
                                    setup_jcl += "    RECORDSIZE(80 80) -\n";
                                    setup_jcl += "    TRACKS(5 5) -\n";
                                    setup_jcl += "    SHAREOPTIONS(2 3) -\n";
                                    setup_jcl += "    UPGRADE )\n";
                                    setup_jcl += "  DEFINE PATH ( -\n";
                                    setup_jcl += "    NAME(" + esds_path_dsn + ") -\n";
                                    setup_jcl += "    PATHENTRY(" + esds_aix_dsn + ") -\n";
                                    setup_jcl += "    UPDATE )\n";
                                    setup_jcl += "/*\n";
                                    submit_and_wait(setup_jcl, 600, true); },
                                  vsam_opts);

                        afterAll([ksds_dsn, esds_dsn, &submit_and_wait]() -> void
                                 {
                                   std::string del_jcl;
                                   del_jcl += "//VSAMDEL$ JOB IZUACCT\n";
                                   del_jcl += "//STEP1    EXEC PGM=IDCAMS\n";
                                   del_jcl += "//SYSPRINT DD SYSOUT=*\n";
                                   del_jcl += "//SYSIN    DD *\n";
                                   del_jcl += "  DELETE " + ksds_dsn + " CLUSTER PURGE\n";
                                   del_jcl += "  DELETE " + esds_dsn + " CLUSTER PURGE\n";
                                   del_jcl += "/*\n";
                                   submit_and_wait(del_jcl, 200); },
                                 vsam_opts);

                        it("should report VS dsorg and *VSAM* volser for KSDS cluster",
                           [&]() -> void
                           {
                             ZDS zds = {0};
                             std::vector<ZDSEntry> entries;
                             int rc = zds_list_data_sets(&zds, ksds_dsn, entries, true);
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             ZDSEntry *found = find_entry(entries, ksds_dsn);
                             Expect(found != nullptr).ToBe(true);
                             Expect(found->dsorg).ToBe(std::string(ZDS_DSORG_VSAM));
                             Expect(found->volser).ToBe(std::string(ZDS_VOLSER_VSAM));
                           });

                        it("should report empty dsorg and *AIX* volser for KSDS alternate index",
                           [&]() -> void
                           {
                             ZDS zds = {0};
                             std::vector<ZDSEntry> entries;
                             int rc = zds_list_data_sets(&zds, ksds_aix_dsn, entries, true);
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             ZDSEntry *found = find_entry(entries, ksds_aix_dsn);
                             Expect(found != nullptr).ToBe(true);
                             Expect(found->dsorg).ToBe(std::string(""));
                             Expect(found->volser).ToBe(std::string(ZDS_VOLSER_AIX));
                           });

                        it("should report empty dsorg and *PATH* volser for KSDS path",
                           [&]() -> void
                           {
                             ZDS zds = {0};
                             std::vector<ZDSEntry> entries;
                             int rc = zds_list_data_sets(&zds, ksds_path_dsn, entries, true);
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             ZDSEntry *found = find_entry(entries, ksds_path_dsn);
                             Expect(found != nullptr).ToBe(true);
                             Expect(found->dsorg).ToBe(std::string(""));
                             Expect(found->volser).ToBe(std::string(ZDS_VOLSER_PATH));
                           });

                        it("should report VS dsorg and *VSAM* volser for ESDS cluster",
                           [&]() -> void
                           {
                             ZDS zds = {0};
                             std::vector<ZDSEntry> entries;
                             int rc = zds_list_data_sets(&zds, esds_dsn, entries, true);
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             ZDSEntry *found = find_entry(entries, esds_dsn);
                             Expect(found != nullptr).ToBe(true);
                             Expect(found->dsorg).ToBe(std::string(ZDS_DSORG_VSAM));
                             Expect(found->volser).ToBe(std::string(ZDS_VOLSER_VSAM));
                           });

                        it("should report empty dsorg and *AIX* volser for ESDS alternate index",
                           [&]() -> void
                           {
                             ZDS zds = {0};
                             std::vector<ZDSEntry> entries;
                             int rc = zds_list_data_sets(&zds, esds_aix_dsn, entries, true);
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             ZDSEntry *found = find_entry(entries, esds_aix_dsn);
                             Expect(found != nullptr).ToBe(true);
                             Expect(found->dsorg).ToBe(std::string(""));
                             Expect(found->volser).ToBe(std::string(ZDS_VOLSER_AIX));
                           });

                        it("should report empty dsorg and *PATH* volser for ESDS path",
                           [&]() -> void
                           {
                             ZDS zds = {0};
                             std::vector<ZDSEntry> entries;
                             int rc = zds_list_data_sets(&zds, esds_path_dsn, entries, true);
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             ZDSEntry *found = find_entry(entries, esds_path_dsn);
                             Expect(found != nullptr).ToBe(true);
                             Expect(found->dsorg).ToBe(std::string(""));
                             Expect(found->volser).ToBe(std::string(ZDS_VOLSER_PATH));
                           });
                      });
             describe("read",
                      [&]() -> void
                      {
                        it("should read from a sequential data set",
                           [&]() -> void
                           {
                             std::string dsn = get_random_ds(3);
                             created_dsns.push_back(dsn);
                             ZDS zds = {0};
                             create_seq(&zds, dsn);
                             write_to_dsn(dsn, "hello world");

                             ZDS read_zds{};
                             ZDSReadOpts read_opts{.zds = &read_zds, .dsname = dsn};
                             std::string content;
                             int rc = zds_read(read_opts, content);
                             ExpectWithContext(rc, read_zds.diag.e_msg).ToBe(0);
                             Expect(content.find("hello world") != std::string::npos).ToBe(true);
                           });

                        it("should read from a PDS member",
                           [&]() -> void
                           {
                             std::string dsn = get_random_ds(3);
                             created_dsns.push_back(dsn);
                             ZDS zds = {0};
                             create_pds(&zds, dsn);
                             write_to_dsn(dsn + "(TESTMEM)", "member data");

                             ZDS read_zds{};
                             ZDSReadOpts read_opts{.zds = &read_zds, .dsname = dsn + "(TESTMEM)"};
                             std::string content;
                             int rc = zds_read(read_opts, content);
                             ExpectWithContext(rc, read_zds.diag.e_msg).ToBe(0);
                             Expect(content.find("member data") != std::string::npos).ToBe(true);
                           });

                        it("should fail to read from a non-existent data set",
                           []() -> void
                           {
                             ZDS read_zds{};
                             ZDSReadOpts read_opts{.zds = &read_zds, .dsname = "NONEXIST.DATASET.NAME"};
                             std::string content;
                             int rc = zds_read(read_opts, content);
                             Expect(rc).Not().ToBe(0);
                           });

                        it("should fail when neither dsname nor ddname is provided",
                           []() -> void
                           {
                             ZDS read_zds{};
                             ZDSReadOpts read_opts{.zds = &read_zds};
                             std::string content;
                             int rc = zds_read(read_opts, content);
                             Expect(rc).ToBe(RTNCD_FAILURE);
                             Expect(std::string(read_zds.diag.e_msg)).ToContain("Either a dsname or ddname must be provided");
                           });

                        it("should throw invalid_argument when ZDS pointer is null",
                           []() -> void
                           {
                             ZDSReadOpts read_opts{.zds = nullptr, .dsname = "SOME.DSN"};
                             std::string content;
                             bool caught = false;
                             try
                             {
                               zds_read(read_opts, content);
                             }
                             catch (const std::invalid_argument &e)
                             {
                               caught = true;
                               Expect(std::string(e.what())).ToContain("zds_read: valid ZDS pointer is required in ZDSReadOpts.zds");
                             }
                             Expect(caught).ToBe(true);
                           });

                        it("should read binary data from a sequential data set",
                           [&]() -> void
                           {
                             std::string dsn = get_random_ds(3);
                             created_dsns.push_back(dsn);
                             ZDS zds = {0};
                             create_seq(&zds, dsn);
                             write_to_dsn(dsn, "binary test");

                             ZDS read_zds{};
                             read_zds.encoding_opts.data_type = eDataTypeBinary;
                             ZDSReadOpts read_opts{.zds = &read_zds, .dsname = dsn};
                             std::string content;
                             int rc = zds_read(read_opts, content);
                             ExpectWithContext(rc, read_zds.diag.e_msg).ToBe(0);
                             Expect(content.empty()).ToBe(false);
                           });

                        it("should read with encoding (IBM-1047 to UTF-8)",
                           [&]() -> void
                           {
                             std::string dsn = get_random_ds(3);
                             created_dsns.push_back(dsn);
                             ZDS zds = {0};
                             create_seq(&zds, dsn);
                             write_to_dsn(dsn, "encoded data");

                             ZDS read_zds{};
                             strcpy(read_zds.encoding_opts.codepage, "IBM-1047");
                             read_zds.encoding_opts.data_type = eDataTypeText;
                             ZDSReadOpts read_opts{.zds = &read_zds, .dsname = dsn};
                             std::string content;
                             int rc = zds_read(read_opts, content);
                             ExpectWithContext(rc, read_zds.diag.e_msg).ToBe(0);
                             Expect(content.empty()).ToBe(false);
                           });

                        it("should read with explicit source encoding (local-encoding)",
                           [&]() -> void
                           {
                             std::string dsn = get_random_ds(3);
                             created_dsns.push_back(dsn);
                             ZDS zds = {0};
                             create_seq(&zds, dsn);
                             write_to_dsn(dsn, "local enc data");

                             ZDS read_zds{};
                             strcpy(read_zds.encoding_opts.codepage, "ISO8859-1");
                             strcpy(read_zds.encoding_opts.source_codepage, "IBM-1047");
                             read_zds.encoding_opts.data_type = eDataTypeText;
                             ZDSReadOpts read_opts{.zds = &read_zds, .dsname = dsn};
                             std::string content;
                             int rc = zds_read(read_opts, content);
                             ExpectWithContext(rc, read_zds.diag.e_msg).ToBe(0);
                             Expect(content.empty()).ToBe(false);
                           });

                        it("should default source encoding to UTF-8 when source_codepage is empty",
                           [&]() -> void
                           {
                             std::string dsn = get_random_ds(3);
                             created_dsns.push_back(dsn);
                             ZDS zds = {0};
                             create_seq(&zds, dsn);
                             write_to_dsn(dsn, "default enc");

                             // Read with codepage=UTF-8 but no source_codepage;
                             // defaults to UTF-8 source
                             ZDS default_zds{};
                             strcpy(default_zds.encoding_opts.codepage, "UTF-8");
                             default_zds.encoding_opts.data_type = eDataTypeText;
                             ZDSReadOpts read_default_opts{.zds = &default_zds, .dsname = dsn};
                             std::string content_default;
                             int rc = zds_read(read_default_opts, content_default);
                             ExpectWithContext(rc, default_zds.diag.e_msg).ToBe(0);

                             // Read again with explicit source_codepage=UTF-8
                             ZDS explicit_zds{};
                             strcpy(explicit_zds.encoding_opts.codepage, "UTF-8");
                             strcpy(explicit_zds.encoding_opts.source_codepage, "UTF-8");
                             explicit_zds.encoding_opts.data_type = eDataTypeText;
                             ZDSReadOpts read_explicit_opts{.zds = &explicit_zds, .dsname = dsn};
                             std::string content_explicit;
                             rc = zds_read(read_explicit_opts, content_explicit);
                             ExpectWithContext(rc, explicit_zds.diag.e_msg).ToBe(0);

                             // Both should produce the same result
                             Expect(content_default).ToBe(content_explicit);
                           });

                        it("should produce a consistent etag after write and read",
                           [&]() -> void
                           {
                             std::string dsn = get_random_ds(3);
                             created_dsns.push_back(dsn);
                             ZDS zds = {0};
                             create_seq(&zds, dsn);

                             std::string data = "etag test data";
                             ZDSWriteOpts write_opts{.zds = &zds, .dsname = dsn};
                             int rc = zds_write(write_opts, data);
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             std::string write_etag(zds.etag);
                             Expect(write_etag.empty()).ToBe(false);

                             // Read back and compute etag independently
                             ZDS read_zds{};
                             ZDSReadOpts read_opts{.zds = &read_zds, .dsname = dsn};
                             std::string content;
                             rc = zds_read(read_opts, content);
                             ExpectWithContext(rc, read_zds.diag.e_msg).ToBe(0);

                             std::stringstream etag_stream;
                             etag_stream << std::hex << zut_calc_adler32_checksum(content);
                             Expect(etag_stream.str()).ToBe(write_etag);
                           });

                        it("should detect etag mismatch on write (conflict resolution)",
                           [&]() -> void
                           {
                             std::string dsn = get_random_ds(3);
                             created_dsns.push_back(dsn);
                             ZDS zds = {0};
                             create_seq(&zds, dsn);

                             std::string data = "original content";
                             ZDSWriteOpts write_opts{.zds = &zds, .dsname = dsn};
                             int rc = zds_write(write_opts, data);
                             ExpectWithContext(rc, zds.diag.e_msg).ToBe(0);
                             std::string valid_etag(zds.etag);

                             // Overwrite with new content (changes the actual etag on disk)
                             ZDS zds2 = {0};
                             std::string data2 = "modified content";
                             ZDSWriteOpts write_opts2{.zds = &zds2, .dsname = dsn};
                             rc = zds_write(write_opts2, data2);
                             ExpectWithContext(rc, zds2.diag.e_msg).ToBe(0);

                             // Attempt to write using the stale (first) etag — should fail
                             ZDS zds3 = {0};
                             strcpy(zds3.etag, valid_etag.c_str());
                             std::string data3 = "conflicting write";
                             ZDSWriteOpts write_opts3{.zds = &zds3, .dsname = dsn};
                             rc = zds_write(write_opts3, data3);
                             Expect(rc).Not().ToBe(0);
                             Expect(std::string(zds3.diag.e_msg)).ToContain("Etag mismatch");
                           });

                        it("should read an uncataloged data set using volser (dynamic allocation)",
                           [&]() -> void
                           {
                             std::string dsn = get_random_ds(3);
                             created_dsns.push_back(dsn);
                             ZDS zds = {0};
                             create_seq(&zds, dsn);
                             write_to_dsn(dsn, "volser test");

                             // Look up the volser for this data set
                             std::vector<ZDSEntry> entries;
                             ZDS list_zds = {0};
                             int rc = zds_list_data_sets(&list_zds, dsn, entries, true);
                             ExpectWithContext(rc, list_zds.diag.e_msg).ToBe(0);
                             Expect(entries.empty()).ToBe(false);
                             std::string volser = entries[0].volser;
                             Expect(volser.empty()).ToBe(false);

                             // Dynamically allocate using volser and read via ddname
                             std::vector<std::string> dds;
                             dds.push_back("alloc dd(input) da('" + dsn + "') shr vol(" + volser + ")");
                             ZDIAG diag{};
                             rc = zut_loop_dynalloc(diag, dds);
                             ExpectWithContext(rc, diag.e_msg).ToBe(0);

                             ZDS read_zds{};
                             ZDSReadOpts read_opts{.zds = &read_zds, .ddname = "INPUT", .dsname = dsn};
                             std::string content;
                             rc = zds_read(read_opts, content);
                             ExpectWithContext(rc, read_zds.diag.e_msg).ToBe(0);
                             Expect(content.find("volser test") != std::string::npos).ToBe(true);

                             zut_free_dynalloc_dds(diag, dds);
                           });

                        it("should read from a dynamically allocated DD without a dsname",
                           [&]() -> void
                           {
                             std::string dsn = get_random_ds(3);
                             created_dsns.push_back(dsn);
                             ZDS zds = {0};
                             create_seq(&zds, dsn);
                             write_to_dsn(dsn, "dd only read test");

                             std::vector<std::string> dds;
                             dds.push_back("alloc dd(OUTDD) da('" + dsn + "') shr");
                             ZDIAG diag{};
                             int rc = zut_loop_dynalloc(diag, dds);
                             ExpectWithContext(rc, diag.e_msg).ToBe(0);

                             ZDS read_zds{};
                             ZDSReadOpts read_opts{.zds = &read_zds, .ddname = "OUTDD"};
                             std::string content;
                             rc = zds_read(read_opts, content);
                             ExpectWithContext(rc, read_zds.diag.e_msg).ToBe(0);
                             Expect(content.find("dd only read test") != std::string::npos).ToBe(true);

                             zut_free_dynalloc_dds(diag, dds);
                           });

                        it("should read from a SYSPRINT-style DD (FB lrecl=80)",
                           [&]() -> void
                           {
                             std::string expected = "SYSPRINT DD read test";

                             std::vector<std::string> dds;
                             dds.push_back("alloc dd(SYSPRINT) lrecl(80) recfm(f,b) blksize(80)");
                             ZDIAG diag{};
                             int rc = zut_loop_dynalloc(diag, dds);
                             ExpectWithContext(rc, diag.e_msg).ToBe(0);

                             ZDS write_zds{};
                             ZDSWriteOpts write_opts{.zds = &write_zds, .ddname = "SYSPRINT"};
                             rc = zds_write(write_opts, expected);
                             ExpectWithContext(rc, write_zds.diag.e_msg).ToBe(0);

                             ZDS read_zds{};
                             ZDSReadOpts read_opts{.zds = &read_zds, .ddname = "SYSPRINT"};
                             std::string output;
                             rc = zds_read(read_opts, output);
                             ExpectWithContext(rc, read_zds.diag.e_msg).ToBe(0);
                             Expect(output.find(expected) != std::string::npos).ToBe(true);

                             zut_free_dynalloc_dds(diag, dds);
                           });

                        describe("DD write operations",
                                 [&]() -> void
                                 {
                                   describe("basic operations",
                                            [&]() -> void
                                            {
                                              it("should write text to FB LRECL=80 DD (SYSPRINT pattern)",
                                                 [&]() -> void
                                                 {
                                                   DDTestContext ctx(created_dsns);
                                                   ctx.allocate_sysprint_dd();

                                                   std::string test_data = "Line 1: Basic text content\nLine 2: More test data\nLine 3: Final line";
                                                   ZDS write_zds{};
                                                   ZDSWriteOpts write_opts{.zds = &write_zds, .ddname = "SYSPRINT"};
                                                   int rc = zds_write(write_opts, test_data);
                                                   ExpectWithContext(rc, write_zds.diag.e_msg).ToBe(0);

                                                   bool has_line1 = ctx.verify_dd_content("SYSPRINT", "Line 1: Basic text content");
                                                   bool has_line2 = ctx.verify_dd_content("SYSPRINT", "Line 2: More test data");
                                                   bool has_line3 = ctx.verify_dd_content("SYSPRINT", "Line 3: Final line");
                                                   Expect(has_line1).ToBe(true);
                                                   Expect(has_line2).ToBe(true);
                                                   Expect(has_line3).ToBe(true);
                                                 });

                                              it("should write binary data to VB DD",
                                                 [&]() -> void
                                                 {
                                                   DDTestContext ctx(created_dsns);
                                                   ctx.allocate_vb_dd("BINDD", 255);

                                                   std::string binary_data = "Binary\x00\x01\x02test\xFF\xFE data";
                                                   ZDS write_zds{};
                                                   write_zds.encoding_opts.data_type = eDataTypeBinary;
                                                   ZDSWriteOpts write_opts{.zds = &write_zds, .ddname = "BINDD"};
                                                   int rc = zds_write(write_opts, binary_data);
                                                   ExpectWithContext(rc, write_zds.diag.e_msg).ToBe(0);

                                                   ZDS read_zds{};
                                                   read_zds.encoding_opts.data_type = eDataTypeBinary;
                                                   ZDSReadOpts read_opts{.zds = &read_zds, .ddname = "BINDD"};
                                                   std::string content;
                                                   rc = zds_read(read_opts, content);
                                                   ExpectWithContext(rc, read_zds.diag.e_msg).ToBe(0);
                                                   Expect(content.find("Binary") != std::string::npos).ToBe(true);
                                                 });

                                              it("should write empty content to DD",
                                                 [&]() -> void
                                                 {
                                                   DDTestContext ctx(created_dsns);
                                                   ctx.allocate_fb_dd("EMPTYDD", 80);

                                                   std::string empty_data = "";
                                                   ZDS write_zds{};
                                                   ZDSWriteOpts write_opts{.zds = &write_zds, .ddname = "EMPTYDD"};
                                                   int rc = zds_write(write_opts, empty_data);
                                                   ExpectWithContext(rc, write_zds.diag.e_msg).ToBe(0);

                                                   ZDS read_zds{};
                                                   ZDSReadOpts read_opts{.zds = &read_zds, .ddname = "EMPTYDD"};
                                                   std::string content;
                                                   rc = zds_read(read_opts, content);
                                                   ExpectWithContext(rc, read_zds.diag.e_msg).ToBe(0);
                                                   Expect(content.empty()).ToBe(true);
                                                 });

                                              it("should handle single-line vs multi-line content",
                                                 [&]() -> void
                                                 {
                                                   DDTestContext ctx(created_dsns);
                                                   ctx.allocate_fb_dd("TESTDD", 80);

                                                   // Test single line without newline
                                                   std::string single_line = "Single line without newline";
                                                   ZDS write_zds{};
                                                   ZDSWriteOpts write_opts{.zds = &write_zds, .ddname = "TESTDD"};
                                                   int rc = zds_write(write_opts, single_line);
                                                   ExpectWithContext(rc, write_zds.diag.e_msg).ToBe(0);

                                                   bool has_single_line = ctx.verify_dd_content("TESTDD", "Single line without newline");
                                                   Expect(has_single_line).ToBe(true);
                                                 });

                                              it("should verify content round-trip accuracy",
                                                 [&]() -> void
                                                 {
                                                   DDTestContext ctx(created_dsns);
                                                   ctx.allocate_fb_dd("ROUNDDD", 133);

                                                   std::string test_content = "Round-trip test line 1\nRound-trip test line 2\nSpecial chars: !@#$%^&*()";
                                                   ZDS write_zds{};
                                                   ZDSWriteOpts write_opts{.zds = &write_zds, .ddname = "ROUNDDD"};
                                                   int rc = zds_write(write_opts, test_content);
                                                   ExpectWithContext(rc, write_zds.diag.e_msg).ToBe(0);

                                                   ZDS read_zds{};
                                                   ZDSReadOpts read_opts{.zds = &read_zds, .ddname = "ROUNDDD"};
                                                   std::string content;
                                                   rc = zds_read(read_opts, content);
                                                   ExpectWithContext(rc, read_zds.diag.e_msg).ToBe(0);
                                                   Expect(content.find("Round-trip test line 1") != std::string::npos).ToBe(true);
                                                   Expect(content.find("Special chars: !@#$%^&*()") != std::string::npos).ToBe(true);
                                                 });
                                            });

                                   describe("record format support",
                                            [&]() -> void
                                            {
                                              it("should handle FB with various LRECL values",
                                                 [&]() -> void
                                                 {
                                                   DDTestContext ctx(created_dsns);

                                                   // Test LRECL=80 (standard)
                                                   ctx.allocate_fb_dd("FB80", 80);
                                                   std::string data80 = "Standard 80-character record for typical mainframe usage";
                                                   ZDS zds80{};
                                                   ZDSWriteOpts opts80{.zds = &zds80, .ddname = "FB80"};
                                                   int rc = zds_write(opts80, data80);
                                                   ExpectWithContext(rc, zds80.diag.e_msg).ToBe(0);

                                                   // Test LRECL=133 (wide reports)
                                                   ctx.allocate_fb_dd("FB133", 133);
                                                   std::string data133 = "Wide format record for reports that need more than 80 characters per line - this is commonly used for listings";
                                                   ZDS zds133{};
                                                   ZDSWriteOpts opts133{.zds = &zds133, .ddname = "FB133"};
                                                   rc = zds_write(opts133, data133);
                                                   ExpectWithContext(rc, zds133.diag.e_msg).ToBe(0);

                                                   // Test LRECL=255 (maximum for FB)
                                                   ctx.allocate_fb_dd("FB255", 255);
                                                   std::string data255 = "Maximum length fixed-block record that can contain up to 255 characters of data which is useful for very wide reports or data transfer operations that need the maximum capacity available in fixed-block format";
                                                   ZDS zds255{};
                                                   ZDSWriteOpts opts255{.zds = &zds255, .ddname = "FB255"};
                                                   rc = zds_write(opts255, data255);
                                                   ExpectWithContext(rc, zds255.diag.e_msg).ToBe(0);

                                                   bool has_fb80 = ctx.verify_dd_content("FB80", "Standard 80-character");
                                                   bool has_fb133 = ctx.verify_dd_content("FB133", "Wide format record");
                                                   bool has_fb255 = ctx.verify_dd_content("FB255", "Maximum length fixed-block");
                                                   Expect(has_fb80).ToBe(true);
                                                   Expect(has_fb133).ToBe(true);
                                                   Expect(has_fb255).ToBe(true);
                                                 });

                                              it("should handle VB with various LRECL values",
                                                 [&]() -> void
                                                 {
                                                   DDTestContext ctx(created_dsns);

                                                   // Test VB with LRECL=80
                                                   ctx.allocate_vb_dd("VB80", 80);
                                                   std::string vb_data = "Variable block record\nShorter line\nThis is a longer line that demonstrates variable length";
                                                   ZDS vb_zds{};
                                                   ZDSWriteOpts vb_opts{.zds = &vb_zds, .ddname = "VB80"};
                                                   int rc = zds_write(vb_opts, vb_data);
                                                   ExpectWithContext(rc, vb_zds.diag.e_msg).ToBe(0);

                                                   // Test VB with LRECL=1000 (large but reasonable)
                                                   ctx.allocate_vb_dd("VB1K", 1000);
                                                   std::string large_vb = "Large variable block record that can handle very long lines";
                                                   ZDS large_zds{};
                                                   ZDSWriteOpts large_opts{.zds = &large_zds, .ddname = "VB1K"};
                                                   rc = zds_write(large_opts, large_vb);
                                                   ExpectWithContext(rc, large_zds.diag.e_msg).ToBe(0);

                                                   bool has_vb80 = ctx.verify_dd_content("VB80", "Variable block record");
                                                   bool has_vb1k = ctx.verify_dd_content("VB1K", "Large variable block");
                                                   Expect(has_vb80).ToBe(true);
                                                   Expect(has_vb1k).ToBe(true);
                                                 });

                                              it("should return out-of-space failure if contents are too long",
                                                 [&]() -> void
                                                 {
                                                   DDTestContext ctx(created_dsns);
                                                   ctx.allocate_fb_dd("TRUNCDD", 40);

                                                   std::string long_line = "This line is much longer than 40 characters and should be truncated by the system";
                                                   ZDS trunc_zds{};
                                                   ZDSWriteOpts trunc_opts{.zds = &trunc_zds, .ddname = "TRUNCDD"};
                                                   int rc = zds_write(trunc_opts, long_line);
                                                   ExpectWithContext(rc, trunc_zds.diag.e_msg).ToBe(RTNCD_FAILURE);
                                                   Expect(std::string(trunc_zds.diag.e_msg)).ToContain("Failed to write all contents to 'DD:TRUNCDD' (possibly out of space)");

                                                   // Verify truncation occurred (should only contain first 40 chars)
                                                   ZDS read_zds{};
                                                   ZDSReadOpts read_opts{.zds = &read_zds, .ddname = "TRUNCDD"};
                                                   std::string content;
                                                   rc = zds_read(read_opts, content);
                                                   ExpectWithContext(rc, read_zds.diag.e_msg).ToBe(0);
                                                   Expect(content.find("This line is much longer than 40 char") != std::string::npos).ToBe(true);
                                                   Expect(content.find("should be truncated by the system") == std::string::npos).ToBe(true);
                                                 });
                                            });

                                   describe("encoding support",
                                            [&]() -> void
                                            {
                                              it("should handle text mode with default encoding",
                                                 [&]() -> void
                                                 {
                                                   DDTestContext ctx(created_dsns);
                                                   ctx.allocate_fb_dd("TEXTDD", 80);

                                                   std::string text_data = "Text mode content with UTF-8 characters";
                                                   ZDS text_zds{};
                                                   text_zds.encoding_opts.data_type = eDataTypeText;
                                                   ZDSWriteOpts text_opts{.zds = &text_zds, .ddname = "TEXTDD"};
                                                   int rc = zds_write(text_opts, text_data);
                                                   ExpectWithContext(rc, text_zds.diag.e_msg).ToBe(0);

                                                   bool has_text_content = ctx.verify_dd_content("TEXTDD", "Text mode content");
                                                   Expect(has_text_content).ToBe(true);
                                                 });

                                              it("should handle binary mode without conversion",
                                                 [&]() -> void
                                                 {
                                                   DDTestContext ctx(created_dsns);
                                                   ctx.allocate_vb_dd("BINDD", 100);

                                                   std::string binary_data = "Binary\x00\x01\x02\x03\xFF\xFE\xFD\xFC data";
                                                   ZDS binary_zds{};
                                                   binary_zds.encoding_opts.data_type = eDataTypeBinary;
                                                   ZDSWriteOpts binary_opts{.zds = &binary_zds, .ddname = "BINDD"};
                                                   int rc = zds_write(binary_opts, binary_data);
                                                   ExpectWithContext(rc, binary_zds.diag.e_msg).ToBe(0);

                                                   // Read back in binary mode
                                                   ZDS read_zds{};
                                                   read_zds.encoding_opts.data_type = eDataTypeBinary;
                                                   ZDSReadOpts read_opts{.zds = &read_zds, .ddname = "BINDD"};
                                                   std::string content;
                                                   rc = zds_read(read_opts, content);
                                                   ExpectWithContext(rc, read_zds.diag.e_msg).ToBe(0);
                                                   Expect(content.find("Binary") != std::string::npos).ToBe(true);
                                                 });

                                              it("should handle mixed content scenarios",
                                                 [&]() -> void
                                                 {
                                                   DDTestContext ctx(created_dsns);
                                                   ctx.allocate_fb_dd("MIXEDDD", 120);

                                                   std::string mixed_data = "Text content\nWith special chars: áéíóú\nAnd numbers: 12345";
                                                   ZDS mixed_zds{};
                                                   mixed_zds.encoding_opts.data_type = eDataTypeText;
                                                   ZDSWriteOpts mixed_opts{.zds = &mixed_zds, .ddname = "MIXEDDD"};
                                                   int rc = zds_write(mixed_opts, mixed_data);
                                                   ExpectWithContext(rc, mixed_zds.diag.e_msg).ToBe(0);

                                                   bool has_mixed_text = ctx.verify_dd_content("MIXEDDD", "Text content");
                                                   bool has_mixed_numbers = ctx.verify_dd_content("MIXEDDD", "And numbers: 12345");
                                                   Expect(has_mixed_text).ToBe(true);
                                                   Expect(has_mixed_numbers).ToBe(true);
                                                 });
                                            });

                                   describe("streaming operations",
                                            [&]() -> void
                                            {
                                              it("should handle zds_write_streamed with pipe input",
                                                 [&]() -> void
                                                 {
                                                   DDTestContext ctx(created_dsns);
                                                   ctx.allocate_fb_dd("STREAMDD", 80);

                                                   // Create a temporary file to simulate pipe input
                                                   std::string temp_file = "/tmp/zds_stream_test_" + std::to_string(getpid());
                                                   std::string content = "Streamed line 1\nStreamed line 2\nStreamed line 3\n";
                                                   std::string encoded_content = zbase64::encode(content);
                                                   std::ofstream temp_out(temp_file);
                                                   temp_out << encoded_content;
                                                   temp_out.close();

                                                   size_t content_len = 0;
                                                   ZDS stream_zds{};
                                                   ZDSWriteOpts stream_opts{.zds = &stream_zds, .ddname = "STREAMDD"};
                                                   int rc = zds_write_streamed(stream_opts, temp_file, &content_len);
                                                   ExpectWithContext(rc, stream_zds.diag.e_msg).ToBe(0);

                                                   // Verify content length was set
                                                   Expect(content_len > 0).ToBe(true);

                                                   // Verify content was written
                                                   bool has_stream_line1 = ctx.verify_dd_content("STREAMDD", "Streamed line 1");
                                                   bool has_stream_line3 = ctx.verify_dd_content("STREAMDD", "Streamed line 3");
                                                   Expect(has_stream_line1).ToBe(true);
                                                   Expect(has_stream_line3).ToBe(true);

                                                   // Cleanup
                                                   unlink(temp_file.c_str());
                                                 });

                                              it("should validate content length parameter",
                                                 [&]() -> void
                                                 {
                                                   DDTestContext ctx(created_dsns);
                                                   ctx.allocate_vb_dd("LENDD", 255);

                                                   std::string temp_file = "/tmp/zds_len_test_" + std::to_string(getpid());
                                                   std::string test_content = "Content length validation test data\nSecond line for length check\n";
                                                   std::string encoded_content = zbase64::encode(test_content);
                                                   std::ofstream temp_out(temp_file);
                                                   temp_out << encoded_content;
                                                   temp_out.close();

                                                   size_t content_len = 0;
                                                   ZDS len_zds{};
                                                   ZDSWriteOpts len_opts{.zds = &len_zds, .ddname = "LENDD"};
                                                   int rc = zds_write_streamed(len_opts, temp_file, &content_len);
                                                   ExpectWithContext(rc, len_zds.diag.e_msg).ToBe(0);

                                                   // Content length should match expected decoded size
                                                   Expect(content_len).ToBe(test_content.length());

                                                   unlink(temp_file.c_str());
                                                 });

                                              it("should handle large data efficiently",
                                                 [&]() -> void
                                                 {
                                                   DDTestContext ctx(created_dsns);
                                                   ctx.allocate_vb_dd("LARGEDD", 1000);

                                                   // Create larger test data
                                                   std::string temp_file = "/tmp/zds_large_test_" + std::to_string(getpid());
                                                   std::stringstream content_stream;
                                                   for (int i = 0; i < 1000; i++)
                                                   {
                                                     content_stream << "Large data test line " << i << " with substantial content to test streaming efficiency\n";
                                                   }
                                                   std::string content = content_stream.str();
                                                   std::string encoded_content = zbase64::encode(content);
                                                   std::ofstream temp_out(temp_file);
                                                   temp_out << encoded_content;
                                                   temp_out.close();

                                                   size_t content_len = 0;
                                                   ZDS large_zds{};
                                                   ZDSWriteOpts large_opts{.zds = &large_zds, .ddname = "LARGEDD"};
                                                   int rc = zds_write_streamed(large_opts, temp_file, &content_len);
                                                   ExpectWithContext(rc, large_zds.diag.e_msg).ToBe(0);

                                                   Expect(content_len > 5000).ToBe(true); // Should be substantial content (reduced from 100 lines)
                                                   bool has_large_line0 = ctx.verify_dd_content("LARGEDD", "Large data test line 0");
                                                   bool has_large_line999 = ctx.verify_dd_content("LARGEDD", "Large data test line 999");
                                                   ExpectWithContext(has_large_line0, "Line 0 is missing in written DD contents").ToBe(true);
                                                   ExpectWithContext(has_large_line999, "Line 999 is missing in written DD contents").ToBe(true);

                                                   unlink(temp_file.c_str());
                                                 });
                                            });

                                   describe("error handling",
                                            [&]() -> void
                                            {
                                              it("should reject empty DD names",
                                                 [&]() -> void
                                                 {
                                                   std::string test_data = "Test data for empty DD name";
                                                   ZDS error_zds{};
                                                   ZDSWriteOpts error_opts{.zds = &error_zds, .ddname = ""};
                                                   int rc = zds_write(error_opts, test_data);

                                                   Expect(rc).Not().ToBe(0);
                                                   Expect(std::string(error_zds.diag.e_msg).length() > 0).ToBe(true);
                                                 });

                                              it("should handle null content_len pointer in streamed operations",
                                                 [&]() -> void
                                                 {
                                                   std::string temp_file = "/tmp/zds_null_test_" + std::to_string(getpid());
                                                   std::string content = "Test data for null pointer\n";
                                                   std::string encoded_content = zbase64::encode(content);
                                                   std::ofstream temp_out(temp_file);
                                                   temp_out << encoded_content;
                                                   temp_out.close();

                                                   ZDS null_zds{};
                                                   ZDSWriteOpts null_opts{.zds = &null_zds, .ddname = "TESTDD"};
                                                   int rc = zds_write_streamed(null_opts, temp_file, nullptr);

                                                   Expect(rc).Not().ToBe(0);
                                                   Expect(std::string(null_zds.diag.e_msg).find("content_len") != std::string::npos).ToBe(true);

                                                   unlink(temp_file.c_str());
                                                 });
                                            });
                                 });
                      });

             describe("DCB abend",
                      [&]() -> void
                      {
                        it("should validate constants around DCB abend use",
                           [&]() -> void
                           {
                             Expect(DCB_ABEND_OPT_OK_TO_RECOVER).ToBe(0x08);
                             Expect(DCB_ABEND_OPT_OK_TO_IGNORE).ToBe(0x04);
                             Expect(DCB_ABEND_OPT_OK_TO_DELAY).ToBe(0x02);
                             Expect(DCB_ABEND_RC_TERMINATE).ToBe(0);
                             Expect(DCB_ABEND_RC_IGNORE).ToBe(4);
                             Expect(DCB_ABEND_RC_DELAY).ToBe(8);
                             Expect(DCB_ABEND_RC_IGNORE_QUIETLY).ToBe(20);
                           });

                        it("should validate the size of DCB_ABEND_PL struct",
                           [&]() -> void
                           {
                             Expect(sizeof(DCB_ABEND_PL)).ToBe(16);
                           });

                        it("should catch abend when writing past max space of a PDS",
                           [&]() -> void
                           {
                             ZDS zds = {0};
                             DS_ATTRIBUTES attr{};
                             attr.dsorg = "PO";
                             attr.recfm = "FB";
                             attr.lrecl = 80;
                             attr.blksize = 6160;
                             attr.alcunit = "TRACKS";
                             attr.primary = 1;
                             attr.secondary = 0;
                             attr.dirblk = 1;

                             std::string ds = get_random_ds(3);
                             created_dsns.push_back(ds);

                             std::string response;
                             int rc = zds_create_dsn(&zds, ds, attr, response);
                             ExpectWithContext(rc, response).ToBe(0);

                             zds.encoding_opts.data_type = eDataTypeText;
                             strcpy(zds.encoding_opts.codepage, "IBM-1047");
                             strcpy(zds.encoding_opts.source_codepage, "IBM-1047");

                             std::string large_data;
                             large_data.reserve(81 * 1000);
                             for (int i = 0; i < 1000; i++)
                             {
                               large_data += std::string(80, 'A') + "\n";
                             }

                             ZDSWriteOpts write_opts{.zds = &zds, .dsname = ds + "(M1)"};

                             // This should fail with an abend. DCB abend exit runs as part of the member write process - no mocking available so we can't test the DCB exit itself,
                             // this just verifies that the abend is percolated and handled by recovery
                             Expect([&]()
                                    { rc = zds_write(write_opts, large_data); })
                                 .ToAbend();
                           });
                      });
           });
}

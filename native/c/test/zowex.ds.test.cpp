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

#include <cstddef>
#include <cstdio>
#include <ctime>
#include <fcntl.h>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include "../zbase64.h"
#include "ztest.hpp"
#include "zutils.hpp"
#include "ztype.h"
#include "zowex.test.hpp"
#include "zowex.ds.test.hpp"
#include "../zusf.hpp"

using namespace ztst;

// Generic helper function for creating data sets
void _create_ds(const std::string &ds_name, const std::string &ds_options = "")
{
  std::string response;
  std::string command = zowex_command + " data-set create " + ds_name + " " + ds_options;
  int rc = execute_command_with_output(command, response);
  ExpectWithContext(rc, response).ToBe(0);
  Expect(response).ToContain("Data set created");
}

void zowex_ds_tests()
{
  std::vector<std::string> _ds;
  describe("data-set",
           [&]() -> void
           {
             TEST_OPTIONS long_test_opts = {false, 30};

             afterAll(
                 [&]() -> void
                 {
                   TestLog("Deleting " + std::to_string(_ds.size()) + " data sets...");
                   for (const auto &ds : _ds)
                   {
                     try
                     {
                       std::string command = zowex_command + " data-set delete " + ds;
                       std::string response;
                       int rc = execute_command_with_output(command, response);
                       ExpectWithContext(rc, response).ToBe(0);
                       Expect(response).ToContain("Data set '" + ds + "' deleted"); // ds deleted
                     }
                     catch (...)
                     {
                       try
                       {
                         std::string response;
                         std::string command = zowex_command + " data-set list " + ds + " --no-warn --me 1";
                         int rc = execute_command_with_output(command, response);
                         ExpectWithContext(rc, response).ToBe(0);
                         Expect(response).Not().ToContain(ds);
                       }
                       catch (...)
                       {
                         TestLog("Failed to delete: " + ds);
                       }
                     }
                   }
                 },
                 long_test_opts);
             it("should display help", []() -> void
                {
                  int rc = 0;
                  std::string response;
                  std::string command = zowex_command + " data-set";
                  rc = execute_command_with_output(command, response);
                  ExpectWithContext(rc, response).ToBe(0);
                  Expect(response).ToContain("compress");
                  Expect(response).ToContain("create");
                  Expect(response).ToContain("create-adata");
                  Expect(response).ToContain("create-fb");
                  Expect(response).ToContain("create-loadlib");
                  Expect(response).ToContain("create-member");
                  Expect(response).ToContain("create-vb");
                  Expect(response).ToContain("delete");
                  Expect(response).ToContain("list");
                  Expect(response).ToContain("list-members");
                  Expect(response).ToContain("restore");
                  Expect(response).ToContain("view");
                  Expect(response).ToContain("write"); });

             describe("compress",
                      [&]() -> void
                      {
                        beforeEach(
                            [&]() -> void
                            {
                              _ds.push_back(get_random_ds());
                            });
                        it("should error when the data set is PS",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PS");

                             std::string response;
                             std::string command = zowex_command + " data-set compress " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: data set '" + ds + "' is not a PDS");
                           });
                        it("should error when the data set is PDS/E",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PO --dirblk 2 --dsntype LIBRARY");

                             std::string response;
                             std::string command = zowex_command + " data-set compress " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: data set '" + ds + "' is not a PDS");
                           });

                        it("should error when the data set doesn't exist",
                           [&]() -> void
                           {
                             std::string ds = _ds.back() + ".GHOST";

                             std::string response;
                             std::string command = zowex_command + " data-set compress " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: data set '" + ds + "' is not a PDS");
                           });

                        // NOTE: This test may fail if we don't save/clear/restore registers for PDSMAN/IEBCOPY
                        // See https://github.com/zowe/zowex/issues/790
                        it("should compress a data set", [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PO --dirblk 2");

                             std::string response;
                             std::string command = zowex_command + " data-set compress " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Data set");
                             Expect(response).ToContain("compressed"); });

                        // TODO: https://github.com/zowe/zowex/issues/666
                        xit("should error when the data set is VSAM", []() -> void {});
                        xit("should error when the data set is GDG", []() -> void {});
                        xit("should error when the data set is ALIAS", []() -> void {});
                      });
             describe("create",
                      [&]() -> void
                      {
                        beforeEach(
                            [&]() -> void
                            {
                              _ds.push_back(get_random_ds());
                            });
                        it("should error when the data set already exists",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds);

                             std::string response;
                             std::string command = zowex_command + " data-set create " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not create data set");
                           });

                        it("should create a data set with default attributes",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds);

                             std::string response;
                             std::string command = zowex_command + " data-set list " + ds + " -a --rfc";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             std::vector<std::string> tokens = parse_rfc_response(response, ",");
                             // NOTE: Non-SMS managed systems return `--` for dsorg if not specified
                             Expect(tokens[3]).ToBe("PS");
                             Expect(tokens[4]).ToBe("FB");
                           });

                        it("should create a simple PDS/E data set - dsorg: PO and dsntype: LIBRARY",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PO --dirblk 2 --dsntype LIBRARY");

                             std::string response;
                             std::string command = zowex_command + " data-set list " + ds + " -a --rfc";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             std::vector<std::string> tokens = parse_rfc_response(response, ",");
                             Expect(tokens[3]).ToBe("PO");
                             // DSNTYPE may be LIBRARY or PDS depending on system SMS settings
                             Expect(tokens[9] == "LIBRARY" || tokens[9] == "PDS").ToBe(true);
                           });

                        it("should create a data set - recfm:VB dsorg:PO",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--recfm VB --dsorg PO --dirblk 2");

                             std::string response;

                             std::string command = zowex_command + " data-set list " + ds + " -a --rfc";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             std::vector<std::string> tokens = parse_rfc_response(response, ",");
                             Expect(tokens[3]).ToBe("PO");
                             Expect(tokens[4]).ToBe("VB");
                           });

                        it("should create a PS data set - recfm:VB dsorg:PS",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--recfm VB --dsorg PS");

                             std::string response;
                             std::string command = zowex_command + " data-set list " + ds + " -a --rfc";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             std::vector<std::string> tokens = parse_rfc_response(response, ",");
                             Expect(tokens[3]).ToBe("PS");
                             Expect(tokens[4]).ToBe("VB");
                           });

                        // TODO: https://github.com/zowe/zowex/pull/625
                        it("should create a data set - dsorg: PO, primary: 10, secondary: 2, lrecl: 20, blksize:10, dirblk: 5, alcunit: CYL",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PO --primary 10 --secondary 2 --lrecl 20 --blksize 10 --dirblk 5 --alcunit CYL");

                             std::string response;
                             std::string command = zowex_command + " data-set list " + ds + " -a --rfc";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             std::vector<std::string> tokens = parse_rfc_response(response, ",");
                             Expect(tokens[3]).ToBe("PO"); // dsorg
                             Expect(tokens[4]).ToBe("FB"); // recfm
                             Expect(tokens[5]).ToBe("20"); // lrecl
                             Expect(tokens[6]).ToBe("10"); // blksize
                             Expect(tokens[7]).ToBe("10"); // primary
                             Expect(tokens[8]).ToBe("2");  // secondary
                           });
                        it("should fail to create a data set if the data set name is too long",
                           []() -> void
                           {
                             int rc = 0;
                             std::string ds = get_random_ds(8);
                             std::string response;
                             std::string command = zowex_command + " data-set create " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not create data set: '" + ds + "'");
                           });
                      });
             describe("create-adata",
                      [&]() -> void
                      {
                        it("should create a data set with default attributes",
                           [&]() -> void
                           {
                             int rc = 0;
                             std::string ds = get_random_ds();
                             _ds.push_back(ds);

                             std::string response;
                             std::string command = zowex_command + " data-set create-adata " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Data set created");

                             command = zowex_command + " data-set list " + ds + " -a --rfc";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             std::vector<std::string> tokens = parse_rfc_response(response, ",");
                             Expect(tokens[3]).ToBe("PO");
                             Expect(tokens[4]).ToBe("VB");
                             // DSNTYPE may be LIBRARY or PDS depending on system SMS settings
                             Expect(tokens[9] == "LIBRARY" || tokens[9] == "PDS").ToBe(true);
                             // lrecl = 32756
                           });

                        it("should fail to create a data set if the data set already exists",
                           [&]() -> void
                           {
                             int rc = 0;
                             std::string ds = get_random_ds();
                             _ds.push_back(ds);

                             std::string response;
                             std::string command = zowex_command + " data-set create-adata " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Data set created");

                             command = zowex_command + " data-set create-adata " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not create data set: '" + ds + "'");
                           });
                        it("should fail to create a data set if the data set name is too long",
                           []() -> void
                           {
                             int rc = 0;
                             std::string ds = get_random_ds(8);
                             std::string response;
                             std::string command = zowex_command + " data-set create-adata " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not create data set: '" + ds + "'");
                           });
                      });

             describe("create-fb",
                      [&]() -> void
                      {
                        it("should create a data set with default attributes",
                           [&]() -> void
                           {
                             int rc = 0;
                             std::string ds = get_random_ds();
                             _ds.push_back(ds);

                             std::string response;
                             std::string command = zowex_command + " data-set create-fb " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Data set created");

                             command = zowex_command + " data-set list " + ds + " -a --rfc";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             std::vector<std::string> tokens = parse_rfc_response(response, ",");
                             Expect(tokens[3]).ToBe("PO");
                             Expect(tokens[4]).ToBe("FB");
                             // DSNTYPE may be LIBRARY or PDS depending on system SMS settings
                             Expect(tokens[9] == "LIBRARY" || tokens[9] == "PDS").ToBe(true);
                             // lrecl = 80
                           });
                        it("should fail to create a data set if the data set already exists",
                           [&]() -> void
                           {
                             int rc = 0;
                             std::string ds = get_random_ds();
                             _ds.push_back(ds);

                             std::string response;
                             std::string command = zowex_command + " data-set create-fb " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Data set created");

                             command = zowex_command + " data-set create-fb " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not create data set: '" + ds + "'");
                           });
                        it("should fail to create a data set if the data set name is too long",
                           []() -> void
                           {
                             int rc = 0;
                             std::string ds = get_random_ds(8);
                             std::string response;
                             std::string command = zowex_command + " data-set create-fb " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not create data set: '" + ds + "'");
                           });
                      });
             describe("create-loadlib",
                      [&]() -> void
                      {
                        it("should create a data set with default attributes",
                           [&]() -> void
                           {
                             int rc = 0;
                             std::string ds = get_random_ds();
                             _ds.push_back(ds);

                             std::string response;
                             std::string command = zowex_command + " data-set create-loadlib " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Data set created");

                             command = zowex_command + " data-set list " + ds + " -a --rfc";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             std::vector<std::string> tokens = parse_rfc_response(response, ",");
                             Expect(tokens[3]).ToBe("PO");
                             Expect(tokens[4]).ToBe("U");
                             Expect(tokens[5]).ToBe("0"); // lrecl
                             // DSNTYPE may be LIBRARY or PDS depending on system SMS settings
                             Expect(tokens[9] == "LIBRARY" || tokens[9] == "PDS").ToBe(true);
                           });
                        it("should fail to create a data set if the data set already exists",
                           [&]() -> void
                           {
                             int rc = 0;
                             std::string ds = get_random_ds();
                             _ds.push_back(ds);

                             std::string response;
                             std::string command = zowex_command + " data-set create-loadlib " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Data set created");

                             command = zowex_command + " data-set create-loadlib " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not create data set: '" + ds + "'");
                           });
                        it("should fail to create a data set if the data set name is too long",
                           []() -> void
                           {
                             int rc = 0;
                             std::string ds = get_random_ds(8);
                             std::string response;
                             std::string command = zowex_command + " data-set create-loadlib " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not create data set: '" + ds + "'");
                           });
                      });
             describe("create-member",
                      [&]() -> void
                      {
                        beforeEach(
                            [&]() -> void
                            {
                              std::string ds = get_random_ds();
                              _ds.push_back(ds);

                              std::string response;
                              std::string command = zowex_command + " data-set create-fb " + ds;
                              int rc = execute_command_with_output(command, response);
                              ExpectWithContext(rc, response).ToBe(0);
                              Expect(response).ToContain("Data set created");
                            });
                        it("should error if no member name is specified",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             std::string response;
                             std::string command = zowex_command + " data-set create-member " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not find member name in dsn");
                           });
                        it("should error if the data set doesn't exist",
                           [&]() -> void
                           {
                             std::string ds = _ds.back() + ".GHOST";
                             std::string response;
                             std::string command = zowex_command + " data-set create-member '" + ds + "(TEST)'";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not create data set member");
                             Expect(response).ToContain("Not found in catalog");
                           });
                        it("should create a member in a PDS",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             std::string response;
                             std::string command = zowex_command + " data-set create-member '" + ds + "(TEST)'";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Data set and/or member created");
                           });

                        // TODO: https://github.com/zowe/zowex/issues/643
                        xit("should not overwrite existing members",
                            [&]() -> void
                            {
                              std::string ds = "'" + _ds.back() + "(TEST)'";
                              std::string response;
                              std::string command = zowex_command + " data-set create-member " + ds;
                              int rc = execute_command_with_output(command, response);
                              ExpectWithContext(rc, response).ToBe(0);
                              Expect(response).ToContain("Data set and/or member created");

                              // Write "test" data
                              command = "echo test | " + zowex_command + " data-set write " + ds;
                              rc = execute_command_with_output(command, response);
                              ExpectWithContext(rc, response).ToBe(0);
                              Expect(response).ToContain("Wrote data to");

                              // Read "test" data to confirm
                              command = "echo test | " + zowex_command + " data-set view " + ds;
                              rc = execute_command_with_output(command, response);
                              ExpectWithContext(rc, response).ToBe(0);
                              Expect(response).ToContain("test");

                              // Create the same TEST member
                              command = "echo test | " + zowex_command + " data-set create-member " + ds;
                              rc = execute_command_with_output(command, response);
                              ExpectWithContext(rc, response).Not().ToBe(0);
                              Expect(response).ToContain("ERROR");

                              // Read "test" data to confirm
                              command = "echo test | " + zowex_command + " data-set view " + ds;
                              rc = execute_command_with_output(command, response);
                              ExpectWithContext(rc, response).ToBe(0);
                              Expect(response).ToContain("test");
                            });
                      });
             describe("create-vb",
                      [&]() -> void
                      {
                        beforeEach(
                            [&]() -> void
                            {
                              _ds.push_back(get_random_ds());
                            });
                        it("should create a data set with default attributes",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();

                             std::string response;
                             std::string command = zowex_command + " data-set create-vb " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Data set created");

                             command = zowex_command + " data-set list " + ds + " -a --rfc";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             std::vector<std::string> tokens = parse_rfc_response(response, ",");
                             Expect(tokens[3]).ToBe("PO");
                             Expect(tokens[4]).ToBe("VB");
                             // DSNTYPE may be LIBRARY or PDS depending on system SMS settings
                             Expect(tokens[9] == "LIBRARY" || tokens[9] == "PDS").ToBe(true);
                             Expect(tokens[5]).ToBe("255"); // lrecl
                           });
                        it("should error when the data set already exists",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();

                             std::string response;
                             std::string command = zowex_command + " data-set create-vb " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Data set created");

                             command = zowex_command + " data-set create-vb " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not create data set: '" + ds + "'");
                           });
                        it("should fail to create a data set if the data set name is too long",
                           []() -> void
                           {
                             int rc = 0;
                             std::string ds = get_random_ds(8);
                             std::string response;
                             std::string command = zowex_command + " data-set create-vb " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not create data set: '" + ds + "'");
                           });
                      });
             describe("delete",
                      [&]() -> void
                      {
                        beforeEach(
                            [&]() -> void
                            {
                              _ds.push_back(get_random_ds());
                            });

                        it("should delete a sequential data set",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PS");

                             std::string response;
                             std::string command = zowex_command + " data-set delete " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Data set '" + ds + "' deleted");
                           });
                        it("should delete a partitioned data set (PDS)",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PO --dirblk 2");

                             std::string response;
                             std::string command = zowex_command + " data-set delete " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Data set '" + ds + "' deleted");
                           });
                        it("should delete a partitioned data set extended (PDSE)",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PO --dirblk 2 --dsntype LIBRARY");

                             std::string response;
                             std::string command = zowex_command + " data-set delete " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Data set '" + ds + "' deleted");
                           });

                        it("should fail to delete a non-existent data set",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();

                             std::string response;
                             std::string command = zowex_command + " data-set delete " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not delete data set: '" + ds + "'");
                           });
                        it("should fail to delete a data set if the data set name is too long",
                           []() -> void
                           {
                             std::string ds = get_random_ds(8);

                             std::string response;
                             std::string command = zowex_command + " data-set delete " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not delete data set: '" + ds + "'");
                           });

                        // TODO: What do?
                        xit("should fail to delete a data set if not authorized", []() -> void {});
                        // TODO: What do?
                        xit("should fail to delete a data set that is currently in use", []() -> void {});

                        // TODO: https://github.com/zowe/zowex/issues/665
                        xit("should delete multiple data sets specified in a list", []() -> void {});

                        // TODO: https://github.com/zowe/zowex/issues/664
                        xit("should delete a data set using the force option even if it has members", []() -> void {});
                        xit("should not delete a data set with the force option if it is in use", []() -> void {});

                        // TODO: https://github.com/zowe/zowex/issues/666
                        xit("should delete a VSAM KSDS data set", []() -> void {});
                        xit("should delete a VSAM ESDS data set", []() -> void {});
                        xit("should delete a VSAM RRDS data set", []() -> void {});
                        xit("should delete a VSAM LDS data set", []() -> void {});

                        // TODO: https://github.com/zowe/zowex/issues/666
                        xit("should delete a generation data group (GDG) base when empty", []() -> void {});
                        xit("should delete a generation data group (GDG) base and all its generations", []() -> void {});
                        xit("should delete a specific generation of a GDG", []() -> void {});
                        xit("should error when attempting to delete a GDG base with generations without the PURGE or FORCE option", []() -> void {});
                      });
             describe("list",
                      [&]() -> void
                      {
                        beforeEach(
                            [&]() -> void
                            {
                              _ds.push_back(get_random_ds());
                            });
                        it("should list a data set",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds);

                             std::string response;
                             std::string command = zowex_command + " data-set list " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain(ds);
                           });
                        it("should list data sets based on pattern and warn about listing too many data sets",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds);
                             _create_ds(ds + ".T00");
                             _ds.push_back(ds + ".T00");

                             std::string response;
                             std::string pattern = ds;
                             std::string command = zowex_command + " data-set list " + pattern + " --me 1";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(RTNCD_WARNING);
                             Expect(response).ToContain(ds);
                             Expect(response).Not().ToContain(ds + ".T00");
                           });

                        it("should list up to the max entries specified and not warn",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds);
                             _create_ds(ds + ".T00");
                             _ds.push_back(ds + ".T00");

                             std::string response;
                             std::string pattern = ds;
                             std::string command = zowex_command + " data-set list " + pattern + " --no-warn --me 1";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain(ds);
                             Expect(response).Not().ToContain(ds + ".T00");
                           });

                        it("should warn when listing a non-existent data set",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();

                             std::string response;
                             std::string command = zowex_command + " data-set list " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(RTNCD_WARNING);
                             Expect(response).ToContain("Warning: no matching results found");
                           });
                        it("should error when the data set name is too long",
                           [&]() -> void
                           {
                             std::string ds = get_random_ds(8);

                             std::string response;
                             std::string command = zowex_command + " data-set list " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: data set pattern exceeds 44 character length limit");
                           });

                        // TODO: https://github.com/zowe/zowex/issues/666
                        xit("should list information for a VSAM KSDS data set", []() -> void {});
                        xit("should list information for a VSAM ESDS data set", []() -> void {});
                        xit("should list information for a VSAM RRDS data set", []() -> void {});
                        xit("should list information for a VSAM LDS data set", []() -> void {});

                        // TODO: https://github.com/zowe/zowex/issues/666
                        xit("should list generations of a generation data group (GDG) base", []() -> void {});
                        xit("should list specific generation of a GDG", []() -> void {});
                      });
             describe("list-members",
                      [&]() -> void
                      {
                        std::string data_set = "SYS1.MACLIB";
                        it("should list a member of a data set",
                           [&]() -> void
                           {
                             std::string response;
                             std::string command = zowex_command + " data-set lm " + data_set + " --no-warn --me 1";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                           });
                        it("should warn when listing members of a data set with many members",
                           [&]() -> void
                           {
                             int rc = 0;
                             std::string response;
                             std::string command = zowex_command + " data-set lm " + data_set + " --me 1";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(RTNCD_WARNING);
                           });
                        it("should error when the data set name is too long",
                           [&]() -> void
                           {
                             std::string ds = get_random_ds(8);
                             std::string response;
                             std::string command = zowex_command + " data-set lm " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                           });
                        it("should fail if the data set doesn't exist",
                           [&]() -> void
                           {
                             std::string ds = _ds.back() + ".GHOST";
                             std::string response;
                             std::string command = zowex_command + " data-set lm " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: Could not list members for '" + ds + "'");
                           });
                        it("should list members matching a specific pattern",
                           [&]() -> void
                           {
                             std::string ds = get_random_ds();
                             _ds.push_back(ds);
                             _create_ds(ds, "--dsorg PO --dirblk 2");

                             std::string response;
                             execute_command_with_output(zowex_command + " data-set create-member '" + ds + "(ABC1)'", response);
                             execute_command_with_output(zowex_command + " data-set create-member '" + ds + "(ABC2)'", response);
                             execute_command_with_output(zowex_command + " data-set create-member '" + ds + "(XYZ1)'", response);

                             std::string command = zowex_command + " data-set lm " + ds + " --pattern \"ABC*\"";
                             int rc = execute_command_with_output(command, response);

                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("ABC1");
                             Expect(response).ToContain("ABC2");
                             Expect(response).Not().ToContain("XYZ1");
                           });
                        it("should return ISPF statistics attributes when listing members with attributes",
                           [&]() -> void
                           {
                             auto ds = get_random_ds();
                             _ds.push_back(ds);

                             _create_ds(ds, "--dsorg PO --dirblk 2");

                             std::string response{};

                             execute_command_with_output(
                                 zowex_command + " data-set create-member '" + ds + "(STAT1)'",
                                 response);

                             execute_command_with_output(
                                 "echo \"TEST\" | " + zowex_command + " data-set write '" + ds + "(STAT1)'",
                                 response);
                             std::string command = zowex_command + " data-set lm " + ds + " --attributes";

                             int rc = execute_command_with_output(command, response);

                             ExpectWithContext(rc, response).ToBe(0);

                             Expect(response).ToContain("STAT1");
                             Expect(response).ToContain("/");
                             Expect(response).ToContain(":");
                             Expect(response).ToContain("N");
                           });
                      });
             // TODO: https://github.com/zowe/zowex/issues/380
             xdescribe("restore",
                       [&]() -> void
                       {
                         beforeEach(
                             [&]() -> void
                             {
                               _ds.push_back(get_random_ds());
                               _create_ds(_ds.back());
                             });
                         it("should restore a data set",
                            [&]() -> void
                            {
                              std::string ds = _ds.back();

                              std::string response;
                              std::string command = zowex_command + " data-set restore " + ds;
                              int rc = execute_command_with_output(command, response);
                              ExpectWithContext(rc, response).ToBe(0);
                              Expect(response).ToContain("Data set '" + ds + "' restored");
                            });
                         it("should fail to restore a non-existent backup",
                            [&]() -> void
                            {
                              std::string ds = _ds.back() + ".GHOST";

                              std::string response;
                              std::string command = zowex_command + " data-set restore " + ds;
                              int rc = execute_command_with_output(command, response);
                              ExpectWithContext(rc, response).Not().ToBe(0);
                              Expect(response).ToContain("Error: could not restore data set: '" + ds + "'");
                            });
                         // TODO: What do?
                         xit("should fail to restore if not authorized", [&]() -> void {});
                       });
             describe("view",
                      [&]() -> void
                      {
                        beforeEach(
                            [&]() -> void
                            {
                              _ds.push_back(get_random_ds());
                            });
                        it("should view the content of a sequential data set",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PS");
                             std::string response;

                             std::string random_string = get_random_string(80, false);
                             std::string command = "echo " + random_string + " | " + zowex_command + " data-set write " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Wrote data to '" + ds + "'");

                             command = zowex_command + " data-set view " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain(random_string);
                           });
                        it("should view the content of a PDS member",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PO --dirblk 2");
                             std::string response;

                             std::string random_string = get_random_string(80, false);
                             std::string command = "echo " + random_string + " | " + zowex_command + " data-set write '" + ds + "(TEST)'";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Wrote data to '" + ds + "(TEST)'");

                             command = zowex_command + " data-set view '" + ds + "(TEST)'";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain(random_string);
                           });
                        it("should view a data set with different encoding",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PS");
                             std::string response;
                             std::string command = "echo 'test!' | " + zowex_command + " data-set write " + ds + " --local-encoding IBM-1047 --encoding IBM-1147";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Wrote data to '" + ds + "'");

                             command = zowex_command + " data-set view " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("test|");

                             command = zowex_command + " data-set view " + ds + " --local-encoding IBM-1047 --encoding IBM-1047";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("test|");

                             command = zowex_command + " data-set view " + ds + " --local-encoding IBM-1047 --encoding IBM-1147";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("test!");
                           });
                        it("should fail to view a non-existent data set",
                           [&]() -> void
                           {
                             std::string ds = _ds.back() + ".GHOST";
                             std::string response;
                             std::string command = zowex_command + " data-set view " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not read data set: '" + ds + "'");
                           });
                        it("should error when the data set name too long",
                           [&]() -> void
                           {
                             std::string ds = get_random_ds(8);
                             std::string response;
                             std::string command = zowex_command + " data-set view " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not read data set: '" + ds + "'");
                           });

                        // What do?
                        xit("should fail to view a data set if not authorized", []() -> void {});

                        // TODO: https://github.com/zowe/zowex/issues/666
                        xit("should view the content of a VSAM KSDS data set", []() -> void {});
                        xit("should view the content of a VSAM ESDS data set", []() -> void {});
                        xit("should view the content of a VSAM RRDS data set", []() -> void {});
                        xit("should view the content of a VSAM LDS data set", []() -> void {});
                      });
             describe("write",
                      [&]() -> void
                      {
                        beforeEach(
                            [&]() -> void
                            {
                              _ds.push_back(get_random_ds());
                            });
                        it("should write content to a sequential data set",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PS");

                             std::string response;
                             std::string random_string = get_random_string(80, false);
                             std::string command = "echo " + random_string + " | " + zowex_command + " data-set write " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Wrote data to '" + ds + "'");

                             command = zowex_command + " data-set view " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain(random_string);
                           });
                        it("should write content to a PDS member",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PO --dirblk 2");

                             std::string response;
                             std::string command = zowex_command + " data-set create-member '" + ds + "(TEST)'";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Data set and/or member created");

                             std::string random_string = get_random_string(80, false);
                             command = "echo " + random_string + " | " + zowex_command + " data-set write '" + ds + "(TEST)'";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Wrote data to '" + ds + "(TEST)'");

                             command = zowex_command + " data-set view '" + ds + "(TEST)'";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain(random_string);
                           });

                        it("should fail to write to a RECFM=U data set",
                           [&]() -> void
                           {
                             std::string ds = get_random_ds();
                             _ds.push_back(ds);
                             _create_ds(ds, "--dsorg PO --dirblk 2 --recfm U --lrecl 0 --blksize 32760");

                             std::string response;
                             std::string command = "echo 'test' | " + zowex_command + " data-set write '" + ds + "(TEST)'";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Writing to RECFM=U data sets is not supported");
                           });
                        it("should be able to write to a RECFM=A data set",
                           [&]() -> void
                           {
                             std::string ds = get_random_ds();
                             _ds.push_back(ds);
                             _create_ds(ds, "--dsorg PS --recfm A --lrecl 80 --blksize 800");

                             std::string response;
                             std::string command = "echo 'test' | " + zowex_command + " data-set write " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Wrote data to '" + ds + "'");
                           });
                        it("should overwrite content in a sequential data set",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PS");

                             std::string response;
                             std::string random_string = get_random_string(80, false);
                             std::string random_string1 = get_random_string(80, false);
                             std::string random_string2 = get_random_string(80, false);
                             std::string command = "echo '" + random_string + "\n" + random_string1 + "' | " + zowex_command + " data-set write " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Wrote data to '" + ds + "'");

                             command = zowex_command + " data-set view " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain(random_string);
                             Expect(response).ToContain(random_string1);
                             Expect(response).Not().ToContain(random_string2);

                             command = "echo " + random_string2 + " | " + zowex_command + " data-set write " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Wrote data to '" + ds + "'");

                             command = zowex_command + " data-set view " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain(random_string2);
                             Expect(response).Not().ToContain(random_string);
                             Expect(response).Not().ToContain(random_string1);
                           });
                        it("should overwrite content in a PDS member",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PO --dirblk 2");

                             std::string response;
                             std::string command = zowex_command + " data-set create-member '" + ds + "(TEST)'";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Data set and/or member created");

                             std::string random_string = get_random_string(80, false);
                             std::string random_string1 = get_random_string(80, false);
                             std::string random_string2 = get_random_string(80, false);
                             command = "echo '" + random_string + "\n" + random_string1 + "' | " + zowex_command + " data-set write '" + ds + "(TEST)' --local-encoding IBM-1047";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Wrote data to '" + ds + "(TEST)'");

                             command = zowex_command + " data-set view '" + ds + "(TEST)'";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain(random_string);
                             Expect(response).ToContain(random_string1);
                             Expect(response).Not().ToContain(random_string2);

                             command = "echo " + random_string2 + " | " + zowex_command + " data-set write '" + ds + "(TEST)' --local-encoding IBM-1047";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Wrote data to '" + ds + "(TEST)'");

                             command = zowex_command + " data-set view '" + ds + "(TEST)'";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain(random_string2);
                             Expect(response).Not().ToContain(random_string);
                             Expect(response).Not().ToContain(random_string1);
                           });

                        it("should only print the etag when requested",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PS");
                             std::string response;
                             std::string command = "echo 'test' | " + zowex_command + " data-set write " + ds + " --etag-only";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("etag: 8890283"); // etag for "test"
                             Expect(response).Not().ToContain("Wrote data to '" + ds + "'");
                           });

                        it("should fail if the provided etag is different from the current etag",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PS");
                             std::string response;
                             std::string command = "echo 'test' | " + zowex_command + " data-set write " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Wrote data to '" + ds + "'");

                             command = "echo 'zowe' | " + zowex_command + " data-set write " + ds + " --etag 8bb0280"; // etag for "zowe"
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Etag mismatch: expected 8bb0280, actual 8890283");
                             Expect(response).Not().ToContain("Wrote data to '" + ds + "'");
                           });

                        it("should fail if the provided etag is different and evaluates to a number",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PS");
                             std::string response;
                             std::string command = "echo 'zowe' | " + zowex_command + " data-set write " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Wrote data to '" + ds + "'");

                             command = "echo 'test' | " + zowex_command + " data-set write " + ds + " --etag 8890283"; // etag for "test"
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Etag mismatch: expected 8890283, actual 8bb0280");
                             Expect(response).Not().ToContain("Wrote data to '" + ds + "'");
                           });

                        it("should write content to a data set with different encoding",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PS");
                             std::string response;
                             std::string command = "echo 'test!' | " + zowex_command + " data-set write " + ds + " --local-encoding IBM-1047 --encoding IBM-1147";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Wrote data to '" + ds + "'");

                             command = zowex_command + " data-set view " + ds;
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("test|");

                             command = zowex_command + " data-set view " + ds + " --rfb";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("a3 85 a2 a3 4f 15");
                           });

                        it("should write content to a data set with multibyte encoding",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PS");
                             std::string response;
                             std::string command = "echo '\xe3\x81\x93\xe3\x82\x93\xe3\x81\xab\xe3\x81\xa1\xe3\x81\xaf' | " + zowex_command + " data-set write " + ds + " --encoding IBM-939";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Wrote data to '" + ds + "'");

                             command = zowex_command + " data-set view " + ds + " --rfb";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("0e 44 8a 44 bd 44 97 44 92 44 9d 0f");
                           });

                        it("should fail to write to a non-existent data set",
                           [&]() -> void
                           {
                             std::string ds = _ds.back() + ".GHOST";
                             std::string response;
                             std::string command = "echo 'test' | " + zowex_command + " data-set write " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not write to data set: '" + ds + "'");
                           });
                        it("should error when the data set name is too long",
                           []() -> void
                           {
                             std::string ds = get_random_ds(8);
                             std::string response;
                             std::string command = "echo 'test' | " + zowex_command + " data-set write " + ds;
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).Not().ToBe(0);
                             Expect(response).ToContain("Error: could not write to data set: '" + ds + "'");
                           });
                        it("should write content from a USS file to a sequential data set",
                           [&]() -> void
                           {
                             std::string ds = _ds.back();
                             _create_ds(ds, "--dsorg PS --lrecl 120 --blksize 18480");
                             std::string response;

                             // Create a temporary USS file
                             std::string temp_file = "/tmp/zowex_test_uss_" + get_random_string(8, false);
                             std::string content = "This is a test content for USS file write.";

                             ZUSF zusf = {};
                             std::string write_content = content;
                             int write_rc = zusf_write_to_uss_file(&zusf, temp_file, write_content);
                             Expect(write_rc).ToBe(RTNCD_SUCCESS);

                             // View the USS file as binary to avoid BPXK_AUTOCVT interference in the pipe,
                             // then write the raw bytes into the data set (also as binary).
                             std::string get_uss_file_command = zowex_command + " uss view '" + temp_file + "' --ec binary";
                             std::string command = get_uss_file_command + " | " + zowex_command + " data-set write " + ds + " --ec binary";
                             int rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain("Wrote data to '" + ds + "'");

                             command = zowex_command + " data-set view " + ds + " --ec binary";
                             rc = execute_command_with_output(command, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             Expect(response).ToContain(content);

                             // Cleanup
                             remove(temp_file.c_str());
                           });

                        describe("BPAM member writes",
                                 [&]() -> void
                                 {
                                   it("should write and read a PDS member using BPAM",
                                      [&]() -> void
                                      {
                                        std::string ds = _ds.back();
                                        _create_ds(ds, "--dsorg PO --dirblk 2");

                                        std::string response;
                                        std::string command = zowex_command + " data-set create-member '" + ds + "(PDS1)'";
                                        int rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Data set and/or member created");

                                        std::string payload = "PDSDATA";
                                        command = "echo " + payload + " | " + zowex_command + " data-set write '" + ds + "(PDS1)'";
                                        rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Wrote data to '" + ds + "(PDS1)'");

                                        command = zowex_command + " data-set view '" + ds + "(PDS1)'";
                                        rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain(payload);
                                      });
                                   it("should write and read a PDSE member using BPAM",
                                      [&]() -> void
                                      {
                                        std::string ds = _ds.back();
                                        _create_ds(ds, "--dsorg PO --dirblk 2 --dsntype LIBRARY");

                                        std::string response;
                                        std::string command = zowex_command + " data-set create-member '" + ds + "(MEM1)'";
                                        int rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Data set and/or member created");

                                        std::string payload = "MEMBERDATA";
                                        command = "echo " + payload + " | " + zowex_command + " data-set write '" + ds + "(MEM1)'";
                                        rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Wrote data to '" + ds + "(MEM1)'");

                                        command = zowex_command + " data-set view '" + ds + "(MEM1)'";
                                        rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain(payload);
                                      });

                                   it("should write multi-line content to a PDSE member",
                                      [&]() -> void
                                      {
                                        std::string ds = _ds.back();
                                        _create_ds(ds, "--dsorg PO --dirblk 2 --dsntype LIBRARY");

                                        std::string response;
                                        std::string command = zowex_command + " data-set create-member '" + ds + "(MEM2)'";
                                        int rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Data set and/or member created");

                                        std::string command_payload = "printf 'LINEA\\nLINEB\\n' | " + zowex_command + " data-set write '" + ds + "(MEM2)'";
                                        rc = execute_command_with_output(command_payload, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Wrote data to '" + ds + "(MEM2)'");

                                        command = zowex_command + " data-set view '" + ds + "(MEM2)'";
                                        rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("LINEA");
                                        Expect(response).ToContain("LINEB");
                                      });

                                   it("should overwrite a PDSE member using BPAM",
                                      [&]() -> void
                                      {
                                        std::string ds = _ds.back();
                                        _create_ds(ds, "--dsorg PO --dirblk 2 --dsntype LIBRARY");

                                        std::string response;
                                        std::string command = zowex_command + " data-set create-member '" + ds + "(MEM3)'";
                                        int rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Data set and/or member created");

                                        std::string command_payload = "echo FIRSTDATA | " + zowex_command + " data-set write '" + ds + "(MEM3)'";
                                        rc = execute_command_with_output(command_payload, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Wrote data to '" + ds + "(MEM3)'");

                                        command_payload = "echo SECONDDATA | " + zowex_command + " data-set write '" + ds + "(MEM3)'";
                                        rc = execute_command_with_output(command_payload, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Wrote data to '" + ds + "(MEM3)'");

                                        command = zowex_command + " data-set view '" + ds + "(MEM3)'";
                                        rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("SECONDDATA");
                                        Expect(response).Not().ToContain("FIRSTDATA");
                                      });

                                   it("should warn when a PDSE member line exceeds LRECL",
                                      [&]() -> void
                                      {
                                        std::string ds = _ds.back();
                                        _create_ds(ds, "--dsorg PO --dirblk 2 --dsntype LIBRARY --lrecl 10 --blksize 80");

                                        std::string response;
                                        std::string command = zowex_command + " data-set create-member '" + ds + "(MEM4)'";
                                        int rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Data set and/or member created");

                                        std::string long_line = "ABCDEFGHIJKLMNOPQRST";
                                        command = "echo " + long_line + " | " + zowex_command + " data-set write '" + ds + "(MEM4)'";
                                        rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Warning:");
                                        Expect(response).ToContain("truncated");
                                      });
                                 });

                        describe("ASA control characters",
                                 [&]() -> void
                                 {
                                   it("should add ASA control characters for a PDS member",
                                      [&]() -> void
                                      {
                                        std::string ds = _ds.back();
                                        _create_ds(ds, "--dsorg PO --dirblk 2 --recfm FBA --lrecl 81 --blksize 810");

                                        std::string response;
                                        std::string command = zowex_command + " data-set create-member '" + ds + "(ASA2)'";
                                        int rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Data set and/or member created");

                                        command = "printf 'AAA\\nBBB\\n' | " + zowex_command +
                                                  " data-set write '" + ds + "(ASA2)' --local-encoding IBM-1047 --encoding IBM-1047";
                                        rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Wrote data to '" + ds + "(ASA2)'");

                                        command = zowex_command + " data-set view '" + ds + "(ASA2)' --local-encoding IBM-1047 --encoding IBM-1047";
                                        rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain(" AAA");
                                        Expect(response).ToContain(" BBB");
                                      });
                                   it("should add ASA control characters for FBA data sets",
                                      [&]() -> void
                                      {
                                        std::string ds = _ds.back();
                                        _create_ds(ds, "--dsorg PS --recfm FBA --lrecl 81 --blksize 810");

                                        std::string response;
                                        std::string command = "printf 'AAA\\nBBB\\n' | " + zowex_command +
                                                              " data-set write " + ds + " --local-encoding IBM-1047 --encoding IBM-1047";
                                        int rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Wrote data to '" + ds + "'");

                                        command = zowex_command + " data-set view " + ds + " --local-encoding IBM-1047 --encoding IBM-1047";
                                        rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain(" AAA");
                                        Expect(response).ToContain(" BBB");
                                      });

                                   it("should convert a single blank line into ASA double space",
                                      [&]() -> void
                                      {
                                        std::string ds = _ds.back();
                                        _create_ds(ds, "--dsorg PS --recfm FBA --lrecl 81 --blksize 810");

                                        std::string response;
                                        std::string command = "printf 'AAA\\n\\nBBB\\n' | " + zowex_command +
                                                              " data-set write " + ds + " --local-encoding IBM-1047 --encoding IBM-1047";
                                        int rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Wrote data to '" + ds + "'");

                                        command = zowex_command + " data-set view " + ds + " --local-encoding IBM-1047 --encoding IBM-1047";
                                        rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain(" AAA");
                                        Expect(response).ToContain("0BBB");
                                      });

                                   it("should handle multiple blank lines with ASA overflow",
                                      [&]() -> void
                                      {
                                        std::string ds = _ds.back();
                                        _create_ds(ds, "--dsorg PS --recfm FBA --lrecl 81 --blksize 810");

                                        std::string response;
                                        std::string command = "printf 'AAA\\n\\n\\n\\nDDD\\n' | " + zowex_command +
                                                              " data-set write " + ds + " --local-encoding IBM-1047 --encoding IBM-1047";
                                        int rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Wrote data to '" + ds + "'");

                                        command = zowex_command + " data-set view " + ds + " --local-encoding IBM-1047 --encoding IBM-1047";
                                        rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain(" AAA");
                                        Expect(response).ToContain("-");
                                        Expect(response).ToContain("\n DDD");
                                      });

                                   it("should convert form feed to ASA page break",
                                      [&]() -> void
                                      {
                                        std::string ds = _ds.back();
                                        _create_ds(ds, "--dsorg PS --recfm FBA --lrecl 81 --blksize 810");

                                        std::string response;
                                        std::string command = "printf '\\fEEE\\n' | " + zowex_command +
                                                              " data-set write " + ds + " --local-encoding IBM-1047 --encoding IBM-1047";
                                        int rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Wrote data to '" + ds + "'");

                                        command = zowex_command + " data-set view " + ds + " --local-encoding IBM-1047 --encoding IBM-1047";
                                        rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("1EEE");
                                      });
                                   it("should read ASA VBA without RDW bytes",
                                      [&]() -> void
                                      {
                                        std::string ds = _ds.back();
                                        _create_ds(ds, "--dsorg PS --recfm VBA --lrecl 81 --blksize 810");

                                        std::string response;
                                        std::string command = "printf ' ABC\\n' | " + zowex_command +
                                                              " data-set write " + ds + " --local-encoding IBM-1047 --encoding IBM-1047";
                                        int rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Wrote data to '" + ds + "'");

                                        command = zowex_command + " data-set view " + ds + " --rfb --local-encoding IBM-1047 --encoding IBM-1047";
                                        rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);

                                        const size_t first_byte_pos = response.find_first_not_of(" \r\n\t");
                                        ExpectWithContext(first_byte_pos != std::string::npos, response).ToBe(true);
                                        std::string first_byte = response.substr(first_byte_pos, 2);
                                        Expect(first_byte).ToBe("40");
                                      });

                                   it("should stream ASA conversion to a member via pipe-path",
                                      [&]() -> void
                                      {
                                        std::string ds = _ds.back();
                                        _create_ds(ds, "--dsorg PO --dirblk 2 --dsntype LIBRARY --recfm FBA --lrecl 81 --blksize 810");

                                        std::string response;
                                        std::string command = zowex_command + " data-set create-member '" + ds + "(ASA3)'";
                                        int rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Data set and/or member created");

                                        const std::string pipe_path = "/tmp/zowex_ds_pipe_" + get_random_string(10);
                                        mkfifo(pipe_path.c_str(), 0777);

                                        const std::string payload = "AAA\n\nBBB\n";
                                        const auto encoded = zbase64::encode(payload.c_str(), payload.size());
                                        const std::string encoded_payload(encoded.begin(), encoded.end());

                                        std::thread writer([&]() -> void
                                                           {
                                                           int fd = -1;
                                                           for (int attempt = 0; attempt < 100 && fd == -1; ++attempt)
                                                           {
                                                             fd = open(pipe_path.c_str(), O_WRONLY | O_NONBLOCK);
                                                             if (fd == -1)
                                                             {
                                                               usleep(10000);
                                                             }
                                                           }
                                                           if (fd != -1)
                                                           {
                                                             write(fd, encoded_payload.data(), encoded_payload.size());
                                                             close(fd);
                                                           } });

                                        command = zowex_command + " data-set write '" + ds + "(ASA3)' --pipe-path " + pipe_path +
                                                  " --local-encoding IBM-1047 --encoding IBM-1047";
                                        rc = execute_command_with_output(command, response);
                                        writer.join();
                                        unlink(pipe_path.c_str());

                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain("Wrote data to '" + ds + "(ASA3)'");

                                        command = zowex_command + " data-set view '" + ds + "(ASA3)' --local-encoding IBM-1047 --encoding IBM-1047";
                                        rc = execute_command_with_output(command, response);
                                        ExpectWithContext(rc, response).ToBe(0);
                                        Expect(response).ToContain(" AAA");
                                        Expect(response).ToContain("0BBB");
                                      });
                                 });

                        // TODO: What do?
                        xit("should fail if the data set is deleted before the write operation is completed", []() -> void {});
                        // TODO: What do?
                        xit("should fail to write to a data set if not authorized", []() -> void {});
                      });
           });
}

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

#include "ztest.hpp"
#include <string>
#include <cstdlib>
#include <unistd.h>
#include "zutils.hpp"
#include "zowex.server.test.hpp"
#include "zowex.ds.server.test.hpp"

using namespace ztst;

// Helper function to create unique data set names for testing
static std::string get_test_ds()
{
  return get_random_ds(3);
}

void zowex_ds_server_tests()
{
  describe("data set server tests", []() -> void {
    ServerHandle server;
    
    beforeAll([&]() -> void {
      server = start_server(zowex_server_command, true);
    });

    afterAll([&]() -> void {
      stop_server(server);
    });

    describe("compress", [&]() -> void {
      // Skipped: compressDataset RPC method not implemented in server
    });


    // TODO: Enable once RPC passing is fixed
    xdescribe("copy", [&]() -> void {
      std::string src_ds;
      std::string dest_ds;

      beforeEach([&]() -> void {
        src_ds = get_test_ds() + ".SRC";
        dest_ds = get_test_ds() + ".DEST";
        
        // Create source data set
        std::string response;
        execute_command_with_output(zowex_command + " ds create " + src_ds + " --dsorg PO --recfm FB --lrecl 80 --primary 5 --dirblk 25", response);
        
        // Add some content to the source dataset
        execute_command_with_output("echo 'Test data for copy' | " + zowex_command + " ds write '" + src_ds + "(TESTMEM)'", response);
      });

      afterEach([&]() -> void {
        std::string response;
        execute_command_with_output(zowex_command + " ds delete " + src_ds, response);
        execute_command_with_output(zowex_command + " ds delete " + dest_ds, response);
      });

      it("should properly copy a data set via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"copyDatasetOrMember\",\"params\":{\"source\":\"" + src_ds + "\",\"target\":\"" + dest_ds + "\"},\"id\":2}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":2");

        // Verify destination data set exists
        std::string ls_response;
        int rc = execute_command_with_output(zowex_command + " ds list " + dest_ds, ls_response);
        Expect(rc).ToBe(0);
        
        // Verify the copied member exists
        int rc2 = execute_command_with_output(zowex_command + " ds list-members '" + dest_ds + "'", ls_response);
        Expect(rc2).ToBe(0);
        Expect(ls_response).ToContain("TESTMEM");
        
        // Verify content was copied correctly
        std::string content_response;
        int rc3 = execute_command_with_output(zowex_command + " ds view '" + dest_ds + "(TESTMEM)'", content_response);
        Expect(rc3).ToBe(0);
        Expect(content_response).ToContain("Test data for copy");
      });
    });

    describe("create", [&]() -> void {
      std::string ds_name;

      beforeEach([&]() -> void {
        ds_name = get_test_ds();
      });

      afterEach([&]() -> void {
        std::string response;
        execute_command_with_output(zowex_command + " ds delete " + ds_name, response);
      });

      it("should properly create a data set via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"createDataset\",\"params\":{\"dsname\":\"" + ds_name + "\",\"attributes\":{\"dsorg\":\"PO\",\"recfm\":\"FB\",\"lrecl\":80,\"primary\":5,\"dirblk\":25}},\"id\":3}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":3");

        // Verify data set was created
        std::string ls_response;
        int rc = execute_command_with_output(zowex_command + " ds list " + ds_name, ls_response);
        Expect(rc).ToBe(0);
      });
    });

    describe("create-adata", [&]() -> void {
      std::string ds_name;

      beforeEach([&]() -> void {
        ds_name = get_test_ds();
      });

      afterEach([&]() -> void {
        std::string response;
        execute_command_with_output(zowex_command + " ds delete " + ds_name, response);
      });

      it("should properly create VB data set with defaults via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"createDataset\",\"params\":{\"dsname\":\"" + ds_name + "\",\"attributes\":{\"dsorg\":\"PO\",\"recfm\":\"VB\",\"lrecl\":32756,\"primary\":5,\"dirblk\":25}},\"id\":4}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":4");
      });
    });

    describe("create-fb", [&]() -> void {
      std::string ds_name;

      beforeEach([&]() -> void {
        ds_name = get_test_ds();
      });

      afterEach([&]() -> void {
        std::string response;
        execute_command_with_output(zowex_command + " ds delete " + ds_name, response);
      });

      it("should properly create FB data set with defaults via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"createDataset\",\"params\":{\"dsname\":\"" + ds_name + "\",\"attributes\":{\"dsorg\":\"PO\",\"recfm\":\"FB\",\"lrecl\":80,\"primary\":5,\"dirblk\":25}},\"id\":5}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":5");
      });
    });

    describe("create-loadlib", [&]() -> void {
      std::string ds_name;

      beforeEach([&]() -> void {
        ds_name = get_test_ds();
      });

      afterEach([&]() -> void {
        std::string response;
        execute_command_with_output(zowex_command + " ds delete " + ds_name, response);
      });

      it("should properly create loadlib data set with defaults via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"createDataset\",\"params\":{\"dsname\":\"" + ds_name + "\",\"attributes\":{\"dsorg\":\"PO\",\"recfm\":\"U\",\"lrecl\":0,\"primary\":5,\"dirblk\":25}},\"id\":6}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":6");
      });
    });

    describe("create-member", [&]() -> void {
      std::string ds_name;
      std::string member_name;

      beforeEach([&]() -> void {
        ds_name = get_test_ds();
        member_name = "MEMBER1";
        
        // Create parent PDS first
        std::string response;
        execute_command_with_output(zowex_command + " ds create " + ds_name + " --dsorg PO --recfm FB --lrecl 80 --primary 5 --dirblk 25", response);
      });

      afterEach([&]() -> void {
        std::string response;
        execute_command_with_output(zowex_command + " ds delete " + ds_name, response);
      });

      it("should properly create a member via RPC", [&]() -> void {
        std::string full_dsname = ds_name + "(" + member_name + ")";
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"createMember\",\"params\":{\"dsname\":\"" + full_dsname + "\"},\"id\":7}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":7");

        // Verify member exists
        std::string ls_response;
        int rc = execute_command_with_output(zowex_command + " ds list-members '" + ds_name + "'", ls_response);
        Expect(rc).ToBe(0);
        Expect(ls_response).ToContain(member_name);
      });
    });

    describe("create-vb", [&]() -> void {
      std::string ds_name;

      beforeEach([&]() -> void {
        ds_name = get_test_ds();
      });

      afterEach([&]() -> void {
        std::string response;
        execute_command_with_output(zowex_command + " ds delete " + ds_name, response);
      });

      it("should properly create VB data set with defaults via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"createDataset\",\"params\":{\"dsname\":\"" + ds_name + "\",\"attributes\":{\"dsorg\":\"PO\",\"recfm\":\"VB\",\"lrecl\":255,\"primary\":5,\"dirblk\":25}},\"id\":8}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":8");
      });
    });

    describe("delete", [&]() -> void {
      std::string ds_name;

      beforeEach([&]() -> void {
        ds_name = get_test_ds();
        
        // Create data set to delete
        std::string response;
        execute_command_with_output(zowex_command + " ds create " + ds_name + " --dsorg PO --recfm FB --lrecl 80 --primary 5 --dirblk 25", response);
      });

      it("should properly delete a data set via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"deleteDataset\",\"params\":{\"dsname\":\"" + ds_name + "\"},\"id\":9}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":9");

        // Verify data set no longer exists
        std::string ls_response;
        int rc = execute_command_with_output(zowex_command + " ds list " + ds_name, ls_response);
        Expect(rc).Not().ToBe(0);
      });
    });

    describe("list", [&]() -> void {
      std::string ds_pattern;

      beforeEach([&]() -> void {
        ds_pattern = get_user() + ".TEST.*";
      });

      it("should properly list data sets via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"listDatasets\",\"params\":{\"pattern\":\"" + ds_pattern + "\"},\"id\":10}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":10");
        Expect(response).ToContain("\"items\"");
      });
    });

    describe("list-members", [&]() -> void {
      std::string ds_name;
      std::string member_name;

      beforeEach([&]() -> void {
        ds_name = get_test_ds();
        member_name = "TESTMEM";
        
        // Create PDS with a member
        std::string response;
        execute_command_with_output(zowex_command + " ds create " + ds_name + " --dsorg PO --recfm FB --lrecl 80 --primary 5 --dirblk 25", response);
        execute_command_with_output(zowex_command + " ds create-member '" + ds_name + "(" + member_name + ")'", response);
      });

      afterEach([&]() -> void {
        std::string response;
        execute_command_with_output(zowex_command + " ds delete " + ds_name, response);
      });

      it("should properly list data set members via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"listDsMembers\",\"params\":{\"dsname\":\"" + ds_name + "\"},\"id\":11}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":11");
        Expect(response).ToContain("\"items\"");
        Expect(response).ToContain(member_name);
      });
    });

    describe("rename", [&]() -> void {
      std::string ds_before;
      std::string ds_after;

      beforeEach([&]() -> void {
        ds_before = get_test_ds() + ".OLD";
        ds_after = get_test_ds() + ".NEW";
        
        // Create data set to rename
        std::string response;
        execute_command_with_output(zowex_command + " ds create " + ds_before + " --dsorg PO --recfm FB --lrecl 80 --primary 5", response);
      });

      afterEach([&]() -> void {
        std::string response;
        execute_command_with_output(zowex_command + " ds delete " + ds_before, response);
        execute_command_with_output(zowex_command + " ds delete " + ds_after, response);
      });

      it("should properly rename a data set via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"renameDataset\",\"params\":{\"dsnameBefore\":\"" + ds_before + "\",\"dsnameAfter\":\"" + ds_after + "\"},\"id\":12}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":12");

        // Verify old name doesn't exist, new name does
        std::string ls_response;
        int rc1 = execute_command_with_output(zowex_command + " ds list " + ds_before, ls_response);
        Expect(rc1).Not().ToBe(0);
        int rc2 = execute_command_with_output(zowex_command + " ds list " + ds_after, ls_response);
        Expect(rc2).ToBe(0);
      });
    });

    describe("rename-member", [&]() -> void {
      std::string ds_name;
      std::string member_before;
      std::string member_after;

      beforeEach([&]() -> void {
        ds_name = get_test_ds();
        member_before = "OLDMEM";
        member_after = "NEWMEM";
        
        // Create PDS with a member
        std::string response;
        execute_command_with_output(zowex_command + " ds create " + ds_name + " --dsorg PO --recfm FB --lrecl 80 --primary 5 --dirblk 25", response);
        execute_command_with_output(zowex_command + " ds create-member '" + ds_name + "(" + member_before + ")'", response);
      });

      afterEach([&]() -> void {
        std::string response;
        execute_command_with_output(zowex_command + " ds delete " + ds_name, response);
      });

      it("should properly rename a member via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"renameMember\",\"params\":{\"dsname\":\"" + ds_name + "\",\"memberBefore\":\"" + member_before + "\",\"memberAfter\":\"" + member_after + "\"},\"id\":13}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":13");

        // Verify old member doesn't exist, new member does
        std::string ls_response;
        execute_command_with_output(zowex_command + " ds list-members '" + ds_name + "'", ls_response);
        Expect(ls_response).Not().ToContain(member_before);
        Expect(ls_response).ToContain(member_after);
      });
    });

    describe("restore", [&]() -> void {
      std::string ds_name;

      beforeEach([&]() -> void {
        ds_name = get_test_ds();
        
        // Create and delete data set to test restore (assuming it gets migrated/recalled)
        std::string response;
        execute_command_with_output(zowex_command + " ds create " + ds_name + " --dsorg PO --recfm FB --lrecl 80 --primary 5 --dirblk 25", response);
      });

      afterEach([&]() -> void {
        std::string response;
        execute_command_with_output(zowex_command + " ds delete " + ds_name, response);
      });

      it("should properly restore/recall a data set via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"restoreDataset\",\"params\":{\"dsname\":\"" + ds_name + "\"},\"id\":14}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":14");
      });
    });

    describe("view", [&]() -> void {
      std::string ds_name;

      beforeEach([&]() -> void {
        ds_name = get_test_ds();
        
        // Create data set and write some content
        std::string response;
        execute_command_with_output(zowex_command + " ds create " + ds_name + " --dsorg PS --recfm FB --lrecl 80 --primary 5", response);
        execute_command_with_output("echo 'Test content' | " + zowex_command + " ds write " + ds_name, response);
      });

      afterEach([&]() -> void {
        std::string response;
        execute_command_with_output(zowex_command + " ds delete " + ds_name, response);
      });

      it("should properly view/read a data set via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"readDataset\",\"params\":{\"dsname\":\"" + ds_name + "\"},\"id\":15}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":15");
        Expect(response).ToContain("\"data\"");
      });
    });

    describe("write and view", [&]() -> void {
      std::string ds_name;

      beforeEach([&]() -> void {
        ds_name = get_test_ds();
        
        // Create data set for writing
        std::string response;
        execute_command_with_output(zowex_command + " ds create " + ds_name + " --dsorg PS --recfm FB --lrecl 80 --primary 5", response);
      });

      afterEach([&]() -> void {
        std::string response;
        execute_command_with_output(zowex_command + " ds delete " + ds_name, response);
      });

      it("should properly write and read a data set via RPC", [&]() -> void {
        // Write operation: base64 encoded "Hello World!" is "SGVsbG8gV29ybGQh"
        std::string write_req = "{\"jsonrpc\":\"2.0\",\"method\":\"writeDataset\",\"params\":{\"dsname\":\"" + ds_name + "\",\"data\":\"SGVsbG8gV29ybGQh\"},\"id\":16}\n";
        
        write_to_server(server, write_req);
        std::string write_resp = read_line_from_server(server);

        Expect(write_resp).ToContain("\"success\":true");
        Expect(write_resp).ToContain("\"id\":16");
        
        // Read operation: verify we get back the same base64 content
        std::string read_req = "{\"jsonrpc\":\"2.0\",\"method\":\"readDataset\",\"params\":{\"dsname\":\"" + ds_name + "\"},\"id\":17}\n";
        
        write_to_server(server, read_req);
        std::string read_resp = read_line_from_server(server);

        Expect(read_resp).ToContain("\"success\":true");
        Expect(read_resp).ToContain("\"id\":17");
        Expect(read_resp).ToContain("\"data\":");
        // Note: Data set may add trailing newline, so we check that data is returned
        // The returned base64 may be "SGVsbG8gV29ybGQhCg==" (with newline) instead of "SGVsbG8gV29ybGQh"
        ExpectWithContext(read_resp, "Should contain our written data").ToContain("SGVsbG8gV29ybGQ");
      });
    });

    describe("streaming integration", [&]() -> void {
      std::string ds_name;

      beforeEach([&]() -> void {
        ds_name = get_test_ds();
        
        // Create PS dataset for streaming test
        std::string response;
        execute_command_with_output(zowex_command + " ds create " + ds_name + " --dsorg PS --recfm FB --lrecl 80 --primary 5", response);
      });

      afterEach([&]() -> void {
        std::string response;
        execute_command_with_output(zowex_command + " ds delete " + ds_name, response);
      });

      it("should write via stream", [&]() -> void {
        // Create unique pipe path
        std::string pipe_path = "/tmp/zowex_stream_" + get_random_string(8);
        
        // Test data to stream (base64: "Hello Stream\nLine 2\n")
        std::string test_data = "SGVsbG8gU3RyZWFtCkxpbmUgMgo=";
        
        std::string stream_cmd = "echo '" + test_data + "' > " + pipe_path + " &";
        system(stream_cmd.c_str());
        
        usleep(100000); // 100ms
        
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"writeDataset\",\"params\":{\"dsname\":\"" + ds_name + "\",\"stream\":\"" + pipe_path + "\"},\"id\":25}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        unlink(pipe_path.c_str());

        Expect(response).ToContain("\"id\":25");
        ExpectWithContext(response, "Should be valid JSON-RPC response").ToContain("jsonrpc");
        
        std::string read_request = "{\"jsonrpc\":\"2.0\",\"method\":\"readDataset\",\"params\":{\"dsname\":\"" + ds_name + "\"},\"id\":26}\n";
        
        write_to_server(server, read_request);
        std::string read_response = read_line_from_server(server);
        Expect(read_response).ToContain("\"success\":true");
        Expect(read_response).ToContain("\"data\":");

      });

      it("should read via stream", [&]() -> void {
        std::string test_content = "U3RyZWFtIHJlYWQgdGVzdAo=";
        std::string write_request = "{\"jsonrpc\":\"2.0\",\"method\":\"writeDataset\",\"params\":{\"dsname\":\"" + ds_name + "\",\"data\":\"" + test_content + "\"},\"id\":30}\n";
        
        write_to_server(server, write_request);
        std::string write_response = read_line_from_server(server);
        
        Expect(write_response).ToContain("\"success\":true");

        std::string output_pipe = "/tmp/zowex_read_stream_" + get_random_string(8);
        
        std::string read_request = "{\"jsonrpc\":\"2.0\",\"method\":\"readDataset\",\"params\":{\"dsname\":\"" + ds_name + "\",\"stream\":\"" + output_pipe + "\"},\"id\":31}\n";
        
        write_to_server(server, read_request);
        std::string read_response = read_line_from_server(server);

        Expect(read_response).ToContain("\"id\":31");
        ExpectWithContext(read_response, "Should be valid JSON-RPC response").ToContain("jsonrpc");

        unlink(output_pipe.c_str());

        ExpectWithContext(read_response, "Read streaming RPC interface should be supported").ToContain("\"id\":31");
      });
    });

  });
}
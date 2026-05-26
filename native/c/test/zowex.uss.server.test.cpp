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
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include "../zbase64.h"
#include "zutils.hpp"
#include "zowex.server.test.hpp"
#include "zowex.uss.server.test.hpp"

using namespace ztst;

// Create unique test directory to avoid conflicts between test runs
static const std::string ussTestDir = "/tmp/zowex_uss_srv_" + get_random_string(10);

void zowex_uss_server_tests()
{
  describe("uss server tests", []() -> void {
    ServerHandle server;
    
    beforeAll([&]() -> void {
      server = start_server(zowex_server_command, true);

      // Create test directory with full permissions for all test operations
      std::string response;
      execute_command_with_output(zowex_command + " uss create-dir " + ussTestDir + " --mode 777", response);
    });

    afterAll([&]() -> void {
      stop_server(server);

      std::string response;
      execute_command_with_output(zowex_command + " uss delete " + ussTestDir + " --recursive", response);
    });

    describe("chmod", [&]() -> void {
      std::string uss_path;

      beforeEach([&]() -> void {
        uss_path = get_random_uss(ussTestDir);
        std::string response;
        execute_command_with_output(zowex_command + " uss create-file " + uss_path, response);
      });

      it("should properly chmod a file via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"chmodFile\",\"params\":{\"fspath\":\"" + uss_path + "\",\"mode\":\"777\"},\"id\":1}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":1");

        // Verify permissions were actually set by checking ls output
        std::string ls_response;
        execute_command_with_output("ls -l " + uss_path, ls_response);
        Expect(ls_response).ToContain("-rwxrwxrwx");
      });
    });

    describe("chown", [&]() -> void {
      std::string uss_path;

      beforeEach([&]() -> void {
        uss_path = get_random_uss(ussTestDir);
        std::string response;
        execute_command_with_output(zowex_command + " uss create-file " + uss_path, response);
      });

      it("should properly chown a file via RPC", [&]() -> void {
        // Get current user ID for chown operation
        std::string resp;
        execute_command_with_output("id -u", resp);
        resp.erase(resp.find_last_not_of(" \t\r\n") + 1); // trim whitespace
        
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"chownFile\",\"params\":{\"fspath\":\"" + uss_path + "\",\"owner\":\"" + resp + "\"},\"id\":2}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":2");

        // Verify ownership was actually set by checking ls -l output
        std::string ls_response;
        execute_command_with_output("ls -l " + uss_path, ls_response);
        
        // ls -l shows the username, so we need to get it via id -un to compare
        std::string username_resp;
        execute_command_with_output("id -un", username_resp);
        username_resp.erase(username_resp.find_last_not_of(" \t\r\n") + 1); // trim whitespace
        
        ExpectWithContext(ls_response, "File should be owned by the user").ToContain(username_resp);
      });
    });

    describe("chtag", [&]() -> void {
      std::string uss_path;

      beforeEach([&]() -> void {
        uss_path = get_random_uss(ussTestDir);
        std::string response;
        execute_command_with_output(zowex_command + " uss create-file " + uss_path, response);
      });

      it("should properly chtag a file via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"chtagFile\",\"params\":{\"fspath\":\"" + uss_path + "\",\"tag\":\"IBM-1047\"},\"id\":3}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":3");
        
        // Verify tag was applied using ls -alT (shows encoding tags)
        std::string ls_response;
        execute_command_with_output("ls -alT " + uss_path, ls_response);
        Expect(ls_response).ToContain("IBM-1047");
      });
    });

    describe("copy", [&]() -> void {
      std::string src_path;
      std::string dest_path;

      beforeEach([&]() -> void {
        src_path = get_random_uss(ussTestDir) + "_src";
        dest_path = get_random_uss(ussTestDir) + "_dest";
        std::string response;
        execute_command_with_output(zowex_command + " uss create-file " + src_path, response);

        // Populate the source file with some sample content via RPC writeFile
        std::string write_req = "{\"jsonrpc\":\"2.0\",\"method\":\"writeFile\",\"params\":{\"fspath\":\"" + src_path + "\",\"data\":\"SGVsbG8gY29weSE=\"},\"id\":40}\n";
        write_to_server(server, write_req);
        read_line_from_server(server); // consume response
      });

      it("should properly copy a file via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"copyUss\",\"params\":{\"srcFsPath\":\"" + src_path + "\",\"dstFsPath\":\"" + dest_path + "\"},\"id\":4}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":4");
        
        // Verify destination file exists after copy
        std::string ls_response;
        int rc = execute_command_with_output("ls " + dest_path, ls_response);
        Expect(rc).ToBe(0);

        // Verify the destination file contains the identical content via RPC readFile
        std::string read_req = "{\"jsonrpc\":\"2.0\",\"method\":\"readFile\",\"params\":{\"fspath\":\"" + dest_path + "\"},\"id\":41}\n";
        write_to_server(server, read_req);
        std::string read_resp = read_line_from_server(server);

        Expect(read_resp).ToContain("\"success\":true");
        Expect(read_resp).ToContain("\"id\":41");
        Expect(read_resp).ToContain("\"SGVsbG8gY29weSE=\""); // same base64 data
      });
    });

    describe("create-dir", [&]() -> void {
      std::string dir_path;

      beforeEach([&]() -> void {
        dir_path = get_random_uss(ussTestDir) + "_newdir";
      });

      it("should properly create a directory via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"createFile\",\"params\":{\"fspath\":\"" + dir_path + "\",\"isDir\":true},\"id\":5}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":5");
        
        // Verify directory was created and has directory permissions
        std::string ls_response;
        int rc = execute_command_with_output("ls -ld " + dir_path, ls_response);
        Expect(rc).ToBe(0);
        Expect(ls_response).ToContain("drwx"); // directory prefix in permissions
      });
    });

    describe("create-file", [&]() -> void {
      std::string file_path;

      beforeEach([&]() -> void {
        file_path = get_random_uss(ussTestDir) + "_newfile";
      });

      it("should properly create a file via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"createFile\",\"params\":{\"fspath\":\"" + file_path + "\",\"isDir\":false},\"id\":6}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":6");
        
        std::string ls_response;
        int rc = execute_command_with_output("ls -l " + file_path, ls_response);
        Expect(rc).ToBe(0);
      });
    });

    describe("delete", [&]() -> void {
      std::string file_path;

      beforeEach([&]() -> void {
        file_path = get_random_uss(ussTestDir) + "_delfile";
        std::string response;
        execute_command_with_output(zowex_command + " uss create-file " + file_path, response);
      });

      it("should properly delete a file via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"deleteFile\",\"params\":{\"fspath\":\"" + file_path + "\"},\"id\":7}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":7");
        
        // Verify file no longer exists (ls should fail)
        std::string ls_response;
        int rc = execute_command_with_output("ls " + file_path, ls_response);
        Expect(rc).Not().ToBe(0);
      });
    });

    describe("issue", [&]() -> void {
      it("should properly issue a UNIX command via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"unixCommand\",\"params\":{\"commandText\":\"echo hello\"},\"id\":8}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":8");
        Expect(response).ToContain("\"data\":\"hello\\n\"");
      });
    });

    describe("list", [&]() -> void {
      std::string dir_path;

      beforeEach([&]() -> void {
        dir_path = get_random_uss(ussTestDir) + "_listdir";
        std::string response;
        // Create test directory with a file inside for listing
        execute_command_with_output(zowex_command + " uss create-dir " + dir_path, response);
        execute_command_with_output(zowex_command + " uss create-file " + dir_path + "/testfile", response);
      });

      it("should properly list files via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"listFiles\",\"params\":{\"fspath\":\"" + dir_path + "\"},\"id\":9}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":9");
        Expect(response).ToContain("\"testfile\"");
      });
    });

    describe("move", [&]() -> void {
      std::string src_path;
      std::string dest_path;

      beforeEach([&]() -> void {
        src_path = get_random_uss(ussTestDir) + "_mvsrc";
        dest_path = get_random_uss(ussTestDir) + "_mvdest";
        std::string response;
        execute_command_with_output(zowex_command + " uss create-file " + src_path, response);

        // Populate the source file with some sample content via RPC writeFile
        std::string write_req = "{\"jsonrpc\":\"2.0\",\"method\":\"writeFile\",\"params\":{\"fspath\":\"" + src_path + "\",\"data\":\"SGVsbG8gbW92ZSE=\"},\"id\":100}\n";
        write_to_server(server, write_req);
        read_line_from_server(server); // consume response
      });

      it("should properly move a file via RPC", [&]() -> void {
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"moveFile\",\"params\":{\"source\":\"" + src_path + "\",\"target\":\"" + dest_path + "\"},\"id\":10}\n";
        
        write_to_server(server, request);
        std::string response = read_line_from_server(server);

        Expect(response).ToContain("\"success\":true");
        Expect(response).ToContain("\"id\":10");
        
        // Verify move: source should not exist, destination should exist
        std::string ls_response;
        int rc1 = execute_command_with_output("ls " + src_path, ls_response);
        Expect(rc1).Not().ToBe(0); // source file gone
        int rc2 = execute_command_with_output("ls " + dest_path, ls_response);
        Expect(rc2).ToBe(0); // destination file exists

        // Verify the destination file contains the identical content via RPC readFile
        std::string read_req = "{\"jsonrpc\":\"2.0\",\"method\":\"readFile\",\"params\":{\"fspath\":\"" + dest_path + "\"},\"id\":101}\n";
        write_to_server(server, read_req);
        std::string read_resp = read_line_from_server(server);

        Expect(read_resp).ToContain("\"success\":true");
        Expect(read_resp).ToContain("\"id\":101");
        Expect(read_resp).ToContain("\"SGVsbG8gbW92ZSE=\""); // same base64 data
      });
    });

    describe("write and view", [&]() -> void {
      std::string file_path;

      beforeEach([&]() -> void {
        file_path = get_random_uss(ussTestDir) + "_wvfile";
      });

      it("should properly write and view a file via RPC", [&]() -> void {
        // Write operation: base64 encoded "Hello World!" is "SGVsbG8gV29ybGQh"
        std::string write_req = "{\"jsonrpc\":\"2.0\",\"method\":\"writeFile\",\"params\":{\"fspath\":\"" + file_path + "\",\"data\":\"SGVsbG8gV29ybGQh\"},\"id\":11}\n";
        
        write_to_server(server, write_req);
        std::string write_resp = read_line_from_server(server);

        Expect(write_resp).ToContain("\"success\":true");
        Expect(write_resp).ToContain("\"id\":11");
        
        // Read operation: verify we get back the same base64 content
        std::string read_req = "{\"jsonrpc\":\"2.0\",\"method\":\"readFile\",\"params\":{\"fspath\":\"" + file_path + "\"},\"id\":12}\n";
        
        write_to_server(server, read_req);
        std::string read_resp = read_line_from_server(server);

        Expect(read_resp).ToContain("\"success\":true");
        Expect(read_resp).ToContain("\"id\":12");
        Expect(read_resp).ToContain("\"SGVsbG8gV29ybGQh\""); // same base64 data
      });
    });

    describe("streaming integration", [&]() -> void {
      std::string file_path;

      beforeEach([&]() -> void {
        file_path = get_random_uss(ussTestDir) + "_stream_file";
      });

      it("should write via stream", [&]() -> void {
        int stream_id = 300;

        const std::string payload = "Hello USS Stream\nLine 2 of USS\n";
        const auto encoded = zbase64::encode(payload.c_str(), payload.size());
        const std::string encoded_payload(encoded.begin(), encoded.end());

        std::string pipe_path;
        std::thread writer;

        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"writeFile\",\"params\":{\"fspath\":\"" + file_path + "\",\"stream\":" + std::to_string(stream_id) + ",\"encoding\":\"binary\"},\"id\":45}\n";

        write_to_server(server, request);

        std::string notification = read_line_from_server(server);

        ExpectWithContext(notification, "Should be sendStream notification").ToContain("\"method\":\"sendStream\"");
        ExpectWithContext(notification, "Should have correct stream ID").ToContain("\"id\":" + std::to_string(stream_id));

        size_t pipe_path_start = notification.find("\"pipePath\":\"") + 12;
        size_t pipe_path_end = notification.find("\"", pipe_path_start);
        pipe_path = notification.substr(pipe_path_start, pipe_path_end - pipe_path_start);

        writer = std::thread([&]() -> void {
          int fd = -1;
          for (int attempt = 0; attempt < 100 && fd == -1; ++attempt) {
            fd = open(pipe_path.c_str(), O_WRONLY | O_NONBLOCK);
            if (fd == -1) {
              usleep(10000);
            }
          }
          if (fd != -1) {
            write(fd, encoded_payload.data(), encoded_payload.size());
            close(fd);
          }
        });

        std::string response = read_line_from_server(server);

        if (writer.joinable()) {
          writer.join();
        }

        Expect(response).ToContain("\"id\":45");
        Expect(response).ToContain("\"success\":true");

        std::string read_request = "{\"jsonrpc\":\"2.0\",\"method\":\"readFile\",\"params\":{\"fspath\":\"" + file_path + "\"},\"id\":46}\n";

        write_to_server(server, read_request);
        std::string read_response = read_line_from_server(server);

        Expect(read_response).ToContain("\"success\":true");
        ExpectWithContext(read_response, "Should contain streamed text").ToContain("SGVsbG8gVVNTIFN0cmVhbQ");
      });

      it("should read via stream", [&]() -> void {
        std::string test_content = "VVNTIFN0cmVhbSByZWFkIHRlc3QK"; // "USS Stream read test\n" base64
        std::string write_request = "{\"jsonrpc\":\"2.0\",\"method\":\"writeFile\",\"params\":{\"fspath\":\"" + file_path + "\",\"data\":\"" + test_content + "\"},\"id\":50}\n";

        write_to_server(server, write_request);
        std::string write_response = read_line_from_server(server);

        Expect(write_response).ToContain("\"success\":true");

        int stream_id = 400;

        std::string read_request = "{\"jsonrpc\":\"2.0\",\"method\":\"readFile\",\"params\":{\"fspath\":\"" + file_path + "\",\"stream\":" + std::to_string(stream_id) + "},\"id\":51}\n";

        write_to_server(server, read_request);

        std::string notification = read_line_from_server(server);
        Expect(notification).ToContain("\"method\":\"receiveStream\"");
        ExpectWithContext(notification, "Should have correct stream ID").ToContain("\"id\":" + std::to_string(stream_id));

        size_t pipe_path_start = notification.find("\"pipePath\":\"") + 12;
        size_t pipe_path_end = notification.find("\"", pipe_path_start);
        std::string output_pipe = notification.substr(pipe_path_start, pipe_path_end - pipe_path_start);

        std::string file_content;
        std::thread reader([&]() -> void {
          // Open FIFO for reading - this will block until the server opens it for writing
          int fd = open(output_pipe.c_str(), O_RDONLY);
          if (fd != -1) {
            char buffer[4096];
            ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
              buffer[bytes_read] = '\0';
              file_content = std::string(buffer, bytes_read);
            }
            close(fd);
          }
        });

        std::string read_response = read_line_from_server(server);

        reader.join();

        Expect(read_response).ToContain("\"id\":51");

        ExpectWithContext(file_content, "Streamed file should contain data").ToContain(test_content);
        ExpectWithContext(read_response, "Should be valid JSON-RPC response").ToContain("jsonrpc");
        ExpectWithContext(read_response, "Read streaming RPC interface should be supported").ToContain("\"id\":51");
      });
    });

  });
}

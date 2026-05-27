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
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <thread>
#include <chrono>
#include "zutils.hpp"
#include "zowex.server.test.hpp"
#include "zowex.job.server.test.hpp"
#include "../zjson.hpp"
#include "../zbase64.h"

using namespace ztst;

// Custom robust reader to handle long JSON lines without truncating at 512 bytes
static std::string read_rpc_response(ServerHandle &handle, int timeout_ms = 5000)
{
  fd_set read_fds;
  struct timeval timeout;
  int fd = fileno(handle.output_stream);
  if (fd == -1)
  {
    throw std::runtime_error("Failed to get file descriptor");
  }

  FD_ZERO(&read_fds);
  FD_SET(fd, &read_fds);

  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_usec = (timeout_ms % 1000) * 1000;

  int result = select(fd + 1, &read_fds, nullptr, nullptr, &timeout);
  if (result <= 0)
  {
    throw std::runtime_error("Timeout waiting for server output");
  }

  std::string line;
  int c;
  while ((c = fgetc(handle.output_stream)) != EOF)
  {
    line += static_cast<char>(c);
    if (c == '\n')
    {
      break;
    }
  }
  return line;
}

static std::string e2a_convert(const std::string &ebcdic_str)
{
  std::string ascii_str = ebcdic_str;
  __e2a_s(const_cast<char *>(ascii_str.c_str()));
  return ascii_str;
}

void zowex_job_server_tests()
{
  describe("jobs server tests", []() -> void {
    ServerHandle server;
    std::vector<std::string> clean_jobs;
    std::vector<std::string> clean_ds;
    std::vector<std::string> clean_uss;

    beforeAll([&]() -> void {
      server = start_server(zowex_server_command, true);
    });

    afterAll([&]() -> void {
      // Clean up jobs
      for (const auto &jobid : clean_jobs) {
        std::string out;
        execute_command_with_output(zowex_command + " job delete " + jobid, out);
      }
      // Clean up datasets
      for (const auto &ds : clean_ds) {
        std::string out;
        execute_command_with_output(zowex_command + " ds delete " + ds, out);
      }
      // Clean up USS files
      for (const auto &file : clean_uss) {
        unlink(file.c_str());
      }
      stop_server(server);
    });

    std::string active_jobid;
    std::string spool_dsn;
    int spool_id = -1;

    it("should submit JCL via submitJcl RPC", [&]() -> void {
      std::string jcl_raw = "//IEFBR14 JOB (IZUACCT),TEST,REGION=0M\n//RUN EXEC PGM=IEFBR14";
      std::string jcl_b64 = zbase64::encode(e2a_convert(jcl_raw));
      std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"submitJcl\",\"params\":{\"jcl\":\"" + jcl_b64 + "\"},\"id\":1}\n";
      
      write_to_server(server, request);
      std::string response = read_rpc_response(server);

      Expect(response).ToContain("\"success\":true");
      Expect(response).ToContain("\"id\":1");

      auto opt_val = zjson::from_str<zjson::Value>(response);
      Expect(opt_val.has_value()).ToBe(true);
      zjson::Value root = opt_val.value();
      zjson::Value result_obj = root["result"];
      active_jobid = result_obj["jobId"].as_string();
      Expect(active_jobid).Not().ToBe("");
      clean_jobs.push_back(active_jobid);

      // Wait for the job to complete and reach OUTPUT status
      bool completed = false;
      for (int i = 0; i < 20; ++i) {
        std::string status_req = "{\"jsonrpc\":\"2.0\",\"method\":\"getJobStatus\",\"params\":{\"jobId\":\"" + active_jobid + "\"},\"id\":100" + std::to_string(i) + "}\n";
        write_to_server(server, status_req);
        std::string status_resp = read_rpc_response(server);
        
        if (status_resp.find("OUTPUT") != std::string::npos) {
          completed = true;
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
      ExpectWithContext(completed, "Job should complete and reach OUTPUT status").ToBe(true);
    });

    it("should query job status via getJobStatus RPC", [&]() -> void {
      std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"getJobStatus\",\"params\":{\"jobId\":\"" + active_jobid + "\"},\"id\":2}\n";
      
      write_to_server(server, request);
      std::string response = read_rpc_response(server);

      Expect(response).ToContain("\"success\":true");
      Expect(response).ToContain("\"id\":2");
      Expect(response).ToContain("\"status\":\"OUTPUT\"");
    });

    it("should list jobs via listJobs RPC", [&]() -> void {
      // Use current user as owner to target specifically and prevent large list outputs
      std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"listJobs\",\"params\":{\"owner\":\"" + get_user() + "\"},\"id\":3}\n";
      
      write_to_server(server, request);
      std::string response = read_rpc_response(server);

      Expect(response).ToContain("\"success\":true");
      Expect(response).ToContain("\"id\":3");
      Expect(response).ToContain(active_jobid);
    });

    it("should list spool files via listSpools RPC", [&]() -> void {
      std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"listSpools\",\"params\":{\"jobId\":\"" + active_jobid + "\"},\"id\":4}\n";
      
      write_to_server(server, request);
      std::string response = read_rpc_response(server);

      Expect(response).ToContain("\"success\":true");
      Expect(response).ToContain("\"id\":4");

      auto opt_val = zjson::from_str<zjson::Value>(response);
      Expect(opt_val.has_value()).ToBe(true);
      zjson::Value root = opt_val.value();
      zjson::Value result_obj = root["result"];
      zjson::Value items_arr = result_obj["items"];
      
      Expect(items_arr.is_array()).ToBe(true);
      Expect(items_arr.as_array().size() > 0).ToBe(true);

      // Find JESMSGLG spool file or use the first one
      for (size_t i = 0; i < items_arr.as_array().size(); ++i) {
        std::string dd = items_arr[i]["ddname"].as_string();
        if (dd == "JESMSGLG" || spool_id == -1) {
          spool_id = static_cast<int>(items_arr[i]["id"].as_double());
          spool_dsn = items_arr[i]["dsname"].as_string();
        }
      }

      Expect(spool_id).Not().ToBe(-1);
      Expect(spool_dsn).Not().ToBe("");
    });

    it("should read spool file by id via readSpool RPC", [&]() -> void {
      std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"readSpool\",\"params\":{\"jobId\":\"" + active_jobid + "\",\"spoolId\":" + std::to_string(spool_id) + "},\"id\":5}\n";
      
      write_to_server(server, request);
      std::string response = read_rpc_response(server);

      Expect(response).ToContain("\"success\":true");
      Expect(response).ToContain("\"id\":5");
      Expect(response).ToContain("data");
    });

    xit("should view job file output via viewJobFile RPC (not exposed via RPC)", [&]() -> void {
      // vf (view job file) is not exposed via RPC on the server
    });

    it("should view job JCL via getJcl RPC", [&]() -> void {
      std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"getJcl\",\"params\":{\"jobId\":\"" + active_jobid + "\"},\"id\":7}\n";
      
      write_to_server(server, request);
      std::string response = read_rpc_response(server);

      Expect(response).ToContain("\"success\":true");
      Expect(response).ToContain("\"id\":7");
      Expect(response).ToContain("data");
    });

    xit("should watch job spool files via watchJob RPC (not exposed via RPC)", [&]() -> void {
      // wch (watch job) is not exposed via RPC on the server
    });

    it("should submit job from a dataset via submitJob RPC", [&]() -> void {
      std::string ds = get_random_ds(3);
      clean_ds.push_back(ds);
      
      // Create DS
      std::string out;
      int rc = execute_command_with_output(zowex_command + " ds create " + ds + " --dsorg PS --recfm FB --lrecl 80", out);
      Expect(rc).ToBe(0);

      // Write JCL to dataset
      rc = execute_command_with_output("echo '//IEFBR14 JOB (IZUACCT),TEST,REGION=0M\\n//RUN EXEC PGM=IEFBR14' | " + zowex_command + " ds write " + ds, out);
      Expect(rc).ToBe(0);

      std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"submitJob\",\"params\":{\"dsname\":\"" + ds + "\"},\"id\":9}\n";
      
      write_to_server(server, request);
      std::string response = read_rpc_response(server);

      Expect(response).ToContain("\"success\":true");
      Expect(response).ToContain("\"id\":9");

      auto opt_val = zjson::from_str<zjson::Value>(response);
      Expect(opt_val.has_value()).ToBe(true);
      zjson::Value root = opt_val.value();
      zjson::Value result_obj = root["result"];
      std::string sub_jobid = result_obj["jobId"].as_string();
      Expect(sub_jobid).Not().ToBe("");
      clean_jobs.push_back(sub_jobid);
    });

    it("should submit job from a USS file via submitUss RPC", [&]() -> void {
      std::string uss_file = "/tmp/zowex_srv_test_job_" + get_random_string(5) + ".jcl";
      clean_uss.push_back(uss_file);

      // Create local file with JCL
      FILE* f = fopen(uss_file.c_str(), "w");
      Expect(f != nullptr).ToBe(true);
      fputs("//IEFBR14 JOB (IZUACCT),TEST,REGION=0M\n//RUN EXEC PGM=IEFBR14\n", f);
      fclose(f);

      // Untag file to ensure it's untagged/binary, identical to the successful CLI tests
      std::string tag_out;
      execute_command_with_output("chtag -r " + uss_file, tag_out);

      std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"submitUss\",\"params\":{\"fspath\":\"" + uss_file + "\"},\"id\":10}\n";
      
      write_to_server(server, request);
      std::string response = read_rpc_response(server);

      Expect(response).ToContain("\"success\":true");
      Expect(response).ToContain("\"id\":10");

      auto opt_val = zjson::from_str<zjson::Value>(response);
      Expect(opt_val.has_value()).ToBe(true);
      zjson::Value root = opt_val.value();
      zjson::Value result_obj = root["result"];
      std::string sub_jobid = result_obj["jobId"].as_string();
      Expect(sub_jobid).Not().ToBe("");
      clean_jobs.push_back(sub_jobid);
    });

    std::string held_jobid;

    it("should hold job via holdJob RPC", [&]() -> void {
      // Submit JCL with TYPRUN=HOLD (base64-encoded)
      std::string held_jcl = "//HELDJOB JOB (IZUACCT),TEST,TYPRUN=HOLD,REGION=0M\n//RUN EXEC PGM=IEFBR14";
      std::string held_jcl_b64 = zbase64::encode(e2a_convert(held_jcl));
      std::string submit_req = "{\"jsonrpc\":\"2.0\",\"method\":\"submitJcl\",\"params\":{\"jcl\":\"" + held_jcl_b64 + "\"},\"id\":110}\n";
      write_to_server(server, submit_req);
      std::string submit_resp = read_rpc_response(server);
      Expect(submit_resp).ToContain("\"success\":true");

      auto opt_val = zjson::from_str<zjson::Value>(submit_resp);
      Expect(opt_val.has_value()).ToBe(true);
      zjson::Value root = opt_val.value();
      zjson::Value result_obj = root["result"];
      held_jobid = result_obj["jobId"].as_string();
      Expect(held_jobid).Not().ToBe("");
      clean_jobs.push_back(held_jobid);

      // Now issue holdJob RPC
      std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"holdJob\",\"params\":{\"jobId\":\"" + held_jobid + "\"},\"id\":11}\n";
      
      write_to_server(server, request);
      std::string response = read_rpc_response(server);

      Expect(response).ToContain("\"success\":true");
      Expect(response).ToContain("\"id\":11");
    });

    it("should release job via releaseJob RPC", [&]() -> void {
      std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"releaseJob\",\"params\":{\"jobId\":\"" + held_jobid + "\"},\"id\":12}\n";
      
      write_to_server(server, request);
      std::string response = read_rpc_response(server);

      Expect(response).ToContain("\"success\":true");
      Expect(response).ToContain("\"id\":12");
    });

    it("should cancel job via cancelJob RPC", [&]() -> void {
      std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"cancelJob\",\"params\":{\"jobId\":\"" + held_jobid + "\"},\"id\":13}\n";
      
      write_to_server(server, request);
      std::string response = read_rpc_response(server);

      Expect(response).ToContain("\"success\":true");
      Expect(response).ToContain("\"id\":13");
    });

    it("should delete job via deleteJob RPC", [&]() -> void {
      std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"deleteJob\",\"params\":{\"jobId\":\"" + held_jobid + "\"},\"id\":14}\n";
      
      write_to_server(server, request);
      std::string response = read_rpc_response(server);

      Expect(response).ToContain("\"success\":true");
      Expect(response).ToContain("\"id\":14");
    });
  });
}

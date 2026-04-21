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

#include <iostream>
#include <stdexcept>

#include "ztest.hpp"
#include "zjb.hpp"
#include <unistd.h>
#include <thread>
#include <chrono>

using namespace ztst;

void sleep_on_status(std::string status, std::string jobid);

void zjb_tests()
{

  describe("zjb tests", []() -> void
           {
             std::string jcl = "//IEFBR14$ JOB IZUACCT\n"
                          "//RUNBR14  EXEC PGM=IEFBR14\n";

             std::string hold_jcl = "//IEFBR14$ JOB IZUACCT,TYPRUN=HOLD\n"
                               "//RUNBR14  EXEC PGM=IEFBR14\n";

             it("should be able to list a job", [&]() -> void
                {
                  ZJB zjb = {0};
                  std::string owner = "*";  // all owners
                  std::string prefix = "*"; // any prefix
                  zjb.jobs_max = 1;    // limit to one
                  std::vector<ZJob> jobs;
                  int rc = zjb_list_by_owner(&zjb, owner, prefix, jobs);
                  ExpectWithContext(rc, zjb.diag.e_msg).ToBe(RTNCD_WARNING); // expect truncated list returned
                  Expect(jobs.size()).ToBe(zjb.jobs_max);                    // expect one job returned
                });

             it("should be able to submit JCL", [&]() -> void
                {
                  ZJB zjb = {0};
                  std::string jobid;

                  int rc = zjb_submit(&zjb, jcl, jobid);
                  ExpectWithContext(rc, zjb.diag.e_msg).ToBe(RTNCD_SUCCESS);
                  Expect(jobid).Not().ToBe("");

                  ZJob zjob;
                  std::string correlator = std::string(zjb.correlator, sizeof(zjb.correlator));

                  sleep_on_status("INPUT", correlator);

                  memset(&zjb, 0, sizeof(zjb));
                  rc = zjb_delete(&zjb, correlator);
                  ExpectWithContext(rc, zjb.diag.e_msg).ToBe(RTNCD_SUCCESS); 
                });

             it("should be able to view a submitted job", [&]() -> void
                {
                  ZJB zjb = {0};
                  std::string jobid;

                  int rc = zjb_submit(&zjb, jcl, jobid);
                  ExpectWithContext(rc, zjb.diag.e_msg).ToBe(RTNCD_SUCCESS);

                  ZJob zjob;
                  std::string correlator = std::string(zjb.correlator, sizeof(zjb.correlator));

                  memset(&zjb, 0, sizeof(zjb));
                  rc = zjb_view(&zjb, correlator, zjob);
                  ExpectWithContext(rc, zjb.diag.e_msg).ToBe(RTNCD_SUCCESS);

                  Expect(zjob.correlator).ToBe(correlator); // vefify submit correlator matches view status correlator

                  sleep_on_status("INPUT", correlator);

                  memset(&zjb, 0, sizeof(zjb));
                  rc = zjb_delete(&zjb, correlator);

                  ExpectWithContext(rc, zjb.diag.e_msg).ToBe(RTNCD_SUCCESS); 
                });

             it("should be able to delete a submitted job", [&]() -> void
                {
                  ZJB zjb = {0};
                  std::string jobid;

                  int rc = zjb_submit(&zjb, jcl, jobid);
                  ExpectWithContext(rc, zjb.diag.e_msg).ToBe(RTNCD_SUCCESS);

                  std::string correlator = std::string(zjb.correlator, sizeof(zjb.correlator));

                  sleep_on_status("INPUT", correlator);

                  memset(&zjb, 0, sizeof(zjb));
                  rc = zjb_delete(&zjb, correlator);

                  ExpectWithContext(rc, zjb.diag.e_msg).ToBe(RTNCD_SUCCESS); 
                });

             it("should be able to read job JCL", [&]() -> void
                {
                  ZJB zjb = {0};
                  std::string jobid;

                  int rc = zjb_submit(&zjb, jcl, jobid);
                  ExpectWithContext(rc, zjb.diag.e_msg).ToBe(RTNCD_SUCCESS);

                  std::string correlator = std::string(zjb.correlator, sizeof(zjb.correlator));
                  std::string returned_jcl;

                  sleep_on_status("INPUT", correlator);

                  memset(&zjb, 0, sizeof(zjb));
                  rc = zjb_read_job_jcl(&zjb, correlator, returned_jcl);

                  ExpectWithContext(rc, zjb.diag.e_msg).ToBe(RTNCD_SUCCESS); 
                });
              
              // https://github.com/zowe/zowex/issues/641
              xit("should be able to list and view SYSOUT files for INPUT jobs", [&]() -> void
                {
                  ZJB zjb = {0};
                  std::string jobid;

                  int rc = zjb_submit(&zjb, hold_jcl, jobid);
                  ExpectWithContext(rc, zjb.diag.e_msg).ToBe(RTNCD_SUCCESS);

                  ZJob zjob;
                  std::string correlator = std::string(zjb.correlator, sizeof(zjb.correlator));

                  sleep_on_status("INPUT", correlator);

                  std::vector<ZJobDD> dds;
                  memset(&zjb, 0, sizeof(zjb));
                  rc = zjb_list_dds(&zjb, correlator, dds);
                  ExpectWithContext(rc, zjb.diag.e_msg).ToBe(RTNCD_SUCCESS);
                  Expect(dds.size()).ToBeGreaterThan(0); // expect at least one DD returned

                  std::string content;
                  memset(&zjb, 0, sizeof(zjb));
                  rc = zjb_read_jobs_output_by_key(&zjb, correlator, dds[0].key, content);
                  ExpectWithContext(rc, zjb.diag.e_msg).ToBe(RTNCD_SUCCESS);
                  Expect(content).Not().ToBe(""); // expect some content returned

                  memset(&zjb, 0, sizeof(zjb));
                  rc = zjb_delete(&zjb, correlator);
                  ExpectWithContext(rc, zjb.diag.e_msg).ToBe(RTNCD_SUCCESS); 
                }); });
}

void sleep_on_status(std::string status, std::string jobid)
{
  int index = 0;
  while (true)
  {
    ZJB zjb = {};
    ZJob zjob = {};
    int rc = zjb_view(&zjb, jobid, zjob);
    const int max_retries = 1000;

    if (RTNCD_SUCCESS != rc)
    {
      std::string error =
          "Error: could not view job: '" + jobid + "' rc: " + std::to_string(rc) + "\n'  " + std::string(zjb.diag.e_msg) + "'";
      throw std::runtime_error(error);
    }

    if (index >= max_retries)
    {
      std::string error =
          "Error: for job: '" + jobid + "' reached max retries of " + std::to_string(max_retries);
      throw std::runtime_error(error);
    }
    if (zjob.full_status == status)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10 * 5)); // wait for job to exit INPUT
    }
    else
    {
      break;
    }
    index++;
  }
}
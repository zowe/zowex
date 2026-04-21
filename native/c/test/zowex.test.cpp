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

#include <cstdlib>
#include <string>
#include "ztest.hpp"
#include "ztype.h"
#include "zutils.hpp"
#include "zowex.test.hpp"
#include "zowex.ds.test.hpp"
#include "zowex.uss.test.hpp"
#include "zowex.job.test.hpp"
#include "zowex.system.test.hpp"
#include "zoweax.console.test.hpp"
#include "zowex.tso.test.hpp"

using namespace ztst;

void zowex_tests()
{

  describe("zowex",
           []() -> void
           {
             it("should list a version of the tool",
                []() -> void
                {
                  int rc = 0;
                  std::string response;
                  rc = execute_command_with_output(zowex_command + " version", response);
                  ExpectWithContext(rc, response).ToBe(0);
                  Expect(response).ToContain("zowex");
                  Expect(response).ToContain("Version:");

                  // Version information
                  static const std::regex rev(R"(Version:\s*(\S+))");
                  std::smatch mv;

                  Expect(std::regex_search(response, mv, rev)).ToBe(true);
                  const std::string version = mv[1].str();

                  Expect(version.length()).ToBeGreaterThanOrEqualTo(5); // X.X.X at minimum

                  Expect(response).ToContain("Build Date:");

                  // Build date information
                  static const std::regex red(R"(Build Date:\s*([^\r\n]+))");
                  std::smatch md;

                  Expect(std::regex_search(response, md, red)).ToBe(true);
                  const std::string buildDate = md[1].str();

                  Expect(buildDate.length()).ToBeGreaterThanOrEqualTo(11); // MMM DD YYYY at minimum

                  Expect(response).Not().ToContain("unknown");
                });
             it("should list a short version of the tool",
                []() -> void
                {
                  int rc = 0;
                  std::string response;
                  rc = execute_command_with_output(zowex_command + " --version", response);
                  ExpectWithContext(rc, response).ToBe(0);
                  Expect(response).Not().ToContain("zowex");
                  Expect(response).Not().ToContain("Version");
                  Expect(response).Not().ToContain("Build Date");
                  Expect(response).Not().ToContain("unknown");

                  static const std::regex rev(R"(^\s*(\d+\.\d+\.\d+)(?:[-+][^\r\n]*)?\s*$)");
                  std::smatch mv;

                  Expect(std::regex_search(response, mv, rev)).ToBe(true);
                  const std::string version = mv[1].str();

                  Expect(version.length()).ToBeGreaterThanOrEqualTo(5); // X.X.X at minimum
                });
#ifdef RELEASE_BUILD
             it("should remain less than 10mb in size",
#else
             xit("should remain less than 10mb in size",
#endif
                []() -> void
                {
                  std::string response;
                  execute_command_with_output("cat ../build-out/zowex | wc -c", response);
                  int file_size = std::stoi(response);
                  ExpectWithContext(file_size, response).ToBeLessThan(10 * 1024 * 1024);
                });
             zowex_ds_tests();
             zowex_uss_tests();
             zowex_job_tests();
             zowex_system_tests();
             zowex_tso_tests();
           });

  describe("zoweax", []() -> void
           { zoweax_console_tests(); });
}

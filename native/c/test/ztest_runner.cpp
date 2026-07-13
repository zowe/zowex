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

#include "zstorage.test.hpp"
#include "zut.test.hpp"
#include "zjb.test.hpp"
#include "zds.test.hpp"
#include "zcn.test.hpp"
#include "zrecovery.test.hpp"
#include "zmetal.test.hpp"
#include "zusf.test.hpp"
#include "zbase64.test.hpp"
#include "zowex.test.hpp"
#include "zowex.uss.test.hpp"
#include "zlogger.test.hpp"
#include "zwto.test.hpp"
#include "parser.test.hpp"
#include "zstd.test.hpp"
#include "zjson.test.hpp"
#include "zowex.server.test.hpp"
#include "zowex.uss.server.test.hpp"
#include "zowex.ds.server.test.hpp"
#include "zowex.job.server.test.hpp"
#include "server/worker.test.hpp"
#include "server/validator.test.hpp"
#include "server_logger.test.hpp"
#include "ztest.hpp"
#include <cstdlib>

using namespace ztst;

#pragma runopts("TRAP(ON,NOSPIE)")
int main(int argc, char *argv[])
{
  // Keep test log output confined to the test working directory rather than
  // the real user's home directory (the default when ZOWEX_LOGS_DIR is unset)
  setenv("ZOWEX_LOGS_DIR", "logs", 1);

  int rc = tests(
      argc, argv,
      []() -> void
      {
        zowex_uss_tests();
        zut_tests();
        zjb_tests();
        zds_tests();
        zcn_tests();
        zstorage_tests();
        zrecovery_tests();
        zmetal_tests();
        zusf_tests();
        zbase64_tests();
        zowex_tests();
        zlogger_tests();
        zwto_tests();
        parser_tests();
        zstd_tests();
        zjson_tests();
        zowex_server_tests();
        zowex_uss_server_tests();
        zowex_ds_server_tests();
        zowex_job_server_tests();
        server_worker_tests();
        server_validator_tests();
        server_logger_tests();
      });

  return rc;
}

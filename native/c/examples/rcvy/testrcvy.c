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

#include "ihasdwa.h"
#include "zmetal.h"
#include "zrecovery.h"
#include "zsetjmp.h"
#include "zwto.h"
#include "zecb.h"

#pragma prolog(ABEXIT, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(ABEXIT, " ZWEEPILG ")
void ABEXIT(SDWA *sdwa, void *abexit_data)
{
  zwto_debug("@TEST called on abend");
  int *i = (int *)abexit_data;
  zwto_debug("@TEST abexit_data %d", *i);
}

#pragma prolog(PERCEXIT, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(PERCEXIT, " ZWEEPILG ")
void PERCEXIT(void *perc_exit_data)
{
  zwto_debug("@TEST called to percolate");
}

int main()
{

  // ------------------------------------------------------------

  // ZSETJMP_ENV zenv = {0};
  // int rc = zsetjmp(&zenv);
  // if (rc == 0)
  // {
  //   zwto_debug("zsetjmp returned 0");
  // }
  // else
  // {
  //   zwto_debug("zsetjmp returned %d", rc);
  // }
  // if (rc == 0)
  // {
  //   zwto_debug("zlongjmp called");
  //   zlongjmp(&zenv);
  // }
  // else
  // {
  //   zwto_debug("zlongjmp not called");
  // }
  // return 0;

  // ------------------------------------------------------------

  // ZRCVY_ENV zenv = {0};
  // zenv.abexit = ABEXIT;
  // zenv.perc_exit = PERCEXIT;
  // int i = 7;
  // zenv.abexit_data = &i;

  // zwto_debug("@TEST main");

  // if (0 == enable_recovery(&zenv))
  // {
  //   zwto_debug("@TEST in if");
  //   s0c3_abend(2);
  // }
  // else
  // {
  //   zwto_debug("@TEST in else");
  //   // s0c3_abend(2);
  // }
  // zwto_debug("@TEST outside of if/else");

  // disable_recovery(&zenv);

  // zwto_debug("@TEST exiting");

  // ------------------------------------------------------------

  ZRCVY_ENV zenv = {0};
  zenv.abexit = ABEXIT;
  zenv.perc_exit = PERCEXIT;
  int i = 7;
  zenv.abexit_data = &i;

  // zwto_debug("@TEST main");

  if (0 == enable_recovery(&zenv))
  {
    // disable_recovery(&zenv);
    s0c3_abend(2);
  }
  else
  {
    zwto_debug("@TEST in else");
    // disable_recovery(&zenv);
    return 1;
    // s0c3_abend(3);
  }

  // zwto_debug("@TEST outside of if/else");
  // s0c3_abend(2);

  // zwto_debug("@TEST before disable recovery");
  disable_recovery(&zenv);

  zwto_debug("@TEST after disable recovery");

  return 0;
}

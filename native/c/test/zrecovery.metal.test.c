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

#include "zwto.h"
#include "zrecovery.metal.test.h"
#include "zrecovery.h"
#include "zssi.h"

#pragma prolog(ZRCVYEN, " ZWEPROLG NEWDSA=(YES,4) ")
#pragma epilog(ZRCVYEN, " ZWEEPILG ")
int ZRCVYEN()
{
  ZRCVY_ENV zenv = {0};
  if (0 == enable_recovery(&zenv))
  {
    s0c3_abend(0);
  }
  else
  {
  }

  disable_recovery(&zenv);
  return 0;
}

#pragma prolog(ZRCVYDIS, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(ZRCVYDIS, " ZWEEPILG ")
int ZRCVYDIS()
{
  ZRCVY_ENV zenv = {0};
  if (0 == enable_recovery(&zenv))
  {
    // do nothing
  }
  disable_recovery(&zenv);
  s0c3_abend(0);
  return 0;
}

#pragma prolog(ZRCVYNST, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(ZRCVYNST, " ZWEEPILG ")
int ZRCVYNST(int *PTR64 rc1, int *PTR64 rc2)
{
  ZRCVY_ENV zenv1 = {0};
  ZRCVY_ENV zenv2 = {0};
  *rc1 = 0;
  *rc2 = 0;

  if (0 == enable_recovery(&zenv1))
  {
    if (0 == enable_recovery(&zenv2))
    {
      s0c3_abend(0); // Trigger inner abend -> goes to zenv2 else
    }
    else
    {
      *rc2 = 1; // zenv2 recovery entered!
      s0c3_abend(0); // Trigger outer abend -> goes to zenv1 else
    }
  }
  else
  {
    *rc1 = 1; // zenv1 recovery entered!
  }

  disable_recovery(&zenv2);
  disable_recovery(&zenv1);
  return 0;
}

#pragma prolog(ZRCVYNCL, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(ZRCVYNCL, " ZWEEPILG ")
int ZRCVYNCL()
{
  ZRCVY_ENV zenv1 = {0};
  ZRCVY_ENV zenv2 = {0};

  if (0 == enable_recovery(&zenv1))
  {
    if (0 == enable_recovery(&zenv2))
    {
      // do nothing, just establish both and exit inner
    }
  }

  disable_recovery(&zenv2);
  disable_recovery(&zenv1);
  s0c3_abend(0); // This should propagate up and not be handled by either
  return 0;
}

#pragma prolog(ZRCVYTHR, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(ZRCVYTHR, " ZWEEPILG ")
int ZRCVYTHR(int *PTR64 thread_id, int *PTR64 rc)
{
  ZRCVY_ENV zenv = {0};
  *rc = -1;

  if (0 == enable_recovery(&zenv))
  {
    s0c3_abend(0);
  }
  else
  {
    *rc = *thread_id; // successful recovery
  }

  disable_recovery(&zenv);
  return 0;
}

#pragma prolog(ZRCVYNUL, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(ZRCVYNUL, " ZWEEPILG ")
int ZRCVYNUL()
{
  // Call iefssreq with a NULL SSOB pointer (which triggers a safe error return rather than abending)
  return iefssreq(NULL);
}

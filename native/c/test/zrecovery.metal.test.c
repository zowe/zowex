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
  enable_recovery(&zenv);
  disable_recovery(&zenv);
  s0c3_abend(0);
  return 0;
}

#pragma prolog(ZRCVYNST_INNER, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(ZRCVYNST_INNER, " ZWEEPILG ")
static int ZRCVYNST_INNER(int *PTR64 rc2) ATTRIBUTE(noinline);
static int ZRCVYNST_INNER(int *PTR64 rc2)
{
  ZRCVY_ENV zenv2 = {0};
  if (0 == enable_recovery(&zenv2))
  {
    s0c3_abend(0); // Trigger inner abend -> goes to zenv2 else
  }
  else
  {
    *rc2 = 1; // zenv2 recovery entered!
    disable_recovery(&zenv2);
    s0c3_abend(0); // Trigger outer abend -> goes to zenv1 else
  }
  return 0;
}

#pragma prolog(ZRCVYNST_HELPER, " ZWEPROLG NEWDSA=(YES,128),SAVE=BAKR ")
#pragma epilog(ZRCVYNST_HELPER, " ZWEEPILG ")
static int ZRCVYNST_HELPER(int *PTR64 rc2) ATTRIBUTE(noinline);
static int ZRCVYNST_HELPER(int *PTR64 rc2)
{
  return ZRCVYNST_INNER(rc2);
}

#pragma prolog(ZRCVYNST, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(ZRCVYNST, " ZWEEPILG ")
int ZRCVYNST(int *PTR64 rc1, int *PTR64 rc2)
{
  ZRCVY_ENV zenv1 = {0};
  *rc1 = 0;
  *rc2 = 0;

  if (0 == enable_recovery(&zenv1))
  {
    ZRCVYNST_HELPER(rc2);
  }
  else
  {
    *rc1 = 1; // zenv1 recovery entered!
  }

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
    // Establish inner recovery as part of nested setup
    enable_recovery(&zenv2);
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

#pragma prolog(ZRCVYSTI, " ZWEPROLG NEWDSA=(YES,128),SAVE=BAKR ")
#pragma epilog(ZRCVYSTI, " ZWEEPILG ")
static int ZRCVYSTI(void) ATTRIBUTE(noinline);
static int ZRCVYSTI(void)
{
  __asm(
      "*                                                   \n"
      " LGHI   5,16462            Dirty R5                 \n"
      " LGHI   6,16463            Dirty R6                 \n"
      " LGHI   7,16464            Dirty R7                 \n"
      " LGHI   8,16465            Dirty R8                 \n"
      " LGHI   9,16466            Dirty R9                 \n"
      " LGHI   10,16467           Dirty R10                \n"
      " LGHI   11,16468           Dirty R11                \n"
      "*                                                    "
      :
      :
      : "r5", "r6", "r7", "r8", "r9", "r10", "r11");
  return 0;
}

#pragma prolog(ZRCVYSTK, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(ZRCVYSTK, " ZWEEPILG ")
int ZRCVYSTK(void)
{
  unsigned long long int regs_before[7] = {0};
  unsigned long long int regs_after[7] = {0};
  int i;

  __asm(
      "*                                                   \n"
      " LGHI   5,1005             Set R5                   \n"
      " LGHI   6,1006             Set R6                   \n"
      " LGHI   7,1007             Set R7                   \n"
      " LGHI   8,1008             Set R8                   \n"
      " LGHI   9,1009             Set R9                   \n"
      " LGHI   10,1010            Set R10                  \n"
      " LGHI   11,1011            Set R11                  \n"
      " STMG   5,11,%0            Capture before R5-R11    \n"
      "*                                                    "
      : "=m"(regs_before[0])
      :
      : "r5", "r6", "r7", "r8", "r9", "r10", "r11");

  ZRCVYSTI();

  __asm(
      "*                                                   \n"
      " STMG   5,11,%0            Capture after R5-R11     \n"
      "*                                                    "
      : "=m"(regs_after[0])
      :
      :);

  for (i = 0; i < 7; i++)
  {
    if (regs_before[i] != regs_after[i])
    {
      return -1;
    }
  }

  return 0;
}

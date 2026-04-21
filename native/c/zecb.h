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

#ifndef ZECBWAIT_H
#define ZECBWAIT_H

#include "ihaecb.h"
#include "ztype.h"

typedef struct ecb ECB;

#if defined(__IBM_METAL__)
#define STIMER_WAIT(time)                                     \
  __asm(                                                      \
      "*                                                  \n" \
      " SLGR  2,2       Init register                     \n" \
      " TAM   ,         AMODE64??                         \n" \
      " JM    *+4+4+2   No, skip switching                \n" \
      " OILH  2,X'8000' Set AMODE31 flag                  \n" \
      " SAM31 ,         Set AMODE31                       \n" \
      "*                                                  \n" \
      " SYSSTATE PUSH       Save SYSSTATE                 \n" \
      " SYSSTATE AMODE64=NO                               \n" \
      "*                                                  \n" \
      " STIMER WAIT,BINTVL=(%0)                           \n" \
      "*                                                  \n" \
      " TMLH  2,X'8000' Did we switch AMODE??             \n" \
      " JNO   *+4+2     No, skip restore                  \n" \
      " SAM64 ,         Set AMODE64                       \n" \
      "*                                                  \n" \
      " NILH  2,X'7FFF' Clear flag if set                 \n" \
      "*                                                  \n" \
      " SYSSTATE POP    Restore SYSSTATE                  \n" \
      "*                                                    " \
      :                                                       \
      : "r"(time)                                             \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define STIMER_WAIT(time)
#endif // __IBM_METAL__

#if defined(__IBM_METAL__)
#define STIMERM_MODEL(stimermm)                               \
  __asm(                                                      \
      "*                                                  \n" \
      " STIMERM SET,"                                         \
      "MF=L                                               \n" \
      "*                                                    " \
      : "DS"(stimermm));
#else
#define STIMERM_MODEL(stimermm) void *stimermm;
#endif

STIMERM_MODEL(stimerm_model); // make this copy in static storage

// NOTE(Keloskly): this may not need to be AMODE31 always; likewise elsewhere
#if defined(__IBM_METAL__)
#define STIMERM_SET(time, parm, exit, id, plist)              \
  __asm(                                                      \
      "*                                                  \n" \
      " SLGR  2,2       Init register                     \n" \
      " TAM   ,         AMODE64??                         \n" \
      " JM    *+4+4+2   No, skip switching                \n" \
      " OILH  2,X'8000' Set AMODE31 flag                  \n" \
      " SAM31 ,         Set AMODE31                       \n" \
      "*                                                  \n" \
      " SYSSTATE PUSH       Save SYSSTATE                 \n" \
      " SYSSTATE AMODE64=NO                               \n" \
      "*                                                  \n" \
      " STIMERM SET,BINTVL=(%2),"                             \
      "WAIT=NO,"                                              \
      "ID=%1,"                                                \
      "PARM=%3,"                                              \
      "EXIT=(%4),"                                            \
      "MF=(E,%0)                                          \n" \
      "*                                                  \n" \
      " TMLH  2,X'8000' Did we switch AMODE??             \n" \
      " JNO   *+4+2     No, skip restore                  \n" \
      " SAM64 ,         Set AMODE64                       \n" \
      "*                                                  \n" \
      " NILH  2,X'7FFF' Clear flag if set                 \n" \
      "*                                                  \n" \
      " SYSSTATE POP    Restore SYSSTATE                  \n" \
      "*                                                    " \
      : "+m"(plist), "=m"(id)                                 \
      : "r"(time), "m"(parm), "r"(exit)                       \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define STIMERM_SET(time, parm, exit, id, plist) \
  {                                              \
    id++;                                        \
  }
#endif // __IBM_METAL__

#if defined(__IBM_METAL__)
#define STIMERM_CANCEL(rc, plist)                             \
  __asm(                                                      \
      "*                                                  \n" \
      " SLGR  2,2       Init register                     \n" \
      " TAM   ,         AMODE64??                         \n" \
      " JM    *+4+4+2   No, skip switching                \n" \
      " OILH  2,X'8000' Set AMODE31 flag                  \n" \
      " SAM31 ,         Set AMODE31                       \n" \
      "*                                                  \n" \
      " SYSSTATE PUSH       Save SYSSTATE                 \n" \
      " SYSSTATE AMODE64=NO                               \n" \
      "*                                                  \n" \
      " STIMERM CANCEL,"                                      \
      "ID=ALL,"                                               \
      "MF=(E,%0)                                          \n" \
      "*                                                  \n" \
      " TMLH  2,X'8000' Did we switch AMODE??             \n" \
      " JNO   *+4+2     No, skip restore                  \n" \
      " SAM64 ,         Set AMODE64                       \n" \
      "*                                                  \n" \
      " ST 15,%1        Save RC                           \n" \
      "*                                                  \n" \
      " NILH  2,X'7FFF' Clear flag if set                 \n" \
      "*                                                  \n" \
      " SYSSTATE POP    Restore SYSSTATE                  \n" \
      "*                                                    " \
      : "+m"(plist), "=m"(rc)                                 \
      :                                                       \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define STIMERM_CANCEL(rc, plist)
#endif // __IBM_METAL__

#if defined(__IBM_METAL__)
#define ECB_POST(ecb, code)                                   \
  __asm(                                                      \
      "*                                                  \n" \
      " L    15,%1                                        \n" \
      "*                                                  \n" \
      " POST %0,(15)                                      \n" \
      "*                                                    " \
      : "+m"(*ecb)                                            \
      : "m"(code)                                             \
      : "r0", "r1", "r14", "r15");
#else
#define ECB_POST(ecb, code)
#endif // __IBM_METAL__

#if defined(__IBM_METAL__)
#define ECB_WAIT(ecb)                                         \
  __asm(                                                      \
      "*                                                  \n" \
      " WAIT ECB=%0                                       \n" \
      "*                                                    " \
      : "+m"(*ecb)                                            \
      :                                                       \
      : "r0", "r1", "r14", "r15");
#else
#define ECB_WAIT(ecb)
#endif // __IBM_METAL__

#if defined(__IBM_METAL__)
#define ECBS_WAIT(count, list)                                \
  __asm(                                                      \
      "*                                                  \n" \
      " WAIT (%1),ECBLIST=%0                              \n" \
      "*                                                    " \
      : "+m"(*list)                                           \
      : "r"(count)                                            \
      : "r0", "r1", "r14", "r15");
#else
#define ECBS_WAIT(count, list)
#endif // __IBM_METAL__

#if defined(__IBM_METAL__)

static void ecb_post(ECB *ecb, int code)
{
  ECB_POST(ecb, code);
}

static void ecb_wait(ECB *ecb)
{
  ECB_WAIT(ecb);
}

// TODO(Kelosky): additionl debugging / testing needed if adopted
static void ecbs_wait(
    int events, volatile ECB *ecbList[],
    int ecbListCount)
{

  union overEcb
  {
    volatile ECB *PTR32 ecb;
    unsigned int word;
  } oEcb = {0};

  if (ecbListCount >= 1)
  {

    oEcb.ecb = ecbList[ecbListCount - 1];
    oEcb.word |= 0x80000000;

    ecbList[ecbListCount - 1] = oEcb.ecb;

    ECBS_WAIT(events, ecbList);

    oEcb.word &= ~0x80000000;
    ecbList[ecbListCount - 1] = oEcb.ecb;
  }

  return;
}

static void ecbs_wait_on_one(
    volatile ECB *ecbList[],
    int ecbListCount)
{

  ecbs_wait(1, ecbList, ecbListCount);

  return;
}

// 1 = 0.01 seconds
static void time_wait(unsigned int interval)
{
  STIMER_WAIT(&interval);

  return;
}

typedef void (*PTR32 zcli_stimer)(void *PTR32);
// NOTE(Kelosky): seems unworkable in LE, should use LE timer equivalent
static void timer(unsigned int time, zcli_stimer cb, void *PTR32 parameter)
{
  int id = 0;                        // TODO(Kelosky): return & allow cancel by
  STIMERM_MODEL(dsa_stimerm_model);  // stack var
  dsa_stimerm_model = stimerm_model; // copy model
  STIMERM_SET(&time, parameter, cb, id, dsa_stimerm_model);
}

static int cancel_timers()
{
  int rc = 0;
  STIMERM_MODEL(dsa_stimerm_model);  // stack var
  dsa_stimerm_model = stimerm_model; // copy model
  STIMERM_CANCEL(rc, dsa_stimerm_model);
  return rc;
}
#endif // __IBM_METAL__

#endif // ECBWAIT_H

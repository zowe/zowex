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

#ifndef ZSSI_H
#define ZSSI_H

#include "zmetal.h"
#include "zssitype.h"
#include "iefjsqry.h"
#include "zmetal.h"

#if defined(__IBM_METAL__)

#ifndef _IEFJESCT_DSECT
#define _IEFJESCT_DSECT
#pragma insert_asm(" IEFJESCT ")
#endif

#ifndef _CVT_DSECT
#define _CVT_DSECT
#pragma insert_asm(" CVT   LIST=NO,DSECT=YES ")
#endif

#ifndef _IEFJSRC_DSECT
#define _IEFJSRC_DSECT
#pragma insert_asm(" IEFJSRC ")
#endif

#endif

#if defined(__IBM_METAL__)
#define IEFSSREQ(ssob, rc)                                    \
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
      " LA 1,%0        -> SSSOB                           \n" \
      "*                                                  \n" \
      " IEFSSREQ                                          \n" \
      "*                                                  \n" \
      " ST 15,%1                                          \n" \
      "*                                                  \n" \
      " TMLH  2,X'8000' Did we switch AMODE??             \n" \
      " JNO   *+4+2     No, skip restore                  \n" \
      " SAM64 ,         Set AMODE64                       \n" \
      "*                                                  \n" \
      " SYSSTATE POP    Restore SYSSTATE                  \n" \
      "*                                                    " \
      : "+m"(*(unsigned char *)ssob), "=m"(rc)                \
      :                                                       \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define IEFSSREQ(ssob, rc)
#endif

#if defined(__IBM_METAL__)
#define IEFSSI_QUERY_MODEL(iefssiqm)   \
  __asm(                               \
      "*                           \n" \
      " IEFSSI MF=(L,IEFQ)         \n" \
      "*                           \n" \
      : "DS"(iefssiqm));
#else
#define IEFSSI_QUERY_MODEL(iefssiqm)
#endif

IEFSSI_QUERY_MODEL(iefssi_query_model); // make this copy in static storage

#if defined(__IBM_METAL__)
#define IEFSSI_QUERY(filter, area, rc, rsn, plist)            \
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
      " IEFSSI REQUEST=QUERY,"                                \
      "SUBNAME=%3,"                                           \
      "WORKAREA=%0,"                                          \
      "PLISTVER=MAX,"                                         \
      "RETCODE=%1,"                                           \
      "RSNCODE=%2,"                                           \
      "MF=(E,%4,COMPLETE)                                 \n" \
      "*                                                  \n" \
      " TMLH  2,X'8000' Did we switch AMODE??             \n" \
      " JNO   *+4+2     No, skip restore                  \n" \
      " SAM64 ,         Set AMODE64                       \n" \
      "*                                                  \n" \
      " SYSSTATE POP    Restore SYSSTATE                  \n" \
      "*                                                    " \
      : "=m"(*area), "=m"(rc), "=m"(rsn)                      \
      : "m"(filter), "m"(plist)                               \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define IEFSSI_QUERY(filter, area, rc, rsn, plist)
#endif

typedef struct jqry___header JQRY_HEADER;

static int iefssi_query(JQRY_HEADER *PTR32 *PTR32 area, int *PTR32 rsn, const char *filter)
{
  int rc = 0;
  char filter_truncated[4] = {};
  memset(filter_truncated, ' ', sizeof(filter_truncated));
  strncpy(filter_truncated, filter, sizeof(filter_truncated));
  IEFSSI_QUERY_MODEL(iefssi_query_model);
  IEFSSI_QUERY(filter_truncated, area, rc, *rsn, iefssi_query_model);
  return rc;
}

// https://www.ibm.com/docs/en/zos/3.1.0?topic=subsystem-making-request-iefssreq-macro
static int iefssreq(SSOB *PTR32 *PTR32 ssob)
{
  int rc = 0;
  IEFSSREQ(ssob, rc);
  return rc;
}

#endif

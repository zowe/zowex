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

#ifndef ZJBM31_H
#define ZJBM31_H

#include "ztype.h"
#include "zssitype.h"
#include "zjsytype.h"

#if defined(__IBM_METAL__)
#ifndef _IHAECVT_DSECT
#define _IHAECVT_DSECT
#pragma insert_asm(" IHAECVT ")
#endif
#ifndef _CVT_DSECT
#define _CVT_DSECT
#pragma insert_asm(" CVT   LIST=NO,DSECT=YES ")
#endif
#endif

#if defined(__IBM_METAL__)
#define IAZSYMBL(jsym, rc)                                    \
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
      " LA 1,%0        -> JSYMPARM                        \n" \
      "*                                                  \n" \
      " IAZSYMBL PARM=(1)                                 \n" \
      "*                                                  \n" \
      " ST 15,%1                                          \n" \
      "*                                                  \n" \
      " TMLH  2,X'8000' Did we switch AMODE??             \n" \
      " JNO   *+4+2     No, skip restore                  \n" \
      " SAM64 ,         Set AMODE64                       \n" \
      "*                                                  \n" \
      " SYSSTATE POP    Restore SYSSTATE                  \n" \
      "*                                                    " \
      : "+m"(*(unsigned char *)jsym), "=m"(rc)                \
      :                                                       \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define IAZSYMBL(jsym, rc)
#endif

// https://www.ibm.com/docs/en/zos/3.1.0?topic=symbols-jes-symbol-iazsymbl-macro
static int iazsymbl(JSYMPARM *PTR32 jsym)
{
  int rc = 0;
  IAZSYMBL(jsym, rc);
  return rc;
}

#endif

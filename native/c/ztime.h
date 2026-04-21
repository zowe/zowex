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

#ifndef ZTIME_H
#define ZTIME_H

#include "ihapsa.h"
#include "cvt.h"

#if defined(__IBM_METAL__)
#define TIME(tod)                                             \
  __asm(                                                      \
      "*                                                  \n" \
      " TIME STCK,%0                                      \n" \
      "*                                                    " \
      : "=m"(tod)                                             \
      :                                                       \
      : "r0", "r1", "r14", "r15");
#else
#define TIME(tod)
#endif

#if defined(__IBM_METAL__)
#define TIME_LOCAL(time, date)                                 \
  __asm(                                                       \
      "*                                                   \n" \
      " TIME DEC,ZONE=LT,LINKAGE=SVC                       \n" \
      "*                                                   \n" \
      " ST 0,%0                                            \n" \
      " ST 1,%1                                            \n" \
      "*                                                    "  \
      : "=m"(time), "=m"(date)                                 \
      :                                                        \
      : "r0", "r1", "r14", "r15");
#else
#define TIME_LOCAL(time, date)
#endif

#if defined(__IBM_METAL__)
#define STCKCONV_MODEL(stckconvm)      \
  __asm(                               \
      "*                           \n" \
      " STCKCONV MF=L              \n" \
      "*                           \n" \
      : "DS"(stckconvm));
#else
#define STCKCONV_MODEL(stckconvm)
#endif

STCKCONV_MODEL(stckconv_model);

#if defined(__IBM_METAL__)
#define STCKCONV(output, tod, plist, rc, format)              \
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
      " STCKCONV STCKVAL=%2,"                                 \
      "CONVVAL=%0,"                                           \
      "TIMETYPE=DEC,"                                         \
      "DATETYPE=" #format ","                                 \
      "MF=(E,%3)                                          \n" \
      "*                                                  \n" \
      " ST    15,%1     Save RC                           \n" \
      "*                                                  \n" \
      " TMLH  2,X'8000' Did we switch AMODE??             \n" \
      " JNO   *+4+2     No, skip restore                  \n" \
      " SAM64 ,         Set AMODE64                       \n" \
      "*                                                  \n" \
      " SYSSTATE POP    Restore SYSSTATE                  \n" \
      "*                                                    " \
      : "=m"(output),                                         \
        "=m"(rc)                                              \
      : "m"(tod),                                             \
        "m"(plist)                                            \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define STCKCONV(output, tod, plist, rc, format)
#endif

#if defined(__IBM_METAL__)
#define CONVTOD_MODEL(convtodm)       \
  __asm(                              \
      "*                          \n" \
      " CONVTOD MF=L              \n" \
      "*                          \n" \
      : "DS"(convtodm));
#else
#define CONVTOD_MODEL(convtodm)
#endif

CONVTOD_MODEL(convtod_model);

#if defined(__IBM_METAL__)
#define CONVTOD(tod, input, plist, rc, format)                \
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
      " CONVTOD TODVAL=%0,"                                   \
      "CONVVAL=%2,"                                           \
      "TIMETYPE=DEC,"                                         \
      "DATETYPE=" #format ","                                 \
      "MF=(E,%3)                                          \n" \
      "*                                                  \n" \
      " ST    15,%1     Save RC                           \n" \
      "*                                                  \n" \
      " TMLH  2,X'8000' Did we switch AMODE??             \n" \
      " JNO   *+4+2     No, skip restore                  \n" \
      " SAM64 ,         Set AMODE64                       \n" \
      "*                                                  \n" \
      " SYSSTATE POP    Restore SYSSTATE                  \n" \
      "*                                                    " \
      : "=m"(tod),                                            \
        "=m"(rc)                                              \
      : "m"(input),                                           \
        "m"(plist)                                            \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define CONVTOD(tod, input, plist, rc, format)
#endif

#if defined(__IBM_METAL__)
#define CONVETOD(tod, input, plist, rc, format)               \
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
      " CONVTOD ETODVAL=%0,"                                  \
      "CONVVAL=%2,"                                           \
      "TIMETYPE=BIN,"                                         \
      "DATETYPE=" #format ","                                 \
      "MF=(E,%3)                                          \n" \
      "*                                                  \n" \
      " ST    15,%1     Save RC                           \n" \
      "*                                                  \n" \
      " TMLH  2,X'8000' Did we switch AMODE??             \n" \
      " JNO   *+4+2     No, skip restore                  \n" \
      " SAM64 ,         Set AMODE64                       \n" \
      "*                                                  \n" \
      " SYSSTATE POP    Restore SYSSTATE                  \n" \
      "*                                                    " \
      : "=m"(tod),                                            \
        "=m"(rc)                                              \
      : "m"(input),                                           \
        "m"(plist)                                            \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define CONVETOD(tod, input, plist, rc, format)
#endif

static void time(unsigned long long *tod)
{
  TIME(*tod);
}

typedef union
{
  unsigned int timei;
  struct
  {
    unsigned char HH;
    unsigned char MM;
    unsigned char SS;
    unsigned char unused;
  } times;
} TIME_UNION;

// TODO(Kelosky): this must be AMODE 31
static void time_local(unsigned int *time, unsigned int *date)
{
  TIME_LOCAL(*time, *date);
}

typedef struct
{
  unsigned char month;
  unsigned char day;
  unsigned char year[2];
} DATE_MMDDYYYY;

typedef struct
{
  unsigned char time[8];
  union
  {
    DATE_MMDDYYYY mmddyyyy;
  } date;
  unsigned char unused[4];
} TIME_STRUCT;

static int stckconv(unsigned long long *tod, TIME_STRUCT *time_struct)
{
  int rc = 0;

  STCKCONV_MODEL(dsa_stckconv_model);
  dsa_stckconv_model = stckconv_model;

  STCKCONV(*time_struct, *tod, dsa_stckconv_model, rc, MMDDYYYY);

  return rc;
}

static int convtod(TIME_STRUCT *time_struct, unsigned long long *tod)
{
  int rc = 0;

  CONVTOD_MODEL(dsa_convtod_model);
  dsa_convtod_model = convtod_model;

  CONVTOD(*tod, *time_struct, dsa_convtod_model, rc, YYDDD);

  return rc;
}

typedef unsigned char etod_t[16];

static int convetod(TIME_STRUCT *time_struct, etod_t *etod)
{
  int rc = 0;

  CONVTOD_MODEL(dsa_convtod_model);
  dsa_convtod_model = convtod_model;

  CONVETOD(*etod[0], *time_struct, dsa_convtod_model, rc, YYYYMMDD);

  return rc;
}

#endif

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

#include "zutm31.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "zstorage.h"
#include "zdbg.h"
#include "ztype.h"
#include "zutcall24.h"
#include "zwto.h"
#include "iefzpmap.h"
#include "csvdlaa.h"

typedef int (*ZUTCALL24_FN)(unsigned int ep, void *PTR32 parm_list) ATTRIBUTE(amode31);

#if defined(__IBM_METAL__)
#ifndef _IAZJSAB_DSECT
#define _IAZJSAB_DSECT
#pragma insert_asm(" IAZJSAB ")
#endif
#ifndef _IHAASCB_DSECT
#define _IHAASCB_DSECT
#pragma insert_asm(" IHAASCB ")
#endif
#ifndef _IHAASSB_DSECT
#define _IHAASSB_DSECT
#pragma insert_asm(" IHAASSB ")
#endif
#ifndef _IHAPSA_DSECT
#define _IHAPSA_DSECT
#pragma insert_asm(" IHAPSA ")
#endif
#ifndef _IKJTCB_DSECT
#define _IKJTCB_DSECT
#pragma insert_asm(" IKJTCB ")
#endif
#ifndef _IHASTCB_DSECT
#define _IHASTCB_DSECT
#pragma insert_asm(" IHASTCB ")
#endif
#endif

#if defined(__IBM_METAL__)
#define IAZXJSAB(user, rc)                                    \
  __asm(                                                      \
      "*                                                  \n" \
      " L 2,%0         -> User                            \n" \
      "*                                                  \n" \
      " IAZXJSAB READ,USERID=(2)                          \n" \
      "*                                                  \n" \
      " ST 15,%1                                          \n" \
      "*                                                    " \
      : "+m"(user), "=m"(rc)                                  \
      :                                                       \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define IAZXJSAB(user, rc)
#endif

#if defined(__IBM_METAL__)
#define IEFPRMLB(parmlib, buffer, callername, rc, rsn)        \
  __asm(                                                      \
      "*                                                  \n" \
      " IEFPRMLB REQUEST=LIST,"                               \
      "BUFFER=%3,"                                            \
      "CALLERNAME=%4,"                                        \
      "MF=(E,%0,COMPLETE)                                 \n" \
      "*                                                  \n" \
      " ST 15,%1                                          \n" \
      " ST 0,%2                                           \n" \
      "*                                                    " \
      : "+m"(parmlib), "=m"(rc), "=m"(rsn), "=m"(buffer)      \
      : "m"(callername)                                       \
      : "r0", "r1", "r14", "r15");
#else
#define IEFPRMLB(parmlib, buffer, callername, rc, rsn)
#endif

#if defined(__IBM_METAL__)
#define IEFPRMLB_MODEL(iefprmlbm)                             \
  __asm(                                                      \
      "*                                                  \n" \
      " IEFPRMLB MF=(L,PARMLIB)                           \n" \
      "*                                                    " \
      : "DS"(iefprmlbm));
#else
#define IEFPRMLB_MODEL(iefprmlbm) void *iefprmlbm;
#endif

#if defined(__IBM_METAL__)
#define CSVAPF_MODEL(csvapfm)                                 \
  __asm(                                                      \
      "*                                                  \n" \
      " CSVAPF MF=(L,APF,0D)                              \n" \
      "*                                                    " \
      : "DS"(csvapfm));
#else
#define CSVAPF_MODEL(csvapfm) void *csvapfm;
#endif

#if defined(__IBM_METAL__)
#define CSVAPF(answer, answer_len, rc, rsn, plist)            \
  __asm(                                                      \
      "*                                                  \n" \
      " LLGT  2,%0                                        \n" \
      "*                                                  \n" \
      " CSVAPF REQUEST=LIST,"                                 \
      "ANSAREA=(2),"                                          \
      "ANSLEN=(%3),"                                          \
      "MF=(E,%4)                                          \n" \
      "*                                                  \n" \
      " ST 15,%1                                          \n" \
      " ST 0,%2                                           \n" \
      "*                                                    " \
      : "=m"(answer), "=m"(rc), "=m"(rsn)                     \
      : "r"(answer_len), "m"(plist)                           \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define CSVAPF(answer, answer_len, rc, rsn, plist)
#endif

// https://www.ibm.com/docs/en/zos/3.1.0?topic=ixg-iazxjsab-obtain-information-about-currently-running-job
int zutm1gur(char user[8])
{
  int rc = 0;
  IAZXJSAB(user, rc);
  return rc;
}

int zutm1apf(struct apfhdr *answer, int *answer_len, int *rsn)
{
  int rc = 0;
  CSVAPF_MODEL(csvapfm);
  CSVAPF(answer, answer_len, rc, *rsn, csvapfm);
  return rc;
}

#if defined(__IBM_METAL__)
#define CSVDYNL_MODEL(csvdynlm)                               \
  __asm(                                                      \
      "*                                                  \n" \
      " CSVDYNL MF=(L,DYNL,0D)                            \n" \
      "*                                                    " \
      : "DS"(csvdynlm));
#else
#define CSVDYNL_MODEL(csvdynlm) void *csvdynlm;
#endif

#if defined(__IBM_METAL__)
#define CSVDYNL(answer, answer_len, rc, rsn, plist, linklist_name) \
  __asm(                                                           \
      "*                                                  \n"      \
      " LLGT  2,%0                                        \n"      \
      "*                                                  \n"      \
      " CSVDYNL REQUEST=LIST,"                                     \
      "ANSAREA=(2),"                                               \
      "ANSLEN=(%3),"                                               \
      "LISTLNKNAME=%5,"                                            \
      "MF=(E,%4)                                          \n"      \
      "*                                                  \n"      \
      " ST 15,%1                                          \n"      \
      " ST 0,%2                                           \n"      \
      "*                                                    "      \
      : "=m"(answer), "=m"(rc), "=m"(rsn)                          \
      : "r"(answer_len), "m"(plist), "m"(linklist_name)            \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define CSVDYNL(answer, answer_len, rc, rsn, plist, linklist_name)
#endif

#pragma prolog(zutm1dyn, " ZWEPROLG NEWDSA=(YES,8) ")
#pragma epilog(zutm1dyn, " ZWEEPILG ")
int zutm1dyn(DLAAHDR *answer, int *answer_len, int *rsn)
{
  int rc = 0;
  char linklist_name[16] = "CURRENT";
  CSVDYNL_MODEL(csvdynlm);
  CSVDYNL(answer, answer_len, rc, *rsn, csvdynlm, linklist_name);
  return rc;
}

// Invoke a dynamically loaded module through the below-the-line shim (zutcall24.s),
// honoring the module's AMODE so it returns correctly to this above-the-line (RMODE 31)
// caller.
//   ep   - entry point from load_module. May not work for `load_module31` (AMODE stripped)
//   parm - the caller-built, VL-terminated R1 parameter list, below the 16 MB line
#pragma prolog(zutm1call24, " ZWEPROLG NEWDSA=(YES,4),LOC24=YES ")
#pragma epilog(zutm1call24, " ZWEEPILG ")
int zutm1call24(unsigned int ep, void *PTR32 parm)
{
  int rc = 0;

  // Copy the position-independent shim into 24-bit storage and run the module.
  unsigned int shim[64];
  int tlen = ZUTCALQ();
  if (tlen > (int)sizeof(shim))
  {
    // The shim grew past the buffer; refuse rather than overflow the stack. If this
    // ever fires, enlarge shim[] to match ZUCALLEN.
    return RTNCD_FAILURE;
  }
  int (*src)() = ZUTCAL24;
  memcpy(shim, (void *)src, tlen);

  union
  {
    void *PTR32 addr;
    ZUTCALL24_FN fn;
  } call = {.addr = shim};
  rc = call.fn(ep, parm);

  return rc;
}

typedef struct prm___list___buffer PRM_LIST_BUFFER;

typedef struct
{
  char dsn[44];
  char volser[6];
  char filler[6];
} entry;

int zutm1lpl(ZDIAG *diag, int *num_dsns, PARMLIB_DSNS *dsns)
{
  int rc = 0;
  int rsn = 0;
  char *callername = "ZOWEX           ";
  IEFPRMLB_MODEL(iefprmlbm);
  IEFPRMLB(iefprmlbm, rc, rc, rc, rsn);

#define MAX_ENTRIES 11
#define MAX_SIZE 60

  struct
  {
    PRM_LIST_BUFFER prm_list_buffer;
    unsigned char entries[MAX_ENTRIES][MAX_SIZE];
  } buffer = {0};

  buffer.prm_list_buffer.prm___list___version = prm___list___ver1;
  buffer.prm_list_buffer.prm___list___buff___size = sizeof(buffer);

  entry *entries = (entry *)((char *)&buffer + sizeof(buffer.prm_list_buffer.prm___list___header));

  IEFPRMLB(iefprmlbm, buffer, callername, rc, rsn);

  if (0 != rc)
  {
    ZDIAG_SET_MSG(diag, "Error: could not list parmlibs rc: '%d' rsn: '%d'", rc, rsn);
    return rc;
  }

  *num_dsns = buffer.prm_list_buffer.prm___num___parmlib___ds;
  for (int i = 0; i < buffer.prm_list_buffer.prm___num___parmlib___ds; i++)
  {
    memcpy(dsns->dsn[i].val, &entries[i].dsn, sizeof(dsns->dsn[i].val));
  }
  return rc;
}
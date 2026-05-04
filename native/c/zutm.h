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

#ifndef ZUTM_H
#define ZUTM_H

#include "ztype.h"
#include "zprmtype.h"
#include "iefjsqry.h"

#if defined(__cplusplus) && defined(__MVS__)
extern "OS"
{
#elif defined(__cplusplus)
extern "C"
{
#endif

#define RET_ARG_MAX_LEN 260
#define MSG_ENTRIES 25

#define ALLOC_STRING_INDEX 0
#define RTDDN_INDEX 1
#define RTDSN_INDEX 1
#define MSG_INDEX 2

#define LAST_PARAMETER_INDEX MSG_INDEX // NOTE(Kelosky): this must be set to the last parameter index

#define INPUT_PARAMETERS LAST_PARAMETER_INDEX + 1

  typedef struct jqry___header JQRY_HEADER;
  typedef struct jqry___subsys___entry JQRY_SUBSYS_ENTRY;
  typedef struct jqry___vt___entry JQRY_VT_ENTRY;

  typedef struct
  {
    short len;
    char str[RET_ARG_MAX_LEN];
    unsigned int rtdd : 1;
    unsigned int rtdsn : 1;
  } BPXWDYN_RET_ARG;

  typedef BPXWDYN_RET_ARG BPXWDYN_PARM;

  typedef struct
  {
    unsigned int code;
    char response[RET_ARG_MAX_LEN * MSG_ENTRIES + 1];
    char ddname[9];
    char dsname[45]; // NOTE(Kelosky): not implemented yet
  } BPXWDYN_RESPONSE;

  typedef struct
  {
    char input[128];
    char output[128];
    int length;
    int reserve_1;
  } SYMBOL_DATA;

  /**
   * @brief Storage Free 64 bit areas
   * @param PTR64 The data to free
   * @return The return code from the service (always 0)
   */
  int ZUTMFR64(void *PTR64);

  /**
   * @brief Storage Release 24 or 31 bit areas
   * @param size The size of the data to release
   * @param data The data to release
   * @return The return code from the service
   */
  int ZUTMSREL(int *size, void *PTR64);

  /**
   * @brief Storage Get 64 bit areas
   * @param PTR64 The new storage area address
   * @param size The size of the data to get
   * @return The return code from the service (always 0)
   */
  int ZUTMGT64(void **PTR64, int *PTR64);

  int ZUTMGUSR(char[8]);
  int ZUTWDYN(BPXWDYN_PARM *, BPXWDYN_RESPONSE *);
  int ZUTEDSCT();
  int ZUTSYMBP(SYMBOL_DATA *);
  int ZUTSRCH(const char *);
  int ZUTRUN(ZDIAG *, const char *, const char *);
  void ZUTDBGMG(const char *);
  unsigned char ZUTMGKEY();
  int ZUTMLPLB(ZDIAG *, int *, PARMLIB_DSNS *);
  int ZUTSSIQ(ZDIAG *, JQRY_HEADER **, const char *filter);
  int ZUTCVTD(const char *ptr, char *time_out);
  int ZUTNOAUT();

#if defined(__cplusplus)
}
#endif

#endif

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

#include <stdio.h>
#include <string.h>
#include <builtins.h>
#include "zwto.h"
#include "zcnm31.h"
#include "zmetal.h"
#include "ieavm105.h"

typedef struct mdb MDB;
typedef struct mdbg MDBG;
typedef struct mdbscp MDBSCP;
typedef struct mdbt MDBT;

#if defined(__IBM_METAL__)
#define MSOPER_MODEL(mcsoperm)      \
  __asm(                            \
      "*                        \n" \
      " MCSOPER MF=(L,ZCNOPR)   \n" \
      "*                        \n" \
      : "DS"(mcsoperm));
#else
#define MSOPER_MODEL(mcsoperm) void *mcsoperm;
#endif

MSOPER_MODEL(mcsoper_model);

#if defined(__IBM_METAL__)
#define MCSOPER_ACTIVATE(id, name, ecb, alet, mcscsa, rc, rsn, plist)    \
  __asm(                                                                 \
      "*                                                             \n" \
      " MCSOPER REQUEST=ACTIVATE,"                                       \
      "NAME=%7,"                                                         \
      "CONSID=%4,"                                                       \
      "TERMNAME=%7,"                                                     \
      "MCSCSA=%2,"                                                       \
      "MCSCSAA=%3,"                                                      \
      "MSGDLVRY=SEARCH,"                                                 \
      "RTNCODE=%0,"                                                      \
      "RSNCODE=%1,"                                                      \
      "MSGECB=%5,"                                                       \
      "MF=(E,%6)                                                     \n" \
      "*                                                             \n" \
      : "=m"(rc), "=m"(rsn), "=m"(alet), "=m"(mcscsa), "=m"(id)          \
      : "m"(ecb), "m"(plist), "m"(name)                                  \
      : "r0", "r1", "r14", "r15");
#else
#define MCSOPER_ACTIVATE(id, name, ecb, alet, mcscsa, rc, rsn, plist)
#endif

int zcnm1act(ZCN *zcn)
{
  PSW psw = {0};
  get_psw(&psw);
  int mode_switch = psw.data.bits.p ? 1 : 0;

  MSOPER_MODEL(dsa_mcsoper_model);
  dsa_mcsoper_model = mcsoper_model;

  unsigned int *PTR32 e = (unsigned int *PTR32)zcn->ecb;
  unsigned int *PTR32 a = (unsigned int *PTR32)zcn->area;

  strcpy(zcn->diag.service_name, "MCSOPER_ACTIVATE");

  if (mode_switch)
  {
    mode_sup();
  }
  MCSOPER_ACTIVATE(zcn->id, zcn->console_name, *e, zcn->alet, a, zcn->diag.service_rc, zcn->diag.service_rsn, dsa_mcsoper_model);
  if (mode_switch)
  {
    mode_prob();
  }

  if (0 != zcn->diag.service_rc)
    zcn->diag.detail_rc = ZCN_RTNCD_SERVICE_FAILURE; // if the service failed, note in RC
  return zcn->diag.detail_rc;
}

#if defined(__IBM_METAL__)
#define MGCRE_MODEL(mgcrem)         \
  __asm(                            \
      "*                        \n" \
      " MGCRE MF=L              \n" \
      "*                        \n" \
      : "DS"(mgcrem));
#else
#define MGCRE_MODEL(mgcrem) void *mgcrem;
#endif

MGCRE_MODEL(mgcre_model);

#if defined(__IBM_METAL__)
#define MGCRE(id, message, cart, authcmdx, plist)                       \
  __asm(                                                                \
      "*                                                            \n" \
      " LA 2,%1                                                     \n" \
      "*                                                            \n" \
      " MGCRE TEXT=(2),"                                                \
      "CART=%2,"                                                        \
      "AUTHCMDX=%3,"                                                    \
      "CONSID=%0,"                                                      \
      "MF=(E,%4)                                                    \n" \
      "*                                                            \n" \
      :                                                                 \
      : "m"(id), "m"(message), "m"(cart), "m"(authcmdx), "m"(plist)     \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define MGCRE(id, message, cart, authcmdx, plist)
#endif

// MGCETXT	Command text - Table 2 "MGCRE mapping", structure "MGCETEXT" https://www.ibm.com/docs/en/zos/3.2.0?topic=rqe-mgcre-information
#ifndef MGCRTEXT
#define MGCRTEXT 126
#endif

// NOTE(Kelosky): this piece is permitted in AMODE64 - for consistency, it remains here
int zcnm1put(ZCN *zcn, const char *command)
{
  PSW psw = {0};
  get_psw(&psw);
  int mode_switch = psw.data.bits.p ? 1 : 0;
  unsigned char key = get_key();
  unsigned char key_zero = 0;

  MGCRE_MODEL(dsa_mgcre_model);
  dsa_mgcre_model = mgcre_model;

  struct MGCETEXT
  {
    short commandLen;
    char command[MGCRTEXT];
  } commandBuffer = {0};

  unsigned short authcmdx = 0x8000; // 1000000000000000 - Master Authority - https://www.ibm.com/docs/en/zos/3.1.0?topic=commands-mgcre-execute-form
  unsigned short *authcmdxp = &authcmdx;

  /* Use precision specifier %.*s as snprintf is unavailable in Metal C */
  commandBuffer.commandLen = sprintf(commandBuffer.command, "%.*s", (int)(sizeof(commandBuffer.command) - 1), command);
  char cart[8] = "ZOWECART";

  strcpy(zcn->diag.service_name, "MGCRE");

  if (mode_switch)
  {
    mode_sup();
  }
  set_key(&key_zero);
  MGCRE(zcn->id, commandBuffer, cart, authcmdxp, dsa_mgcre_model);
  set_key(&key);
  if (mode_switch)
  {
    mode_prob();
  }

  zcn->diag.service_rc = 0;
  zcn->diag.service_rsn = 0;

  return RTNCD_SUCCESS; // NOTE(Kelosky): no return code for MGCRE
}

#if defined(__IBM_METAL__)
#define MCSOPMSG_MODEL(mcsopmsgm)     \
  __asm(                              \
      "*                          \n" \
      " MCSOPMSG MF=(L,ZCNOPM)    \n" \
      "*                          \n" \
      : "DS"(mcsopmsgm));
#else
#define MCSOPMSG_MODEL(mcsopmsgm) void *mcsopmsgm;
#endif

MCSOPMSG_MODEL(mcsopmsg_model);

#if defined(__IBM_METAL__)
#define MCSOPMSG_GETMSG(id, area, alet, cart, rc, rsn, plist) \
  __asm(                                                      \
      "*                                                \n"   \
      " SLR  2,2                                        \n"   \
      " IAC  2                                          \n"   \
      " SACF X'200'        ASC mode                     \n"   \
      "*                                                \n"   \
      " MCSOPMSG REQUEST=GETMSG,"                             \
      "CONSID=%4,"                                            \
      "CART=%5,"                                              \
      "CMDRESP=YES,"                                          \
      "RTNCODE=%0,"                                           \
      "RSNCODE=%1,"                                           \
      "MF=(E,%6)                                        \n"   \
      "*                                                \n"   \
      " ST   1,%2                                       \n"   \
      " STAM 1,1,%3                                     \n"   \
      "*                                                \n"   \
      "*SACF X'000'                                     \n"   \
      " SACF 0(2)          Restore                      \n"   \
      "*                                                \n"   \
      : "=m"(rc), "=m"(rsn), "=m"(area), "=m"(alet)           \
      : "m"(id), "m"(cart), "m"(plist)                        \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define MCSOPMSG_GETMSG(id, area, alet, cart, rc, rsn, plist)
#endif

#if defined(__IBM_METAL__)
#define MCSOPMSG_RESUME(id, rc, rsn, plist)                 \
  __asm(                                                    \
      "*                                                \n" \
      " SLR  2,2                                        \n" \
      " IAC  2                                          \n" \
      " SACF X'200'        ASC mode                     \n" \
      "*                                                \n" \
      " MCSOPMSG REQUEST=RESUME,"                           \
      "CONSID=%2,"                                          \
      "RTNCODE=%0,"                                         \
      "RSNCODE=%1,"                                         \
      "MF=(E,%3)                                        \n" \
      "*                                                \n" \
      " SACF 0(2)          Restore                      \n" \
      "*                                                \n" \
      : "=m"(rc), "=m"(rsn)                                 \
      : "m"(id), "m"(plist)                                 \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define MCSOPMSG_RESUME(id, rc, rsn, plist)
#endif

#if defined(__IBM_METAL__)
#define CLEAR_ARS()          \
  __asm(                     \
      "*                 \n" \
      " LAM 0,15,=16F'0' \n" \
      "*                 \n" \
      :                      \
      :                      \
      :);
#else
#define CLEAR_ARS() \
  {                 \
  } // NOTE(Kelosky): if literals become unreachable, pass in object of zeros
#endif

// ___     ____________________________________________
//  A     | MDB Header section                         |
//  |     |                                            |
//  |     |  MDBLEN -------                            |
//  |     |____________________________________________|
//  |     | __________________________________________ |
//  |     || General Object (MDBG) - Required         ||
//  |     ||                                          ||
//  |     ||   One only                               ||
//  |     ||__________________________________________||
//  |     | __________________________________________ |
//  |     || Control Program Object (MDBC) - Optional ||
//  M     ||                                          ||
//  D     ||   One only                               ||
//  B     ||__________________________________________||
//  L     |  __________________________________________|
//  E     || Text Object  (MDBT) - Optional           ||
//  N     ||                                          ||
//  |     ||   One for each line of message text      ||
//  |     ||__________________________________________||
//  |     |                     .                      |
//  |     |                     .                      |
//  |     |                     .                      |
//  |     | __________________________________________ |
//  |     || Text Object  (MDBT) - Optional           ||
//  |     ||                                          ||
//  |     ||   One for each line of message text      ||
//  |     ||__________________________________________||
//  V__   |____________________________________________|

#define _MI .BUILTN 1 // for the __far nonsense

#define RTNCD_RESUME_OK 4

#define RTNCD_NO_MORE_MESSAGES 8
#define RSNCD_NO_MORE_MESSAGES 0

static void extract_control_info(unsigned int alet, unsigned char *offset, ZCN *zcn) ATTRIBUTE(armode);
static void extract_control_info(unsigned int alet, unsigned char *offset, ZCN *zcn)
{
  MDBSCP *FAR mdbscp = __set_far_ALET_offset(alet, offset);
  char *FAR reply_id = __set_far_ALET_offset(0, zcn->reply_id);

  zcn->reply_id_len = mdbscp->mdbcrpyl;
  if (zcn->reply_id_len > 8) {
    zcn->reply_id_len = 8;
  }
  __far_memcpy(reply_id, mdbscp->mdbcrpyi, zcn->reply_id_len);
}

static int extract_text(unsigned int alet, unsigned char *offset, int len, char *resp) ATTRIBUTE(armode);
static int extract_text(unsigned int alet, unsigned char *offset, int len, char *resp)
{
  char *FAR text = __set_far_ALET_offset(alet, offset + sizeof(MDBT)); // -> text

  char b[256] = {0};
  char *FAR bp = __set_far_ALET_offset(0, b);

  if (len > 256) {
    len = 256;
  }

  __far_memcpy(bp, text, len);

  memcpy(resp, b, len);
  resp += len;
  memcpy(resp, "\n", 1);

  return len + 1;
}

int zcnm1get(ZCN *zcn, char *resp)
{
  PSW psw = {0};
  get_psw(&psw);
  int mode_switch = psw.data.bits.p ? 1 : 0;

  CLEAR_ARS();

  MCSOPMSG_MODEL(dsa_mcsopmsg_model);
  dsa_mcsopmsg_model = mcsopmsg_model;

  char cart[8] = "ZOWECART";
  void *area = NULL;
  int alet = 0;

  zcn->diag.service_rc = 0;

  // while there are records
  while (0 == zcn->diag.service_rc)
  {
    if (mode_switch)
    {
      mode_sup();
    }
    CLEAR_ARS();
    MCSOPMSG_GETMSG(zcn->id, area, alet, cart, zcn->diag.service_rc, zcn->diag.service_rsn, dsa_mcsopmsg_model);
    CLEAR_ARS();
    if (mode_switch)
    {
      mode_prob();
    }

    strcpy(zcn->diag.service_name, "MCSOPMSG_GETMSG");

    // break if no more messages
    if (RTNCD_NO_MORE_MESSAGES == zcn->diag.service_rc && RSNCD_NO_MORE_MESSAGES == zcn->diag.service_rsn)
      break;

    // break if unexpected error
    if (0 != zcn->diag.service_rc)
    {
      return zcn->diag.service_rc;
    }

    // NOTE(Kelosky): treat everything as MDB since every structure begins with 2 byte len and 2 byte type
    unsigned char *FAR p = __set_far_ALET_offset(alet, area);
    MDB *FAR mdb = (MDB * FAR) p;

    // obtain total length
    short type = 0;
    short total_len = mdb->mdblen;

    // general entry
    total_len -= sizeof(MDB);
    p = p + sizeof(MDB);
    mdb = (MDB * FAR) p;

    // while entries
    while (total_len > 0)
    {
      memcpy(&type, mdb->mdbtype, sizeof(short));
      if (mdbtobj == type)
      {
        void *offset = __get_far_offset(p); // -> MDB
        int text_len = mdb->mdblen - sizeof(MDBT);

        if ((zcn->buffer_size - zcn->buffer_size_needed) >= text_len + 1)
        {
          int bytes_written = extract_text(alet, offset, text_len, resp);
          resp += bytes_written;
        }
        else
        {
          zcn->diag.detail_rc = ZCN_RTNCD_INSUFFICIENT_BUFFER;
        }
        zcn->buffer_size_needed += text_len + 1; // return max number of bytes needed
      }

      if (mdbcobj == type) // if control type, attempt to copy reply information
      {
        void *offset = __get_far_offset(p); // -> MDBSCP
        extract_control_info(alet, offset, zcn);
      }

      total_len -= mdb->mdblen;
      p = p + mdb->mdblen;
      mdb = (MDB * FAR) p;
    }
  }

  zcn->diag.service_rc = 0;
  zcn->diag.service_rsn = 0;

  // https://www.ibm.com/docs/en/zos/3.1.0?topic=messages-description
  mode_sup();
  CLEAR_ARS();
  MCSOPMSG_RESUME(zcn->id, zcn->diag.service_rc, zcn->diag.service_rsn, dsa_mcsopmsg_model);
  CLEAR_ARS();
  mode_prob();

  strcpy(zcn->diag.service_name, "MCSOPMSG_RESUME");

  if (RTNCD_RESUME_OK == zcn->diag.service_rc)
  {
    return RTNCD_SUCCESS;
  }

  zcn->diag.detail_rc = ZCN_RTNCD_SERVICE_FAILURE;
  return zcn->diag.detail_rc;
}

#if defined(__IBM_METAL__)
#define MCSOPER_DEACTIVATE(id, rc, rsn, plist)    \
  __asm(                                          \
      "*                                      \n" \
      " MCSOPER REQUEST=DEACTIVATE,"              \
      "CONSID=%2,"                                \
      "RTNCODE=%0,"                               \
      "RSNCODE=%1,"                               \
      "MF=(E,%3)                              \n" \
      "*                                      \n" \
      : "=m"(rc), "=m"(rsn)                       \
      : "m"(id), "m"(plist)                       \
      : "r0", "r1", "r14", "r15");
#else
#define MCSOPER_DEACTIVATE(id, rc, rsn, plist)
#endif

int zcnm1dea(ZCN *zcn)
{
  PSW psw = {0};
  get_psw(&psw);
  int mode_switch = psw.data.bits.p ? 1 : 0;

  MSOPER_MODEL(dsa_mcsoper_model);
  dsa_mcsoper_model = mcsoper_model;

  strcpy(zcn->diag.service_name, "MCSOPER_DEACTIVATE");
  if (mode_switch)
  {
    mode_sup();
  }
  MCSOPER_DEACTIVATE(zcn->id, zcn->diag.service_rc, zcn->diag.service_rsn, dsa_mcsoper_model);
  if (mode_switch)
  {
    mode_prob();
  }

  if (0 != zcn->diag.service_rc)
    zcn->diag.detail_rc = ZCN_RTNCD_SERVICE_FAILURE; // if the service failed, note in RC
  return zcn->diag.detail_rc;
}

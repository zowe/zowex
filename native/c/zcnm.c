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
#include "zwto.h"
#include "zcnm31.h"
#include "zcntype.h"
#include "zecb.h"
#include "zrecovery.h"

#pragma prolog(ZCNACT, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(ZCNACT, " ZWEEPILG ")
// #pragma option_override(ZCNACT, "opt(level,0)")
int ZCNACT(ZCN *zcn)
{
  int rc = 0;
  rc = test_auth();
  if (0 != rc)
  {
    strcpy(zcn->diag.service_name, "TESTAUTH");
    zcn->diag.e_msg_len = sprintf(zcn->diag.e_msg, "Not authorized - %d", rc);
    zcn->diag.detail_rc = ZCN_RTNCD_NOT_AUTH;
    return RTNCD_FAILURE;
  }

  ZRCVY_ENV zenv = {0};

  if (0 == enable_recovery(&zenv))
  {
    ZCN zcn31 = {0};
    memcpy(&zcn31, zcn, sizeof(ZCN));
    rc = zcnm1act(&zcn31);
    memcpy(zcn, &zcn31, sizeof(ZCN));
    if (0 != rc)
    {
      zcn->diag.e_msg_len = sprintf(zcn->diag.e_msg, "Error activating console, service: %s, rc: %d, service_rc: %d, service_rsn: %d", zcn->diag.service_name, rc, zcn->diag.service_rc, zcn->diag.service_rsn);
      rc = RTNCD_FAILURE;
    }
  }
  else
  {
    zcn->diag.e_msg_len = sprintf(zcn->diag.e_msg, "Unexpected abend occurred during activation");
    rc = RTNCD_FAILURE;
  }

  disable_recovery(&zenv);

  return rc;
}

#pragma prolog(ZCNPUT, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(ZCNPUT, " ZWEEPILG ")
int ZCNPUT(ZCN *zcn, const char *command)
{
  int rc = 0;

  rc = test_auth();
  if (0 != rc)
  {
    strcpy(zcn->diag.service_name, "TESTAUTH");
    zcn->diag.e_msg_len = sprintf(zcn->diag.e_msg, "Not authorized - %d", rc);
    zcn->diag.detail_rc = ZCN_RTNCD_NOT_AUTH;
    return RTNCD_FAILURE;
  }

  ZCN zcn31 = {0};
  memcpy(&zcn31, zcn, sizeof(ZCN));
  rc = zcnm1put(&zcn31, command);
  memcpy(zcn, &zcn31, sizeof(ZCN));

  if (0 != rc)
  {
    zcn->diag.e_msg_len = sprintf(zcn->diag.e_msg, "Error writting data to console, service: %s, rc: %d, service_rc: %d, service_rsn: %d", zcn->diag.service_name, rc, zcn->diag.service_rc, zcn->diag.service_rsn);
    return RTNCD_FAILURE;
  }

  return rc;
}

#pragma prolog(ZCNTIMER, " ZWEPROLG NEWDSA=(YES,8),SAVE=BAKR,SAM64=YES")
#pragma epilog(ZCNTIMER, " ZWEEPILG ")
static void ZCNTIMER(void *PTR32 parameter)
{
  ECB *e = (ECB *)parameter;
  ecb_post(e, ZCN_POST_TIMEOUT);
}

#pragma prolog(ZCNMABEX, " ZWEPROLG NEWDSA=(YES,8) ")
#pragma epilog(ZCNMABEX, " ZWEEPILG ")
static void ZCNMABEX(SDWA *sdwa, void *abexit_data)
{
  ECB *PTR32 e = (ECB * PTR32) abexit_data;
  if (e)
  {
    ecb_post(e, ZCN_POST_ABEND);
    cancel_timers();
  }
}

#pragma prolog(ZCNGET, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(ZCNGET, " ZWEEPILG ")
int ZCNGET(ZCN *zcn, char *response)
{
  int rc = 0;

  ZRCVY_ENV zenv = {0};
  zenv.abexit = ZCNMABEX;
  zenv.abexit_data = (void *PTR64)zcn->ecb;

  rc = test_auth();
  if (0 != rc)
  {
    strcpy(zcn->diag.service_name, "TESTAUTH");
    zcn->diag.e_msg_len = sprintf(zcn->diag.e_msg, "Not authorized - %d", rc);
    zcn->diag.detail_rc = ZCN_RTNCD_NOT_AUTH;
    return RTNCD_FAILURE;
  }

  if (zcn->ecb)
  {
    int timeout = zcn->timeout * 100; // convert to seconds
    timer(timeout, ZCNTIMER, (ECB * PTR32) zcn->ecb);
    ecb_wait((ECB * PTR32) zcn->ecb);
    cancel_timers();
  }

  ZCN zcn31 = {0};
  memcpy(&zcn31, zcn, sizeof(ZCN));
  rc = zcnm1get(&zcn31, response);
  memcpy(zcn, &zcn31, sizeof(ZCN));

  if (0 != rc)
  {
    zcn->diag.e_msg_len = sprintf(zcn->diag.e_msg, "Error getting data from console, service: %s, rc: %d, service_rc: %d, service_rsn: %d", zcn->diag.service_name, rc, zcn->diag.service_rc, zcn->diag.service_rsn);
    return RTNCD_FAILURE;
  }

  return RTNCD_SUCCESS;
}

#pragma prolog(ZCNDACT, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(ZCNDACT, " ZWEEPILG ")
int ZCNDACT(ZCN *zcn)
{
  int rc = 0;

  rc = test_auth();
  if (0 != rc)
  {
    strcpy(zcn->diag.service_name, "TESTAUTH");
    zcn->diag.e_msg_len = sprintf(zcn->diag.e_msg, "Not authorized - %d", rc);
    zcn->diag.detail_rc = ZCN_RTNCD_NOT_AUTH;
    return RTNCD_FAILURE;
  }

  ZCN zcn31 = {0};
  memcpy(&zcn31, zcn, sizeof(ZCN));
  rc = zcnm1dea(&zcn31);
  memcpy(zcn, &zcn31, sizeof(ZCN));

  if (0 != rc)
  {
    zcn->diag.e_msg_len = sprintf(zcn->diag.e_msg, "Error deactivating console, service: %s, rc: %d, service_rc: %d, service_rsn: %d", zcn->diag.service_name, rc, zcn->diag.service_rc, zcn->diag.service_rsn);
    return RTNCD_FAILURE;
  }

  return RTNCD_SUCCESS;
}
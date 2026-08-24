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

#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <unistd.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <algorithm>
#include "zcnm.h"
#include "zcn.hpp"
#include "zut.hpp"
#include "zcntype.h"

int zcn_build_default_console_name(std::string &name)
{
  name.clear();
  int rc = zut_get_current_user(name);
  if (0 != rc || name.empty())
  {
    name.clear();
    return RTNCD_FAILURE;
  }
  if (name.length() > 7)
  {
    name.erase(7);
  }

  static std::atomic<unsigned int> console_seq(0);
  name += std::to_string(((unsigned int)getpid() + console_seq++) % 10u);
  return RTNCD_SUCCESS;
}

int zcn_activate(ZCN *zcn, const std::string &console_name)
{
  int rc = 0;
  zcn->diag.detail_rc = 0;

  memcpy(zcn->eye, ZCN_EYE, sizeof(zcn->eye));
  memset(zcn->cart, 0x00, sizeof(zcn->cart));

  zut_uppercase_pad_truncate(zcn->console_name, console_name, sizeof(zcn->console_name));

  zcn->ecb = (unsigned int *)__malloc31(sizeof(unsigned int));
  if (nullptr == zcn->ecb)
  {
    return ZCN_RTNCD_SERVICE_FAILURE;
  }
  memset(zcn->ecb, 0x00, sizeof(unsigned int));

  rc = ZCNACT(zcn);

  if (0 != rc)
  {
    free(zcn->ecb);
    zcn->ecb = nullptr;
  }

  return rc;
}

int zcn_put(ZCN *zcn, const std::string &command)
{
  int rc = 0;
  zcn->diag.detail_rc = 0;

  // unique cart
  static unsigned int cart_seq = 0;
  std::array<char, sizeof(zcn->cart) + 1> cart_buf = {};
  const unsigned int cart_val = ((unsigned int)time(nullptr) ^ (++cart_seq << 8)) & 0xFFFFFFu;
  snprintf(cart_buf.data(), cart_buf.size(), "ZC%06X", cart_val);
  memcpy(zcn->cart, cart_buf.data(), sizeof(zcn->cart));

  char *command31 = (char *)__malloc31(command.length() + 1);
  if (command31 == nullptr)
  {
    ZDIAG_SET_MSG(&zcn->diag, "Failed to allocate 31-bit memory for command: %s", command.c_str());
    return RTNCD_FAILURE;
  }
  memset(command31, 0x00, command.length() + 1);
  strncpy(command31, command.c_str(), command.length());

  rc = ZCNPUT(zcn, command31);
  free(command31);

  return rc;
}

int zcn_get(ZCN *zcn, std::string &response)
{
  int rc = 0;
  zcn->diag.detail_rc = 0;

  if ('\0' == zcn->cart[0])
    memcpy(zcn->cart, "ZOWECART", sizeof(zcn->cart));

  // user caller buffer size if provided
  if (0 == zcn->buffer_size)
    zcn->buffer_size = ZCN_DEFAULT_BUFFER_SIZE;
  *zcn->ecb = 0; // reset ECB if follow up call

  if (zcn->timeout <= 0)
    zcn->timeout = ZCN_DEFAULT_TIMEOUT;
  if (zcn->timeout > ZCN_MAX_TIMEOUT)
    zcn->timeout = ZCN_MAX_TIMEOUT;

  char *resp31 = (char *)__malloc31(zcn->buffer_size);
  if (resp31 == nullptr)
  {
    ZDIAG_SET_MSG(&zcn->diag, "Failed to allocate 31-bit memory for response");
    return RTNCD_FAILURE;
  }
  memset(resp31, 0x00, zcn->buffer_size);

  rc = ZCNGET(zcn, resp31);

  if (0 == rc)
    response += std::string(resp31);
  free(resp31);

  return rc;
}

int zcn_deactivate(ZCN *zcn)
{
  zcn->diag.detail_rc = 0;
  if (zcn->ecb)
  {
    free(zcn->ecb);
    zcn->ecb = nullptr;
  }

  return ZCNDACT(zcn);
}

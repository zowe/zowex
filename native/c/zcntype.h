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

#ifndef ZCNTYPE_H
#define ZCNTYPE_H

#include <stdint.h>
#include "ztype.h"

// RTNCD_CODE_SUCCESS ztype.h         -1
#define ZCN_RTNCD_SERVICE_FAILURE -2
#define ZCN_RTNCD_NOT_AUTH -3
#define ZCN_RTNCD_INSUFFICIENT_BUFFER -4

#define ZCN_DEFAULT_BUFFER_SIZE 4096
#define ZCN_DEFAULT_TIMEOUT 5
#define ZCN_MAX_TIMEOUT 30

#define ZCN_POST_TIMEOUT 1
#define ZCN_POST_ABEND 2

ZNP_PACK_ON

#define ZCN_EYE "ZCN"

// NOTE(Kelosky): struct is padded to nearest double word boundary; ensure proper alignment for fields
typedef struct
{
  char eye[3];              // future use
  unsigned char version[1]; // future use
  int32_t len;              // future use

  unsigned int *PTR64 ecb; // save and set to NULL to prevent waiting, int used to avoid unnecessary includes

  int32_t buffer_size;
  int32_t buffer_size_needed; // total ammount of buffer size needed to satisfy request

  char console_name[8]; // console name used, upper cased/padded/truncated

  int16_t unused;       // non-zero if reply found in control
  int16_t reply_id_len; // non-zero if reply found in control
  int timeout;          // timeout in seconds

  char reply_id[8]; // if reply_id_len is non-zero

  char cart[8]; // command and response token (CART) correlating a command to its responses; set per request by zcn_put

  int32_t id;
  uint32_t alet;

  void *PTR64 area;

  ZDIAG diag;

} ZCN;

ZNP_PACK_OFF

#endif

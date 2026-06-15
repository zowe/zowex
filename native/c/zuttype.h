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

#ifndef ZUTTYPE_H
#define ZUTTYPE_H

#include <stdint.h>
#include "ztype.h"

#define ZUT_RTNCD_SEARCH_SUCCESS 1
#define ZUT_RTNCD_SEARCH_WARNING 6

#define ZUT_RTNCD_SERVICE_FAILURE -2
#define ZUT_RTNCD_LOAD_FAILURE -3
#define ZUT_CALLER_ERROR -4

ZNP_PACK_ON

// NOTE(Kelosky): struct is padded to nearest double word boundary; ensure proper alignment for fields
typedef struct
{
  char eye[3];              // future use
  unsigned char version[1]; // future use
  int32_t len;              // future use

  ZDIAG diag;

} ZUT;

ZNP_PACK_OFF

#endif

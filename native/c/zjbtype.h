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

#ifndef ZJBTYPE_H
#define ZJBTYPE_H

#include <stdint.h>
#include "ztype.h"

// RTNCD_CODE_SUCCESS ztype.h         -1
#define ZJB_RTNCD_SERVICE_FAILURE -2
#define ZJB_RTNCD_SUBMIT_ERROR -3
#define ZJB_RTNCD_INSUFFICIENT_BUFFER -4
#define ZJB_RTNCD_JOB_NOT_FOUND -5
#define ZJB_RTNCD_UNEXPECTED_ERROR -6
#define ZJB_RTNCD_JES3_NOT_SUPPORTED -7
#define ZJB_RTNCD_VERBOSE_INFO_NOT_FOUND -8
#define ZJB_RTNCD_JOB_DSN_KEY_NOT_FOUND -9
#define ZJB_RTNCD_CORRELATOR_NOT_FOUND -10
#define ZJB_RTNCD_PARSE_ERROR_SIZE -11
#define ZJB_RTNCD_PARSE_ERROR_MALFORMED -12
#define ZJB_RTNCD_PARSE_ERROR_NUMBER_LEN -13
#define ZJB_RTNCD_PARSE_ERROR_NUMBER_CHAR -14
#define ZJB_RTNCD_PARSE_ERROR_NUMBER_CONVERSION -15
#define ZJB_RTNCD_JOB_DSN_NOT_FOUND -16

#define ZJB_RSNCD_MAX_JOBS_REACHED -1
#define ZJB_RSNCD_JOBID_NOT_FOUND -2
#define ZJB_RSNCD_CORRELATOR_NOT_FOUND -3

#define ZJB_DEFAULT_BUFFER_SIZE 128000
#define ZJB_DEFAULT_MAX_JOBS 1000
#define ZJB_DEFAULT_MAX_DDS 100

#define ZJB_UNKNOWN_RC ""

ZNP_PACK_ON

// NOTE(Kelosky): struct is padded to nearest double word boundary; ensure proper alignment for fields
typedef struct
{
  char eye[3];              // future use
  unsigned char version[1]; // future use
  int32_t len;              // future use

  unsigned char reserve_0[4];
  int32_t jobs_max;

  int32_t dds_max;
  int32_t buffer_size;

  int32_t buffer_size_needed; // total amount of buffer size needed to satisfy request
  unsigned char reserve_1[4];

  char correlator[64]; // job correlator
  char jobid[8];       // job id
  char owner_name[8];  // owner name used, upper cased/padded/truncated
  char prefix_name[8]; // prefix used, upper cased/padded/truncated
  char status_name[8]; // status used, upper cased/padded/truncated
  char token[80];

  ZEncode encoding_opts; // User-specified, desired encoding options for spool contents

  ZDIAG diag;

} ZJB;

ZNP_PACK_OFF

#endif

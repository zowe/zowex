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

#ifndef ZTYPE_H
#define ZTYPE_H

#include <stdint.h>

#define RTNCD_WARNING 1
#define RTNCD_SUCCESS 0
#define RTNCD_FAILURE -1

#ifndef ZMIN
#define ZMIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#if defined(__IBM_METAL__) || defined(__open_xl__)
#define PTR32 __ptr32
#else
#define PTR32
#endif

#if defined(__IBM_METAL__)
#define PTR64 __ptr64
#define ATTRIBUTE(...) __attribute__((__VA_ARGS__)) // ATTRIBUTE(amode31)
#define FAR __far
#define ASMREG(register) __asm(register)
#else
#define PTR64
#define ATTRIBUTE(...)
#define FAR
#define ASMREG(register)
// #define __malloc31(len) malloc(len)

#endif

#if defined(__MVS__)
#ifdef __open_xl__
#define ZNP_PACK_ON _Pragma("pack(1)")
#define ZNP_PACK_OFF _Pragma("pack()")
#else
#define ZNP_PACK_ON _Pragma("pack(packed)")
#define ZNP_PACK_OFF _Pragma("pack(reset)")
#endif
#else
#define ZNP_PACK_ON
#define ZNP_PACK_OFF
#endif

ZNP_PACK_ON

// NOTE(Kelosky): struct is padded to nearest double word boundary; ensure proper alignment for fields
typedef struct
{
  char eye[3];              // future use
  unsigned char version[1]; // future use
  int32_t len;              // future use

  char service_name[24]; // name of the service that was called

  int32_t detail_rc;  // maps to #defines in various headers
  int32_t service_rc; // return code from whatever service was called

  int32_t service_rsn;           // reason code from whatever service was called
  int32_t service_rsn_secondary; // secondary reason code from whatever service was called

  unsigned char reserve_1[4];
  int32_t e_msg_len;

  char e_msg[256];

  void *PTR64 data;

  unsigned int data_len;
  unsigned char reserve_2[4];

} ZDIAG;

ZNP_PACK_OFF

enum DataType
{
  eDataTypeText = 0,
  eDataTypeBinary = 1
};

typedef struct _ZEncode
{
  char codepage[16];
  char source_codepage[16];
  int64_t data_type;
} ZEncode;

#define FIFO_CHUNK_SIZE 32768

// Safe diagnostic message macro to prevent buffer overflow
#if defined(__IBM_METAL__) && !defined(__cplusplus)
#define ZDIAG_SET_MSG(diag, fmt, ...)                                 \
  {                                                                   \
    (diag)->e_msg_len = sprintf((diag)->e_msg, (fmt), ##__VA_ARGS__); \
  }
#else
#define ZDIAG_SET_MSG(diag, fmt, ...)                                                         \
  {                                                                                           \
    (diag)->e_msg_len = snprintf((diag)->e_msg, sizeof((diag)->e_msg), (fmt), ##__VA_ARGS__); \
    if ((diag)->e_msg_len >= sizeof((diag)->e_msg))                                           \
    {                                                                                         \
      (diag)->e_msg_len = sizeof((diag)->e_msg) - 1;                                          \
      (diag)->e_msg[sizeof((diag)->e_msg) - 1] = '\0';                                        \
    }                                                                                         \
  }
#endif

#endif

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

#ifndef ZSTORAGE_H
#define ZSTORAGE_H
/**
 * @file zstorage.h
 * @brief Storage management functions and macros for IBM Z Metal environment.
 *
 * This file contains storage management functions and macros for IBM Z Metal environment.
 */

#include <stdio.h>
#include "zmetal.h"

#if defined(__IBM_METAL__)
/**
 * @def STORAGE_OBTAIN(addr, size, loc)
 * @brief Macro to obtain storage in the IBM Z Metal environment.
 *
 * @param addr Pointer to the storage address.
 * @param size Size of the storage to obtain.
 * @param loc Location code for the storage (e.g., 24, 31).
 */
#define STORAGE_OBTAIN(addr, size, loc)                       \
  __asm(                                                      \
      "*                                                  \n" \
      " LLGF  0,%1      = storage length                  \n" \
      "*                                                  \n" \
      " STORAGE OBTAIN,"                                      \
      "LENGTH=(0),"                                           \
      "CALLRKY=YES,"                                          \
      "LOC=(" #loc ",64),"                                    \
      "COND=NO                                            \n" \
      "*                                                  \n" \
      " ST    1,%0      -> Save storage address           \n" \
      "*                                                    " \
      : "=m"(addr)                                            \
      : "m"(size)                                             \
      : "r0", "r1", "r14", "r15");
#else
#define STORAGE_OBTAIN(addr, size, loc)
#endif

#if defined(__IBM_METAL__)
/**
 * @def STORAGE_RELEASE(addr, size)
 * @brief Macro to release storage in the IBM Z Metal environment.
 *
 * @param addr Pointer to the storage address.
 * @param size Size of the storage to release.
 */
#define STORAGE_RELEASE(addr, size)                           \
  __asm(                                                      \
      "*                                                  \n" \
      " LLGF  0,%1      = storage length                  \n" \
      " LA    1,%0      -> storage address                \n" \
      "*                                                  \n" \
      " STORAGE RELEASE,"                                     \
      "LENGTH=(0),"                                           \
      "ADDR=(1),"                                             \
      "COND=NO,"                                              \
      "CALLRKY=YES                                        \n" \
      "*                                                    " \
      : "=m"(*(unsigned char *)addr)                          \
      : "m"(size)                                             \
      : "r0", "r1", "r14", "r15");
#else
#define STORAGE_RELEASE(addr, size)
#endif

#if defined(__IBM_METAL__)
/**
 * @def IARST64_GET(size, areaaddr)
 * @brief Macro to get a 64-bit storage area in the IBM Z Metal environment.
 *
 * @param size Size of the storage area to get.
 * @param areaaddr Pointer to the storage area address.
 */
#define IARST64_GET(size, areaaddr)                           \
  __asm(                                                      \
      "*                                                  \n" \
      " IARST64 REQUEST=GET,"                                 \
      "AREAADDR=%0,"                                          \
      "SIZE=%1,"                                              \
      "COMMON=NO,"                                            \
      "OWNINGTASK=CURRENT,"                                   \
      "FPROT=NO,"                                             \
      "TYPE=PAGEABLE,"                                        \
      "CALLERKEY=YES,"                                        \
      "FAILMODE=ABEND,"                                       \
      "REGS=SAVE                                          \n" \
      "*                                                    " \
      : "=m"(areaaddr)                                        \
      : "m"(size)                                             \
                                                              \
      : "r0", "r1", "r14", "r15");
#else
#define IARST64_GET(size, areaaddr)
#endif

#if defined(__IBM_METAL__)
/**
 * @def IARST64_FREE(areaaddr, temp)
 * @brief Macro to free a 64-bit storage area in the IBM Z Metal environment.
 *
 * @param areaaddr Pointer to the storage area address.
 * @param temp Temporary storage for the address.
 */
#define IARST64_FREE(areaaddr, temp)                          \
  __asm(                                                      \
      "*                                                  \n" \
      " LA 2,%1        -> Storage address                 \n" \
      "*                                                  \n" \
      " IARST64 REQUEST=FREE,"                                \
      "AREAADDR=(2),"                                         \
      "REGS=SAVE                                          \n" \
      "*                                                    " \
      : "=m"(*(unsigned char *)areaaddr)                      \
      : "m"(temp)                                             \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define IARST64_FREE(areaaddr, temp)
#endif

/**
 * @brief Obtain storage for 24-bit addressing.
 *
 * @param size Size of the storage to obtain.
 * @return Pointer to the obtained storage address.
 */
static void *PTR32 storage_obtain24(int size)
{
  void *PTR32 addr = NULL;
  STORAGE_OBTAIN(addr, size, 24);
  return addr;
}

/**
 * @brief Obtain storage for 31-bit addressing.
 *
 * @param size Size of the storage to obtain.
 * @return Pointer to the obtained storage address.
 */
static void *PTR32 storage_obtain31(int size)
{
  void *PTR32 addr = NULL;
  STORAGE_OBTAIN(addr, size, 31);
  return addr;
}

/**
 * @brief Release storage.
 *
 * @param size Size of the storage to release.
 * @param addr Pointer to the storage address.
 */
static void storage_release(int size, void *PTR32 addr)
{
  STORAGE_RELEASE(addr, size);
}

/**
 * @brief Get 64-bit storage area.
 *
 * @param size Size of the storage area to get.
 * @return Pointer to the obtained storage area address.
 */
static void *PTR64 storage_get64(int size)
{
  void *PTR64 storage = NULL;
  IARST64_GET(size, storage);
  return storage;
}

/**
 * @brief Free 64-bit storage area.
 *
 * @param storage Pointer to the storage area address.
 */
static void storage_free64(void *PTR64 storage)
{
  void *PTR64 temp = storage;
  IARST64_FREE(storage, temp);
}

#endif // ZSTORAGE_H

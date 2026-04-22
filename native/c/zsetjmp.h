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

#ifndef ZSETJMP_H
#define ZSETJMP_H
#include <stdio.h>
#include <string.h>
#include "zwto.h"
#include "zmetal.h"
#include "ihasaver.h"

typedef struct savf4sa SAVF4SA;

#if defined(__IBM_METAL__)
#define JUMP_ENV(f4sa, rc)                                        \
  __asm(                                                          \
      "*                                                      \n" \
      " LA   15,%0            -> F4SA                         \n" \
      " LMG  14,14,8(15)      Restore R14                     \n" \
      " LMG  0,12,24(15)      Restore R0-R12                  \n" \
      " LG   13,128(,15)      = prev R13                      \n" \
      " LG   13,136(,13)      = next R13                      \n" \
      " LGHI 15," #rc "       Set RC                          \n" \
      " BR   14               Branch and never return         \n" \
      "*                                                        " \
      : : "m"(f4sa) :);
#else
#define JUMP_ENV(f4sa, rc)
#endif

typedef struct
{
  SAVF4SA f4sa;

} ZSETJMP_ENV;

#pragma prolog(ZLONGJMP, " ZWEPROLG NEWDSA=NO ")
#pragma epilog(ZLONGJMP, " ZWEEPILG ")
static void ZLONGJMP(ZSETJMP_ENV *zenv)
{
  JUMP_ENV(zenv->f4sa, 4); // TODO(Kelosky): document non-zero return code
}

static void zlongjmp(ZSETJMP_ENV *zenv)
{
  ZLONGJMP(zenv);
}

// NOTE(Kelosky): this function may "return twice" like setjmp()
// NOTE(Kelosky): we must not have this function inline so to save and return to the mainline
#pragma reachable(zsetjmp)
static int zsetjmp(ZSETJMP_ENV *zenv) ATTRIBUTE(noinline);
static int zsetjmp(ZSETJMP_ENV *zenv)
{
  unsigned long long int r13 = 0;
  GET_STACK_ENV(r13); // NOTE(Kelosky): this is the same as get_prev_r13() but will be inlined
  unsigned char *save_area = (unsigned char *)r13;
  memcpy(&zenv->f4sa, save_area, sizeof(SAVF4SA));

  return 0;
}

#endif

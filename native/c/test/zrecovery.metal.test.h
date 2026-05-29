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

#ifndef ZRECOVERY_METAL_TEST_H
#define ZRECOVERY_METAL_TEST_H

#include "ztype.h"

#if defined(__cplusplus) && defined(__MVS__)
extern "OS"
{
#elif defined(__cplusplus)
extern "C"
{
#endif

  int ZRCVYEN(void);
  int ZRCVYDIS(void);
  int ZRCVYNST(int *PTR64 rc1, int *PTR64 rc2);
  int ZRCVYNCL(void);
  int ZRCVYTHR(int *PTR64 thread_id, int *PTR64 rc);
  int ZRCVYNUL(void);
  int ZRCVYSTK(void);

#if defined(__cplusplus)
}
#endif

#endif // ZRECOVERY_METAL_TEST_H

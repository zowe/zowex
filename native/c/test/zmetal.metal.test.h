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

#ifndef ZMETAL_METAL_TEST_H
#define ZMETAL_METAL_TEST_H

#if defined(__cplusplus) && defined(__MVS__)
extern "OS"
{
#elif defined(__cplusplus)
extern "C"
{
#endif

  void *ZMTLLOAD(const char *);
  int ZMTLDEL(const char *);
  int ZMTLJAOF();
  int ZMTLPST();
  int ZMTLTAUT();

#if defined(__cplusplus)
}
#endif

#endif // ZRECOVERY_METAL_TEST_H

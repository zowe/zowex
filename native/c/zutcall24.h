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

#ifndef ZUTCALL24_H
#define ZUTCALL24_H

// ZUTCAL24 should be copied into 24-bit storage.
int ZUTCAL24();
// ZUTCALQ returns ZUTCAL24's length in bytes (for copy to memory area)
int ZUTCALQ();

#endif
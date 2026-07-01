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

#ifndef ZUTM31_H
#define ZUTM31_H

#include "ztype.h"
#include "zprmtype.h"
#include "csvapfaa.h"
#include "csvdynlst.h"

int zutm1lpl(ZDIAG *, int *, PARMLIB_DSNS *) ATTRIBUTE(amode31);
int zutm1gur(char[8]) ATTRIBUTE(amode31);
int zutm1apf(struct apfhdr *, int *, int *) ATTRIBUTE(amode31);
int zutm1dyn(struct csvdynlst *, int *, int *) ATTRIBUTE(amode31);

// Invoke a dynamically loaded module through a below-the-line shim, honoring the
// module's AMODE (24 or 31) so it returns correctly to this above-the-line caller.
//   ep   - entry point from load_module (low 32 bits, AMODE bit preserved)
//   parm - caller-built, VL-terminated R1 parameter list, below the 16 MB line
int zutm1call24(unsigned int ep, void *PTR32 parm) ATTRIBUTE(amode31);

#endif

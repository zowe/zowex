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

int zutm1lpl(ZDIAG *, int *, PARMLIB_DSNS *) ATTRIBUTE(amode31);
int zutm1gur(char[8]) ATTRIBUTE(amode31);
int zutm1apf(struct apfhdr *, int *, int *) ATTRIBUTE(amode31);

// Invoke a DFSMSdfp utility program (IEBCOPY/IEBGENER), passing the utility's
// two-entry parameter list (option-list address + ddname-list address, VL-terminated).
// This function honors AMODE 24 and return correctly to this above-the-line caller.
// The parameter list itself is specific to the DFSMSdfp utilities.
int zutm1sdfp(unsigned int ep, void *PTR32 opt_list, void *PTR32 ddname_list) ATTRIBUTE(amode31);

#endif

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

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <algorithm>
#include <iconv.h>
#include <cstdlib>
#include <string>
#include <sstream>
#include "ztype.h"
#include "zut.hpp"

// NOTE(Kelosky): alternatives we'll likely use / consider in the future
// - CEA, probably needed to achieve z/OSMF parity (allows starting, stopping TSO address spaces)
// - IKJEFT01, requires authorized caller
// - IKJEFTSR, limited TSO dynamic environment
// - Load TMP directly, untested, but potentially useful if we read/write SYSTSIN/SYSTSPRT
int ztso_issue(const std::string &command, std::string &response)
{
  int rc = zut_run_program("tsocmd", {command}, response);
  // discard first line - tsocmd prints the args to stderr
  int split_pos = response.find_first_of('\n');
  if (split_pos != std::string::npos)
  {
    response.erase(0, split_pos + 1); // erase up to and including the newline
  }
  return rc;
}

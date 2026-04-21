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

#include "zmetal.h"
#include "ztime.h"
#include "zwto.h"
#include "zdbg.h"

#pragma prolog(main, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(main, " ZWEEPILG ")

int main()
{
  unsigned long long tod = 0;

  TIME_STRUCT time_struct = {0};
  int rc = 0;

  // get stck
  zut_dump_storage("tod before STCK", &tod, sizeof(tod));
  __asm(" STCK %0" : "=m"(tod));
  zut_dump_storage("tod after STCK", &tod, sizeof(tod));

  // convert to YYYYDDD
  rc = stckconv(&tod, &time_struct);
  zwto_debug("stckconv returned %d", rc);
  zut_dump_storage("time_struct", &time_struct, sizeof(time_struct));

  // convert back to stck
  rc = convtod(&time_struct, &tod);
  zwto_debug("convdod returned %d", rc);
  zut_dump_storage("tod after CONVTOD", &tod, sizeof(tod));

  // demo_setjmp();
  return 0;
}
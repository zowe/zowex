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

#include "zwto.h"
#include "zmetal.h"

// #define FAIL 1 //  enable at compile time to enable failure HLASM cases.

#pragma prolog(sub5, " ZWEPROLG NEWDSA=(YES,128),SAVE=BAKR,BSM=YES,SAM64=YES ")
#pragma epilog(sub5, " ZWEEPILG ")
int sub5()
{
  zwto_debug("sub5 called");
  return 0;
}

#pragma prolog(sub6, " ZWEPROLG NEWDSA=(YES,128),SAVE=BAKR,BSM=NO,SAM64=NO ")
#pragma epilog(sub6, " ZWEEPILG ")
int sub6()
{
  zwto_debug("sub6 called");
  return 0;
}

#pragma prolog(sub, " ZWEPROLG NEWDSA=(YES,128),SAVE=BAKR,BSM=NO ")
#pragma epilog(sub, " ZWEEPILG ")
int sub()
{
  zwto_debug("sub called");
  return 0;
}

#ifdef FAIL

// .ERR100 - BSM=YES may only be used with SAVE=BAKR
#pragma prolog(sub1, " ZWEPROLG NEWDSA=(YES,128),SAVE=NO,BSM=YES ")
#pragma epilog(sub1, " ZWEEPILG ")
int sub1()
{
  zwto_debug("sub1 called");
  return 0;
}

// .ERR040 - SAM64=YES may only be used with SAVE=BAKR
#pragma prolog(sub2, " ZWEPROLG NEWDSA=(YES,128),SAVE=NO,SAM64=YES ")
#pragma epilog(sub2, " ZWEEPILG ")
int sub2()
{
  zwto_debug("sub2 called");
  return 0;
}

// .ERR040 - SAM64=YES may only be used with SAVE=BAKR
#pragma prolog(sub3, " ZWEPROLG NEWDSA=(YES,128),SAVE=SA,SAM64=YES ")
#pragma epilog(sub3, " ZWEEPILG ")
int sub3()
{
  zwto_debug("sub3 called");
  return 0;
}

// .ERR100 - BSM=YES may only be used with SAVE=BAKR
#pragma prolog(sub4, " ZWEPROLG NEWDSA=(YES,128),SAVE=SA,BSM=YES ")
#pragma epilog(sub4, " ZWEEPILG ")
int sub4()
{
  zwto_debug("sub4 called");
  return 0;
}

// .ERR080 - USENAB=YES may not be used with SAVE=NO
#pragma prolog(sub7, " ZWEPROLG SAM64=NO,USENAB=YES,SAVE=NO ")
#pragma epilog(sub7, " ZWEEPILG ")
int sub7()
{
  zwto_debug("sub7 called");
  return 0;
}

// .ERR010 - NEWDSA= must be YES|NO
#pragma prolog(sub8, " ZWEPROLG NEWDSA=(FAIL,128) ")
#pragma epilog(sub8, " ZWEEPILG ")
int sub8()
{
  zwto_debug("sub8 called");
  return 0;
}

// .ERR020 - SAVE= must be SA|BAKR|NO
#pragma prolog(sub9, " ZWEPROLG NEWDSA=(YES,128),SAVE=FAIL ")
#pragma epilog(sub9, " ZWEEPILG ")
int sub9()
{
  zwto_debug("sub9 called");
  return 0;
}

// .ERR030 - SAM64= must be NO|YES
#pragma prolog(sub10, " ZWEPROLG NEWDSA=(YES,128),SAM64=FAIL ")
#pragma epilog(sub10, " ZWEEPILG ")
int sub10()
{
  zwto_debug("sub10 called");
  return 0;
}

// .ERR050 - LOC24= must be NO|YES
#pragma prolog(sub11, " ZWEPROLG NEWDSA=(YES,128),LOC24=FAIL ")
#pragma epilog(sub11, " ZWEEPILG ")
int sub11()
{
  zwto_debug("sub11 called");
  return 0;
}

// .ERR060 - NEWDSA=(YES...) may not be used with USENAB=YES
#pragma prolog(sub12, " ZWEPROLG NEWDSA=(YES,128),USENAB=YES ")
#pragma epilog(sub12, " ZWEEPILG ")
int sub12()
{
  zwto_debug("sub12 called");
  return 0;
}

// .ERR070 - USENAB= must be NO|YES
#pragma prolog(sub13, " ZWEPROLG NEWDSA=(YES,128),USENAB=FAIL ")
#pragma epilog(sub13, " ZWEEPILG ")
int sub13()
{
  zwto_debug("sub13 called");
  return 0;
}

// .ERR090 - BSM= must be YES|NO
#pragma prolog(sub14, " ZWEPROLG NEWDSA=(YES,128),BSM=FAIL ")
#pragma epilog(sub14, " ZWEEPILG ")
int sub14()
{
  zwto_debug("sub14 called");
  return 0;
}

// .ERR120 - NEWDSA=YES may not be used with SAVE=NO
#pragma prolog(sub15, " ZWEPROLG NEWDSA=(YES,128),SAVE=NO ")
#pragma epilog(sub15, " ZWEEPILG ")
int sub15()
{
  zwto_debug("sub15 called");
  return 0;
}

#endif

#pragma prolog(main, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(main, " ZWEEPILG ")
int main()
{

  PSW psw = {0};
  get_psw(&psw);
  if (psw.data.bits.ba)
    zwto_debug("main ba mode");
  if (psw.data.bits.ea)
    zwto_debug("main ea mode");

  int rc = sub();
  zwto_debug("main returned %d", rc);

  get_psw(&psw);
  if (psw.data.bits.ba)
    zwto_debug("main ba mode");
  if (psw.data.bits.ea)
    zwto_debug("main ea mode");
  // int mode_switch = psw.p ? 0 : 1;

  // ZSETJMP_ENV zenv = {0};

  // int rc = zsetjmp(&zenv);
  // if (rc == 0)
  // {
  //   zwto_debug("zsetjmp returned 0");
  //   test(&zenv);
  //   // ZLONGJMP(&zenv);
  // }
  // else
  // {
  //   zwto_debug("zsetjmp returned %d", rc);
  // }

  return 0;
}

*  This program and the accompanying materials are
*  made available under the terms of the Eclipse Public License v2.0
*  which accompanies this distribution, and is available at
*  https://www.eclipse.org/legal/epl-v20.html
*
*  SPDX-License-Identifier: EPL-2.0
*
*  Copyright Contributors to the Zowe Project.
ZAM24    RSECT ,
ZAM24    AMODE 31
ZAM24    RMODE 31
*
         YREGS ,
*
*        On entry: R1=DCB abend parameter list, R14=return address
*        Call ZAMDEXIT with R1 preserved, then return
*
ENTRY24  DS    0H
         USING IO_CTRL,R8
*
         ST    R1,WORK             Save PLIST in IO_CTRL "work" field
         LARL  R15,ZAMDEXT@        ->-> ZAMDEXIT@
         L     R15,0(,R15)         -> ZAMDEXIT
         OILH  R15,X'8000'         Set AMODE 31 bit for BASSM
         LA    R1,WORK              -> IO_CTRL "work" field
         BASSM 0,R15               Call ZAMDEXIT in AMODE 31
*
CONSTANT DS    0D
         LTORG ,
*
ZAMDEXT@ DC    V(ZAMDEXIT)
*
*        Separate ENTRY to obtain length of this module
*
         ENTRY ZAM24Q
ZAM24Q   DS    0H
         LHI   R15,ZQM24LEN         = length of this module
         BR    R14                  return to caller
*
ZQM24LEN EQU *-ZAM24                Dynamically obtain length of module
*
*        IO_CTRL structure (see zamtypes.h)
*
IO_CTRL  DSECT ,
EYE      DS    CL8
WORK     DS    D                    Work area
         ORG   WORK                 Org back to work
PLIST    DS    A                    PLIST address
         ORG   ,                    Reset location counter
*
         END   ,

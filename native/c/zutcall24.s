* This program and the accompanying materials are
* made available under the terms of the Eclipse Public License v2.0
* which accompanies this distribution, and is available at
* https://www.eclipse.org/legal/epl-v20.html
*
* SPDX-License-Identifier: EPL-2.0
*
* Copyright Contributors to the Zowe Project.
ZUTCAL24  RSECT ,
ZUTCAL24  AMODE 31
ZUTCAL24  RMODE 31
*
           YREGS ,
*
* Common below-the-line shim for invoking a dynamically loaded
* module. It honors the module's own AMODE and returns correctly to
* an above-the-line (RMODE 31) caller. Driven by zutm1call24 for any
* loaded module -- e.g. ZUTMSDFP (IEBCOPY / IEBGENER) and ZUTRUN
* (arbitrary programs); the parameter-list shape is the caller's.
*
* This routine is position independent. zutm1call24 copies it below
* the 16 MB line and runs the copy, so the return address handed to
* an AMODE 24 module is a 24-bit address: when the module returns
* with a plain AMODE 24 BR 14 the branch still lands here. BASSM
* enters the module in its own AMODE (24 or 31) from the AMODE bit
* that LOAD leaves in the entry point.
*
* On entry (AMODE 31, standard OS linkage):
*  R1  -> 2-word parameter list:
*            +0  module entry point (bit 32 = its AMODE)
*            +4  module parameter list (caller-built, VL-terminated)
*  R13 -> caller save area, R14 -> return, R15 -> entry point
* On exit:
*    R15 =  the module's return code
*
         USING SAVER,R13             Map caller save area
         STM   R14,R12,SAVGRS14      Save caller registers
         L     R15,0(,R1)            R15 -> module EP (AMODE bit kept)
         L     R2,SAVNEXT            R2  -> module save area (NAB)
         L     R1,4(,R1)             R1  -> module parameter list
OUR      USING SAVER,R2              Addressability to our save area
         ST R13,OUR.SAVPREV          Our back chain -> caller save area
         DROP OUR
         LR R13,R2                   R13 -> our save area
         LARL  R14,ZUZEROS           -> zeros (PC-relative; copy safe)
         LMH   R0,R15,0(R14)         Clear GPR high halves
         BASSM R14,R15               Enter module in its AMODE 
         SAM31 ,                     May have returned in AMODE 24
         L     R13,SAVPREV           R13 -> restore caller save area
         L     R14,SAVGRS14          Restore caller R14
         LM    R2,R12,SAVGRS2        Restore caller R2-R12
         BR    R14                   Return to above-the-line caller
*
* Separate ENTRY to obtain the length of this routine for the copy.
*
         ENTRY ZUTCALQ
ZUTCALQ  DS    0H
         LHI   R15,ZUCALLEN          = length of this module
         BR    R14                   Return to caller
*
         DS    0F
ZUZEROS  DC    16F'0'                Zeros for the LMH above
*
ZUCALLEN EQU   *-ZUTCAL24           Length of this module

         IHASAVER ,                  Generate DSECT mappings for SAVER
         END   ,
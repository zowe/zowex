*  This program and the accompanying materials are
*  made available under the terms of the Eclipse Public License v2.0
*  which accompanies this distribution, and is available at
*  https://www.eclipse.org/legal/epl-v20.html
*
*  SPDX-License-Identifier: EPL-2.0
*
*  Copyright Contributors to the Zowe Project.
ZUTUCALL RSECT ,
ZUTUCALL AMODE 31
ZUTUCALL RMODE 31
*
         YREGS ,
*
*  Common below-the-line trampoline for invoking a dynamically loaded
*  module. It honors the module's own AMODE and returns correctly to
*  an above-the-line (RMODE 31) caller. Driven by zutm1call for any
*  loaded module -- e.g. ZUTMSDFP (IEBCOPY / IEBGENER) and ZUTRUN
*  (arbitrary programs); the parameter-list shape is the caller's.
*
*  This routine is position independent. zutm1call copies it below
*  the 16 MB line and runs the copy, so the return address handed to
*  an AMODE 24 module is a 24-bit address: when the module returns
*  with a plain AMODE 24 BR 14 the branch still lands here. BASSM
*  enters the module in its own AMODE (24 or 31) from the AMODE bit
*  that LOAD leaves in the entry point.
*
*  On entry (AMODE 31, standard OS linkage):
*    R1  -> 3-word parameter list:
*             +0  module entry point (bit 32 = its AMODE)
*             +4  module parameter list (caller-built, VL-terminated)
*             +8  18-word save area, below the 16 MB line
*    R13 -> caller save area, R14 -> return, R15 -> entry point
*  On exit:
*    R15 =  the module's return code
*
         STM   R14,R12,12(R13)     Save caller registers
         L     R15,0(,R1)          R15 -> module EP (AMODE bit kept)
         L     R2,8(,R1)           R2  -> module save area (<16M)
         L     R1,4(,R1)           R1  -> module parameter list
         ST    R13,4(,R2)          Chain caller SA behind module SA
         LR    R13,R2              R13 -> module save area
         LARL  R14,ZUZEROS         -> zeros (PC-relative; copy safe)
         LMH   R0,R15,0(R14)       Clear GPR high halves (bit 32 kept)
         BASSM R14,R15             Enter module in its AMODE
         SAM31 ,                   May have returned in AMODE 24
         L     R13,4(,R13)         R13 -> caller save area
         L     R14,12(,R13)        Restore caller R14
         LM    R2,R12,28(R13)      Restore R2-R12 (R15 = module RC)
         BSM   0,R14               Return, restoring caller AMODE
*
         DS    0F
ZUZEROS  DC    16F'0'              Zeros for the LMH above
*
*  Separate ENTRY to obtain the length of this routine for the copy.
*
         ENTRY ZUTUCALQ
ZUTUCALQ DS    0H
         LHI   R15,ZUCALLEN        = length of this module
         BR    R14                 Return to caller
*
ZUCALLEN EQU   *-ZUTUCALL          Length of this module
*
         END   ,

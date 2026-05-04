DRIVER  RSECT
DRIVER  AMODE 24
DRIVER  RMODE 24
        YREGS   ,
*
        SYSSTATE ARCHLVL=2
*
        SAVE  (14,12),,WORK
        LR    R3,R1                Save input parameters
*
        SAM24
*
        LARL  R12,CONSTANT
        USING CONSTANT,R12
*
        STORAGE OBTAIN,                                                +
               LENGTH=WRKLEN,                                          +
               LOC=(24,64),                                            +
               COND=NO
*
        LR    R10,R1               -> new storage
        USING WRK,R10
*
        LA    R0,WRK               -> new storage
        LHI   R1,WRKLEN
        SLR   R15,R15
        MVCL  R0,R14
*
        LA    R2,WRKSAVE
*
OUR     USING SAVER,R2
CALLER  USING SAVER,R13
*
        ST    R2,CALLER.SAVNEXT
        ST    R13,OUR.SAVPREV
*
        DROP  OUR,CALLER
        LR    R13,R2
        USING SAVER,R13
*
        JAS   R14,PRNTAMD
*
        L     R15,=V(SUB)
        OILH  R15,X'8000'          Set AMODE 31 bit for BASSM
*        OILL  R15,X'0001'          Set AMODE 64 bit for BASSM
        BASSM R14,R15
*
        JAS   R14,PRNTAMD
*
        L     R13,SAVPREV-SAVER(,R13)
*
        LA    R1,WRK
        STORAGE RELEASE,                                               +
               LENGTH=WRKLEN,                                          +
               ADDR=(1)
*
        RETURN (14,12),RC=0
*
CONSTANT DS    0D
         LTORG ,
*
PRNTAMD DS    0H
        ST    R14,WRKR14
        TAM   ,
        BRC   8,AMODE24
        BRC   4,AMODE31
        BRC   1,AMODE64
        EXRL  0,*                  should never get here
*
AMODE24 DS    0H
        WTO   'AMODE24'
        J     CONTINU
*
AMODE31 DS    0H
        WTO   'AMODE31'
        J     CONTINU
*
AMODE64 DS    0H
        WTO   'AMODE64'
        J     CONTINU
*
CONTINU DS    0H
        L     R14,WRKR14
        BR    R14
*
*
WRK     DSECT ,
WRKSAVE DS    XL72
WRKR14  DS    F
WRKLEN  EQU   *-WRK
*
        IHASAVER ,
        END

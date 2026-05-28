#ifdef __open_xl__
#pragma pack(1)
#else
#pragma pack(packed)
#endif

#ifndef __iazjsab__
#define __iazjsab__

struct iazjsab {
  unsigned char  jsabid[4];    /* JSAB ID                              */
  void * __ptr32 jsabnext;     /* JSAB CHAIN FIELD                     */
  int            jsablen;      /* Length of control block     @410D001 */
  unsigned char  jsabvers;     /* CONTROL BLOCK VERSION                */
  unsigned char  jsabflg1;     /* JSAB FLAG 1                 @410D001 */
  unsigned char  jsabflg2;     /* JSAB FLAG 2                 @410D001 */
  struct {
    unsigned char  _jsabclev; /* CREATING COMPONENT'S CODE LEVEL */
    } jsabclr;
  unsigned char  jsabscid[4];  /* SCHEDULING COMPONENT'S ID            */
  struct {
    unsigned char  _jsabjbid[8]; /* JOB ID */
    } jsabwkid;
  unsigned char  jsabjbnm[8];  /* JOB NAME                             */
  unsigned char  jsabpref[8];  /* PREFIX USED IN MESSAGES              */
  unsigned char  jsabusid[8];  /* USERID                               */
  unsigned char  jsabssnm[4];  /* Creating Subsystem name     @Z23D116 */
  unsigned char  jsabresc[16]; /* Reserved for future use     @Z23D116 */
  double         jsabestk;     /* PROGRAM ENTRY START TIME             */
  double         jsabxstk;     /* PROGRAM EXECUTION START TIME         */
  void * __ptr32 jsabuser;     /* USER AREA POINTER                    */
  unsigned char  jsabgpnm[8];  /* XCF group name              @520LSDS */
  struct {
    unsigned char  _jsabjfl1;    /* JES Status flags            @Z21LSYM */
    unsigned char  _jsabjfl2;    /* More JES Status flags       @Z21LSYM */
    unsigned char  _jsabjfl3;    /* More JES Status flags       @Z21LSYM */
    unsigned char  _jsabjfl4;    /* More JES Status flags       @Z21LSYM */
    unsigned char  _jsabjfl5;    /* More JES Status flags       @Z22LEVT */
    unsigned char  _filler1[3];  /* Reserved for status flags   @Z22LEVT */
    } jsabjsta;
  int            jsabresv[5];  /* Reserved for future use     @OW17398 */
  unsigned char  jsabcorr[64]; /* Job correlator              @Z21LSYM */
  };

#define jsabclev jsabclr._jsabclev
#define jsabjbid jsabwkid._jsabjbid
#define jsabjfl1 jsabjsta._jsabjfl1
#define jsabjfl2 jsabjsta._jsabjfl2
#define jsabjfl3 jsabjsta._jsabjfl3
#define jsabjfl4 jsabjsta._jsabjfl4
#define jsabjfl5 jsabjsta._jsabjfl5

/* Values for field "jsabvers" */
#define jsabvrs1 1    /* JSAB version 1              @Z21LSYM */
#define jsabvrs2 2    /* JSAB version 2              @Z21LSYM */
#define jsabvrsn 2    /* Current JSAB version        @Z21LSYM */

/* Values for field "jsabflg1" */
#define jsabnval 0x80 /* This JSAB is not valid      @410D001 */
#define jsabstsk 0x40 /* Subtask level JSAB          @R05LOPI */

/* Values for field "jsabjfl1" */
#define jsabj1sp 0x80 /* JES supports JES status   @OW17398   */
#define jsabj1ps 0x40 /* Waiting for PSO           @OW17398   */
#define jsabj1cn 0x20 /* Waiting for CS (Cancel)   @OW17398   */
#define jsabj1st 0x10 /* Waiting for CS (Status)   @OW17398   */
#define jsabj1tr 0x08 /* Waiting for job term      @OW17398   */
#define jsabj1rq 0x04 /* Waiting for job reenqueue @OW17398   */
#define jsabj1iw 0x02 /* Initiator waiting for job @OW17398   */
#define jsabj1ss 0x01 /* Waiting for SPOOL space   @OW17398   */

/* Values for field "jsabjfl2" */
#define jsabj2cm 0x80 /* Waiting for JES Cross     @OW17398   */
#define jsabj2sa 0x40 /* Waiting for SAPI          @R10LMSD   */
#define jsabj2nu 0x20 /* Waiting for notify user   @Z11LSSI   */
#define jsabj2es 0x10 /* Waiting for extended      @Z11LSSI   */
#define jsabj2pc 0x08 /* Waiting for JES class     @Z11LSSI   */
#define jsabj2pn 0x04 /* Waiting for JES nodes     @Z11LSSI   */
#define jsabj2ps 0x02 /* Waiting for JES spool     @Z11LSSI   */
#define jsabj2pi 0x01 /* Waiting for JES inits     @Z11LSSI   */

/* Values for field "jsabjfl3" */
#define jsabj3px 0x80 /* Waiting for JES JESPLEX   @Z11LSSI   */
#define jsabj3wo 0x40 /* Waiting for WTO           @Z11LSSI   */
#define jsabj3er 0x20 /* Waiting for ENDREQ        @Z11LSSI   */
#define jsabj3jd 0x10 /* Waiting for JDS access    @Z11LSSI   */
#define jsabj3da 0x08 /* Waiting for dynamic       @Z11LSSI   */
#define jsabj3tc 0x04 /* Waiting for TCPIP NJE     @Z11LSSI   */
#define jsabj3fs 0x02 /* Waiting for FSS request - @Z11LSSI   */
#define jsabj3ci 0x01 /* Waiting for CI driver -   @Z11LSSI   */

/* Values for field "jsabjfl4" */
#define jsabj4st 0x80 /* Waiting for SETUP         @Z11LSSI   */
#define jsabj4vl 0x40 /* Waiting for validate      @Z11LSSI   */
#define jsabj4sj 0x20 /* Waiting for SJF services  @Z11LSSI   */
#define jsabj4dy 0x10 /* Waiting for dynamic       @Z11LSSI   */
#define jsabj4dc 0x08 /* Waiting for dynamic       @Z11LSSI   */
#define jsabj4nq 0x04 /* Waiting for change ENQ    @Z11LSSI   */
#define jsabj4dd 0x02 /* Waiting for change DD     @Z11LSSI   */
#define jsabj4jd 0x01 /* Waiting for JES Device    @Z12LSSI   */

/* Values for field "jsabjfl5" */
#define jsabj5jm 0x80 /* Waiting for Job           @Z22LEVT   */

#endif

#ifdef __open_xl__
#pragma pack()
#else
#pragma pack(reset)
#endif

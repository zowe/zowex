#ifdef __open_xl__
#pragma pack(1)
#else
#pragma pack(packed)
#endif

#ifndef __assb__
#define __assb__

struct assb {
  struct {
    unsigned char  _assbassb[4]; /* ACRONYM IN EBCDIC - ASSB.            */
    int            _assbsmfl;    /* Lock word for BMFLSD lock for TCTIOT */
    } assbegin;
  void * __ptr32 assbubav;                                 /* @0TC                                 */
  void * __ptr32 assbubad;                                 /* @0TC                                 */
  void * __ptr32 assbupav;                                 /* @0TC                                 */
  unsigned char  assbr014[4];                              /* @0TC                                 */
  unsigned char  assbxmf1;                                 /* CROSS MEMORY FLAGS 1            @L4A */
  unsigned char  assbxmf2;                                 /* CROSS MEMORY FLAGS 2            @L4A */
  short int      assbxmcc;                                 /* CROSS MEMORY CONNECTIONS COUNT. @L1A */
  void * __ptr32 assbcbtp;                                 /* POINTER TO IHAACBT              @LEC */
  int            assbvsc;                                  /* VIO SLOT ALLOCATED COUNT.       @D3A */
  int            assbnvsc;                                 /* NON-VIO SLOT ALLOCATED COUNT.   @D3A */
  int            assbasrr;                                 /* ADDRESS SPACE RE-READS TO BE    @D4C */
  void * __ptr32 assbdexp;                                 /* POINTER TO IHADEXP              @LDC */
  struct {
    int            _assbstw1; /* FIRST WORD OF ASSBSTKN.         @L2C */
    int            _assbisqn; /* Initial address space sequence  @L2C */
    } assbstkn;
  void * __ptr32 assbbpsa;                                 /* IBMPM ANCHOR BLOCK.             @02C */
  int            assbcsct;                                 /* CACHING FACILITY STOP COUNT     @LGA */
  void * __ptr32 assbbalv;                                 /* VIRTUAL ADDRESS OF THE BASIC    @L2C */
  void * __ptr32 assbbald;                                 /* BASIC ACCESS LIST DESIGNATOR.   @L2C */
  void * __ptr32 assbxmse;                                 /* ADDRESS OF XMSE FOR THIS        @L2A */
  int            assbtsqn;                                 /* NEXT TTOKEN SEQUENCE NUMBER.    @L3A */
  int            assbvcnt;                                 /* COUNT OF CURRENT TASKS WITH     @D5A */
  void * __ptr32 assbpalv;                                 /* PASN ACCESS LIST VIRTUAL        @D6C */
  void * __ptr32 assbasei;                                 /* ADDRESS OF ADDRESS SPACE        @D6C */
  void * __ptr32 assbrma;                                  /* ADDRESS OF ADDRESS SPACE        @D6C */
  double         assbhst;                                  /* CPU time for Hiperspace              */
  double         assbiipt;                                 /* CPU time for I/O interrupt           */
  int            assbanec;                                 /* ALESERV ADD WITH NO EAX COUNT.  @L6A */
  void * __ptr32 assbsdov;                                 /* ADDRESS OF SHARED DATA OBJECT   @LDC */
  int            assbmcso;                                 /* NUMBER OF CONSOLE IDS ACTIVATED @LAA */
  void * __ptr32 assbdfas;                                 /* ADDRESS OF DFP=SMSX STRUCTURE   @L7A */
  struct {
    unsigned char  _assbflg0; /* ASSB FLAG BYTE 0.               @D7C */
    unsigned char  _assbflg1; /* ASSB FLAG BYTE 1                @LIC */
    unsigned char  _assbflg2; /* ASSB FLAG BYTE 2.               @D7C */
    unsigned char  _assbflg3; /* ASSB FLAG BYTE 3                @D7C */
    } assbflgs;
  void * __ptr32 assbascb;                                 /* ADDRESS OF ASCB.                @L9C */
  void * __ptr32 assbasrf;                                 /* CREATED ASSB FORWARD POINTER.   @L9A */
  void * __ptr32 assbasrb;                                 /* CREATED ASSB BACKWARD POINTER.  @L9A */
  void * __ptr32 assbssd;                                  /* ADDRESS OF THE SUSPENDED SRB    @D8A */
  void * __ptr32 assbmqma;                                 /* CONTROL BLOCK ANCHOR FOR        @06C */
  double         assblasb;                                 /* TOKEN INDICATING IF MVS/APPC    @LBC */
  void * __ptr32 assbsch;                                  /* POINTER TO APPC SCHEDULER       @LBA */
  int            assbfsc;                                  /* COUNT ACCUMULATED BY IEAMFCNT   @01A */
  void * __ptr32 assbjsab;                                 /* ADDRESS OF JOB SCHEDULER        @D9A */
  void * __ptr32 assbrctw;                                 /* ADDRESS OF RCT's WEB.           @LSA */
  struct {
    unsigned char  _assbldxh[4]; /* High half                       @0PA */
    void * __ptr32 _assbldxl;    /* Low half of address             @0PA */
    } assbldax;
  void * __ptr32 assbtlmi;                                 /* ADDRESS OF TAILORED LOCK        @LOA */
  void * __ptr32 assbsdas;                                 /* POINTER TO WORKING STORAGE      @DBA */
  int            assbtpin;                                 /* THE COUNT OF UCB PIN REQUESTS   @LFA */
  int            assbspin;                                 /* THE COUNT OF UCB PIN REQUESTS   @LFA */
  int            assbect1;                                 /* THE COUNT OF ALLOCATION         @H2A */
  int            assbect2;                                 /* THE COUNT OF ALLOCATION         @H2A */
  int            assbmt_n_;                                /* MEMTERM DISABLE COUNT.  WHEN    @DAA */
  unsigned char  assbdfp[4];                               /* RESERVED FOR USE BY DFP.        @03A */
  short int      assbsasi;                                 /* SASI info                       @NUC */
  short int      assbsnew;                                 /* Count of SASN=NEW connections        */
  void * __ptr32 assbnttp;                                 /* ADDRESS OF ADDRESS SPACE LEVEL  @LIA */
  struct {
    unsigned char  _assboepc; /* POST CODE: '81'X=>BPX1EXC       @LMA */
    struct {
      unsigned char  _filler1[2];  /* Unused.                         @LMA */
      unsigned char  _assboees;    /* Exit Status passed on BPX1EXI.  @LMA */
      } assboecd;
    } assboecb;
  void * __ptr32 assboasb;                                 /* OpenMVS ADDRESS SPACE BLOCK.    @LLA */
  void * __ptr32 assbxsba;                                 /* XSB POOL QUEUE.                 @LKA */
  void * __ptr32 assbdlcb;                                 /* Contains the address of the Dynamic  */
  void * __ptr32 assbvab;                                  /* ADDRESS OF VSM ADDRESS SPACE    @LNC */
  void * __ptr32 assblmab;                                 /* LATCH MANAGER ADDRESS SPACE     @LNC */
  int            assbioct;                                 /* DIV/IO count                    @0HC */
  void * __ptr32 assbctt;                                  /* CTT field                       @H3A */
  int            assbxrct;                                 /* XES REQUEST COUNT               @LQA */
  struct {
    int            _assb___nonenct___psrb___cp___disps; /* Count of non-enclave preemptable SRB */
    } assbetsc___prezos11;
  void * __ptr32 assbtasb;                                 /* TCPIP ASSB Extension            @07A */
  int            assbtpma;                                 /* OWNER:  IOS.                    @DCA */
  int            assbrosu;                                 /* OWNER:  IOS.                    @DCA */
  int            assbtpmt;                                 /* OWNER:  IOS.                    @DCA */
  unsigned char  assbssdt[4];                              /* SSD Trailer                     @LSC */
  void * __ptr32 assbtawq;                                 /* ADDRESS OF TASK WEB QUEUE.      @LSA */
  void * __ptr32 assbwcml;                                 /* ADDRESS OF CML PROMOTION WEB.   @LSA */
  void * __ptr32 assbws3s;                                 /* ADDRESS OF ASYNCHRONOUS EXIT    @LSA */
  void * __ptr32 assbwsss;                                 /* ADDRESS OF SUSPENDED STATUS     @LSA */
  void * __ptr32 assbcapq;                                 /* ADDRESS OF FIRST WEB ON THE     @LSC */
  void * __ptr32 assbptar;                                 /* Pointer used by RTM processing  @09C */
  int            assbwtct;                                 /* When this counter is non-zero,       */
  int            assbsbct;                                 /* XES-owned count of requests     @05A */
  void * __ptr32 assbarbp;                                 /* ARM (Automatic Restart Manager) @LVA */
  void * __ptr32 assbrctr;                                 /* ADDRESS OF RCT's RB                  */
  void * __ptr32 assbscah;                                 /* Address of the SCA (SPIE/ESPIE  @PAA */
  unsigned char  assbttfl;                                 /* Transaction Trace flags.        @MCA */
  unsigned char  assbwmf1;                                 /* WLM flags                       @M8A */
  short int      assbpswc;                                 /* Preemptable-class SRB short     @P5A */
  void * __ptr32 assbixga;                                 /* Pointer to SLC address space    @LXC */
  double         assbjbni;                                 /* Jobname for the Initiated       @P1A */
  double         assbjbns;                                 /* Jobname for the START/MOUNT/    @P1A */
  double         assbasst;                                 /* Additional SRB Service Time.    @LYA */
  double         assbphtm;                                 /* Preemptable-class Time. The     @0PC */
  void * __ptr32 assbcrwq;                                 /* Client Related WEB Queue.       @LYA */
  void * __ptr32 assbscwq;                                 /* Suspended Client related WEB    @LYA */
  int            assblcnt;                                 /* Number of latched operations    @LZA */
  int            assbacnt;                                 /* Number of asynchronous requests @M1C */
  int            assblcpd;                                 /* CPOOLID of the cpool created    @M1A */
  struct {
    short int      _assbslpc; /* Slip PER serialization count    @NxA */
    short int      _assbslnc; /* Slip Non-PER serialization count     */
    } assbslsc;
  void * __ptr32 assbpvtc;                                 /* Queue of privately managed      @M4A */
  struct {
    unsigned char  _assbctxf;    /* Context Services flags          @MAM */
    unsigned char  _assbctx2[3]; /* Reserved context services.      @MAA */
    } assbctx;
  unsigned char  assbhale[16];                             /* Copy of Home ALE                @P7A */
  struct {
    int            _assb___time___on___zaap___disps; /* Count of task dispatches for zAAP */
    } assb___time___on___zcbp___disps;
  int            assbsrsn;                                 /* Suspend/Resume sequence number       */
  void * __ptr32 assbwlms;                                 /* Address of WLM managed server   @M8A */
  void * __ptr32 assbbcba;                                 /* Address of SOMObjects data structure */
  unsigned char  assbcsm[4];                               /* CSM user bitmap                 @P9A */
  int            assbpect;                                 /* Number of Pause elements allocated   */
  void * __ptr32 assbrrsa;                                 /* RRS data area pointer           @0EA */
  struct {
    unsigned char  _assbofl0; /* ASSB USS flag byte 0            @DEA */
    unsigned char  _assbofl1; /* ASSB USS flag byte 1            @DEA */
    } assboflg;
  unsigned char  assbscaf[2];                              /* CPU affinity indicator          @MDA */
  int            assbctxc;                                 /* Number of private contexts owned by  */
  int            assbrmct;                                 /* Number of PKM 8 to 15 resource       */
  void * __ptr32 assblrba;                                 /* License manager resource block  @MFA */
  void * __ptr32 assbslfa;                                 /* Shadow LFT address              @MIA */
  unsigned char  assbr1d0;                                 /* Reserved                        @0NC */
  unsigned char  assbfabricpriority;                       /* Fabric I/O Priority. Will be zero if */
  unsigned char  assbioms[2];                              /* I/O Management Support Data.         */
  short int      assbpromotioncount;                       /* Number of WEBs to promote.           */
  char           assbtiop;                                 /* Tape I/O Priority               @MPA */
  char           assbcsdp;                                 /* Channel Subsystem I/O Priority. @MEA */
  struct {
    struct {
      struct {
        double         _assb___time___ifa___on___cp; /* @H4C */
        } assb___time___zcbp___on___cp;
      struct {
        struct {
          double         _assb___time___on___ifa; /* @H4C */
          } assb___time___on___zaap;
        } assb___time___on___zcbp;
      } assb___ifa___time___area;
    } assb___zcbp___time___area;
  struct {
    double         _assb___time___on___cp; /* Synonym for ASSB_TASK_TIME_ON_CP */
    } assb___task___time___on___cp;
  unsigned char  assbmtci[5];                              /* Memterm component ID. Set only  @MJA */
  char           assbflg4;                                 /* Flags                           @MLA */
  char           assbqiop;                                 /* Queued Director I/O Priority         */
  unsigned char  assbcryp;                                 /* Crypto flags. Serialization:    @MTA */
  double         assbphtm___base;                          /* Value in ASSBPHTM at the end of @PCA */
  void * __ptr32 assbearlymemtermrm;                       /* Address of Early Memterm Resource    */
  int            assb___laa___cpid;                        /* Anchor for LAA CPOOL            @MOA */
  struct {
    struct {
      double         _assb___ifa___phtm; /* @0PA */
      } assb___zaap___phtm;
    } assb___zcbp___phtm;
  struct {
    int            _assb___time___on___ziip___disps; /* Count of task dispatches for zIIP    */
    int            _assb___ziip___enct___disps;      /* Count of enclave dispatches for zIIP */
    } assb___enct___prezos11;
  struct {
    struct {
      int            _assb___nonenct___psrb___zaap___disps; /* Count of non-enclave preemptable */
      } assb___nonenct___psrb___zcbp___disps;
    int            _assb___nonenct___psrb___ziip___disps; /* Count of non-enclave preemptable */
    } assb___ifa___enct___prezos11;
  double         assb___base___phtm;                       /* Base value, set by WLM          @MQA */
  struct {
    double         _assb___ifa___base___phtm; /* Base value, set by WLM          @MQA */
    } assb___zcbp___base___phtm;
  double         assb___base___enct;                       /* Base value, set by WLM          @MQA */
  struct {
    double         _assb___ifa___base___enct; /* Base value, set by WLM          @MQA */
    } assb___zcbp___base___enct;
  void * __ptr32 assb___cp___affinity___node;              /* WUQ for CP affinity             @MQA */
  struct {
    void * __ptr32 _assb___ifa___affinity___node; /* WUQ for IFA affinity            @MQA */
    } assb___zcbp___affinity___node;
  struct {
    void * __ptr32 _assb___sup___affinity___node;
    } assb___ziip___affinity___node;
  int            assbsrbcpoolpmecount;                     /* Count of active SRB CPOOL PMEs       */
  struct {
    double         _assb___time___on___sup; /* @H4A */
    } assb___time___on___ziip;
  struct {
    double         _assb___time___sup___on___cp; /* @H4A */
    } assb___time___ziip___on___cp;
  struct {
    double         _assb___sup___phtm;
    } assb___ziip___phtm;
  double         assb___srb___time___on___cp;              /* Time on CP in SRB mode for this      */
  struct {
    double         _assb___sup___enct;
    } assb___ziip___enct;
  struct {
    double         _assb___ifa___on___cp___enct; /* Enclave time for IFA on CP in an */
    } assb___zcbp___on___cp___enct;
  void * __ptr32 assbsown;                                 /* Address of the Suspended SRB    @MOC */
  void * __ptr32 assbsowt;                                 /* SSD Static Ownership queue      @MSA */
  struct {
    double         _assb___ifa___on___cp___base___enct; /* Enclave base time for IFA on CP in */
    } assb___zcbp___on___cp___base___enct;
  double         assb___time___at___pdp;                   /* Trickle promotion CPU time (PDP      */
  double         assb___srbt___base;                       /* BASE TIME FOR ASCBSRBT          @MWA */
  void * __ptr32 assbcasc;                                 /* Address of the Console Address  @PEA */
  int            assbnumberunauthpets;                     /* Number of unauthorized PETs          */
  double         assb___asst___time___on___cp;             /* Additional SRB Service Time     @H5A */
  double         assb___switch___to___zaapziip___count;    /* When not zAAP_On_zIIP initially,     */
  double         assbasab;                                 /* Address of IQP ASAB             @N6A */
  struct {
    double         _assb___sup___base___enct; /* Base value, set by WLM          @N3A */
    } assb___ziip___base___enct;
  struct {
    double         _assb___sup___on___cp___enct; /* See ASSB_zIIP_ON_CP_ENCT */
    } assb___ziip___on___cp___enct;
  struct {
    double         _assb___sup___on___cp___base___enct; /* Enclave base time for SUP on CP in */
    } assb___ziip___on___cp___base___enct;
  void * __ptr32 assbrmin;                                 /* Address of RTM's reserved RMPL       */
  void * __ptr32 assbrtma;                                 /* Address of RTM's reserved RTM2WA     */
  double         assb___hdlockpromotion___time___at___pdp; /* Non-enclave HD=YES lock promote      */
  struct {
    void * __ptr32 _assb___smfcms___lockinst___addr; /* Address of the SMF CMS */
    } assb___lockinst___ptrs;
  void * __ptr32 assb___enqdeq___cms___lockinst___addr;    /* Address of the ENQ/DEQ CMS           */
  void * __ptr32 assb___latch___cms___lockinst___addr;     /* Address of the Latch CMS             */
  void * __ptr32 assb___cms___lockinst___addr;             /* Address of the CMS                   */
  void * __ptr32 assb___local___lockinst___addr;           /* Address of the local and CML lock    */
  int            assb___hdlockpromote___disps;             /* Count of non-enclave HD=YES lock     */
  struct {
    void * __ptr32 _assbsawq;      /* -                 ADDRESS OF ADDRESS SPACE SRB WEB */
    unsigned char  _assbr304[252]; /* Reserved. DO NOT USE            @N8A               */
    } assb___300;
  struct {
    void * __ptr32 _assbhreq;      /* -                 Local lock requestor address    @N8A */
    short int      _assbhasi;      /* -               Local lock owning ASID          @N8A   */
    unsigned char  _assbr406[250]; /* Reserved. DO NOT USE            @N8A                   */
    } assb___400;
  struct {
    double         _assb___enct;                         /* Enclave time in an address space     */
    int            _assb___enct___disps;                 /* Count of enclave dispatches for cp   */
    int            _assb___enct___hdlockpromote___disps; /* Count of enclave HD=YES lock         */
    double         _assb___enct___hdlockpromote___time;  /* Enclave HD=YES lock promote CPU      */
    unsigned char  _assbr518[232];                       /* Reserved. DO NOT USE            @0PC */
    } assb___500;
  struct {
    struct {
      struct {
        double         _assb___ifa___enct; /* @0PA */
        } assb___zaap___enct;
      } assb___zcbp___enct;
    struct {
      int            _assb___zaap___enct___disps; /* Count of enclave */
      } assb___zcbp___enct___disps;
    int            _assbbrokenup___seqnum;                  /* Sequence number of changes to        */
    unsigned char  _assbdiag610[160];                       /* Diagnostic data for IBM use only@NWC */
    struct {
      void          *_assb___ccb___realaddr;  /* Real address of            @0UA */
      void          *_assb___aicb___realaddr; /* Real address of           @0UA  */
      } assb___ccbaicb___realaddrs;
    unsigned char  _assbr6c0[16];                           /* Reserved. DO NOT USE            @0UC */
    struct {
      void          *_assb___aicb___virtaddr;     /* Virtual address of        @0UA  */
      void          *_assb___crypctrs___virtaddr; /* Virtual address of         @0UA */
      void          *_assb___nnpictrs___virtaddr; /* Virtual address of         @0UA */
      } assb___ctrs___virtaddr;
    struct {
      double         _assb___srb___time___on___zcbp; /* Non-preemptable SRB time on zCBP */
      } assb___after___ctrs;
    double         _assb___asst___time___on___zcbp;         /* non-enclave                          */
    int            _assb___srb___time___on___zcbp___disps;  /* Count of dispatches for              */
    int            _assb___asst___time___on___zcbp___disps; /* Count of dispatches for non-enclave  */
    } assb___600;
  struct {
    int            _assbetsc;      /* Enclave TCB Summary Count       @N8M */
    unsigned char  _assbr704[252]; /* Reserved. DO NOT USE            @N8A */
    } assb___700;
  struct {
    int            _assbcmlc;      /* -                 Count of CML locks held by */
    unsigned char  _assbr804[4];   /* Reserved.                       @NFA         */
    struct {
      void * __ptr32 _assbsvrb; /* Address of first available SVRB @NFA */
      int            _assbsync; /* Synchronization count           @NFA */
      } assbsupc;
    unsigned char  _assbr810[240]; /* Reserved. DO NOT USE            @NFA         */
    } assb___800;
  struct {
    double         _assb___vartime___at___pdp;            /* Total time promoted to a variable    */
    double         _assb___varweighted___time___at___pdp; /* Time promoted to a variable          */
    unsigned char  _assb___hcwa[8];                       /* HCW                             @NNC */
    double         _assb___scmbc;                         /* Storage Class Memory (SCM) block     */
    double         _assb___ziip___phtm___base;            /* Value in ASSB_zIIP_PHTM at the end   */
    int            _assb___majors___preempted;            /* Number of times major timeslices     */
    int            _assb___minors___preempted;            /* Number of times minor timeslices     */
    double         _assbinitiatorjobid;                   /* Initiator JOB ID                     */
    int            _assbinitiatorseqnum;                  /* Initiator Sequence Number.           */
    unsigned char  _assbr93c[4];                          /* Reserved                        @0PA */
    unsigned char  _assbdiag940[112];                     /* Diagnostic data for IBM use only@NVC */
    double         _assb___time___java___on___ziip;       /* Time Java is zIIP eligible and       */
    double         _assb___time___java___on___cp;         /* Time Java is zIIP eligible and       */
    } assb___900;
  double         assbend;                                  /* END OF ASSB.                         */
  };

#define assbassb                               assbegin._assbassb
#define assbsmfl                               assbegin._assbsmfl
#define assbstw1                               assbstkn._assbstw1
#define assbisqn                               assbstkn._assbisqn
#define assbflg0                               assbflgs._assbflg0
#define assbflg1                               assbflgs._assbflg1
#define assbflg2                               assbflgs._assbflg2
#define assbflg3                               assbflgs._assbflg3
#define assbldxh                               assbldax._assbldxh
#define assbldxl                               assbldax._assbldxl
#define assboepc                               assboecb._assboepc
#define assboees                               assboecb.assboecd._assboees
#define assb___nonenct___psrb___cp___disps     assbetsc___prezos11._assb___nonenct___psrb___cp___disps
#define assbslpc                               assbslsc._assbslpc
#define assbslnc                               assbslsc._assbslnc
#define assbctxf                               assbctx._assbctxf
#define assbctx2                               assbctx._assbctx2
#define assb___time___on___zaap___disps        assb___time___on___zcbp___disps._assb___time___on___zaap___disps
#define assbofl0                               assboflg._assbofl0
#define assbofl1                               assboflg._assbofl1
#define assb___time___ifa___on___cp            assb___zcbp___time___area.assb___ifa___time___area.assb___time___zcbp___on___cp._assb___time___ifa___on___cp
#define assb___time___on___ifa                 assb___zcbp___time___area.assb___ifa___time___area.assb___time___on___zcbp.assb___time___on___zaap._assb___time___on___ifa
#define assb___time___on___cp                  assb___task___time___on___cp._assb___time___on___cp
#define assb___ifa___phtm                      assb___zcbp___phtm.assb___zaap___phtm._assb___ifa___phtm
#define assb___time___on___ziip___disps        assb___enct___prezos11._assb___time___on___ziip___disps
#define assb___ziip___enct___disps             assb___enct___prezos11._assb___ziip___enct___disps
#define assb___nonenct___psrb___zaap___disps   assb___ifa___enct___prezos11.assb___nonenct___psrb___zcbp___disps._assb___nonenct___psrb___zaap___disps
#define assb___nonenct___psrb___ziip___disps   assb___ifa___enct___prezos11._assb___nonenct___psrb___ziip___disps
#define assb___ifa___base___phtm               assb___zcbp___base___phtm._assb___ifa___base___phtm
#define assb___ifa___base___enct               assb___zcbp___base___enct._assb___ifa___base___enct
#define assb___ifa___affinity___node           assb___zcbp___affinity___node._assb___ifa___affinity___node
#define assb___sup___affinity___node           assb___ziip___affinity___node._assb___sup___affinity___node
#define assb___time___on___sup                 assb___time___on___ziip._assb___time___on___sup
#define assb___time___sup___on___cp            assb___time___ziip___on___cp._assb___time___sup___on___cp
#define assb___sup___phtm                      assb___ziip___phtm._assb___sup___phtm
#define assb___sup___enct                      assb___ziip___enct._assb___sup___enct
#define assb___ifa___on___cp___enct            assb___zcbp___on___cp___enct._assb___ifa___on___cp___enct
#define assb___ifa___on___cp___base___enct     assb___zcbp___on___cp___base___enct._assb___ifa___on___cp___base___enct
#define assb___sup___base___enct               assb___ziip___base___enct._assb___sup___base___enct
#define assb___sup___on___cp___enct            assb___ziip___on___cp___enct._assb___sup___on___cp___enct
#define assb___sup___on___cp___base___enct     assb___ziip___on___cp___base___enct._assb___sup___on___cp___base___enct
#define assb___smfcms___lockinst___addr        assb___lockinst___ptrs._assb___smfcms___lockinst___addr
#define assbsawq                               assb___300._assbsawq
#define assbr304                               assb___300._assbr304
#define assbhreq                               assb___400._assbhreq
#define assbhasi                               assb___400._assbhasi
#define assbr406                               assb___400._assbr406
#define assb___enct                            assb___500._assb___enct
#define assb___enct___disps                    assb___500._assb___enct___disps
#define assb___enct___hdlockpromote___disps    assb___500._assb___enct___hdlockpromote___disps
#define assb___enct___hdlockpromote___time     assb___500._assb___enct___hdlockpromote___time
#define assbr518                               assb___500._assbr518
#define assb___ifa___enct                      assb___600.assb___zcbp___enct.assb___zaap___enct._assb___ifa___enct
#define assb___zaap___enct___disps             assb___600.assb___zcbp___enct___disps._assb___zaap___enct___disps
#define assbbrokenup___seqnum                  assb___600._assbbrokenup___seqnum
#define assbdiag610                            assb___600._assbdiag610
#define assb___ccb___realaddr                  assb___600.assb___ccbaicb___realaddrs._assb___ccb___realaddr
#define assb___aicb___realaddr                 assb___600.assb___ccbaicb___realaddrs._assb___aicb___realaddr
#define assbr6c0                               assb___600._assbr6c0
#define assb___aicb___virtaddr                 assb___600.assb___ctrs___virtaddr._assb___aicb___virtaddr
#define assb___crypctrs___virtaddr             assb___600.assb___ctrs___virtaddr._assb___crypctrs___virtaddr
#define assb___nnpictrs___virtaddr             assb___600.assb___ctrs___virtaddr._assb___nnpictrs___virtaddr
#define assb___srb___time___on___zcbp          assb___600.assb___after___ctrs._assb___srb___time___on___zcbp
#define assb___asst___time___on___zcbp         assb___600._assb___asst___time___on___zcbp
#define assb___srb___time___on___zcbp___disps  assb___600._assb___srb___time___on___zcbp___disps
#define assb___asst___time___on___zcbp___disps assb___600._assb___asst___time___on___zcbp___disps
#define assbetsc                               assb___700._assbetsc
#define assbr704                               assb___700._assbr704
#define assbcmlc                               assb___800._assbcmlc
#define assbr804                               assb___800._assbr804
#define assbsvrb                               assb___800.assbsupc._assbsvrb
#define assbsync                               assb___800.assbsupc._assbsync
#define assbr810                               assb___800._assbr810
#define assb___vartime___at___pdp              assb___900._assb___vartime___at___pdp
#define assb___varweighted___time___at___pdp   assb___900._assb___varweighted___time___at___pdp
#define assb___hcwa                            assb___900._assb___hcwa
#define assb___scmbc                           assb___900._assb___scmbc
#define assb___ziip___phtm___base              assb___900._assb___ziip___phtm___base
#define assb___majors___preempted              assb___900._assb___majors___preempted
#define assb___minors___preempted              assb___900._assb___minors___preempted
#define assbinitiatorjobid                     assb___900._assbinitiatorjobid
#define assbinitiatorseqnum                    assb___900._assbinitiatorseqnum
#define assbr93c                               assb___900._assbr93c
#define assbdiag940                            assb___900._assbdiag940
#define assb___time___java___on___ziip         assb___900._assb___time___java___on___ziip
#define assb___time___java___on___cp           assb___900._assb___time___java___on___cp

/* Values for field "assbxmf1" */
#define assbxeax                            0x80 /* ADDRESS SPACE OWNS ENTRY TABLES @L4A               */

/* Values for field "assbstw1" */
#define assbstyp                            0xF0 /* FIRST 4 BITS REPRESENT STOKEN   @L3A               */

/* Values for field "assbmcso" */
#define assbemcs___activated                0x80 /* At least one EMCS console was   @MKA               */

/* Values for field "assbflg0" */
#define assbbsdn                            0x80 /* BYPASS SVC DUMP                 @D7C               */
#define assbcdsi                            0x40 /* CDSI Resources Held             @DDA               */
#define assbpsch                            0x20 /* Parallel Detach SRB scheduled   @09A               */
#define assbpnsw                            0x10 /* If on, this space is declared by                   */
#define assbnoml                            0x08 /* NoML used internally                               */
#define assbsdumpas                         0x04 /* SDUMP is dumping this address   @0JC               */
#define assbsdumpnd                         0x02 /* SDUMP set the tasks in this     @0JA               */
#define assbsdumpresetnd                    0x01 /* Request SDUMP to set tasks      @0JA               */

/* Values for field "assbflg1" */
#define assbntsl                            0x20 /* JOB STEP HAS CREATED            @LIA               */
#define assbfrst                            0x10 /* The first pool of SVRBs has     @0CA               */

/* Values for field "assbflg2" */
#define assbenfl                            0x80 /* IF ON, INDICATES ADDRESS SPACE  @LRA               */
#define assbnswf                            0x40 /* If on, indicates IEAVEGR found  @LSA               */
#define assbpran                            0x20 /* No longer set - kept for        @09C               */

/* Values for field "assbflg3" */
#define assbarm                             0x80 /* The job or STC running in this  @LVA               */
#define assbnrst                            0x40 /* The Automatic Restart Manager   @LVA               */
#define assbgdps                            0x20 /* If on, indicates this is the                       */
#define assbmdip                            0x10 /* If on, indicates that a Memterm                    */
#define assbbcpiiused                       0x08 /* If on, indicates this                              */
#define assb___initially___zaap___on___ziip 0x04 /* Indicates that zAAP on zIIP was                    */
#define assbmtdc                            0x02 /* If on, indicates that Memterm dump                 */

/* Values for field "assbmt_n_" */
#define assbmtp                             0x80 /* MEMTERM PENDING.  TURNED ON     @DAA               */

/* Values for field "assbdfp" */
#define assboam                             0x80 /* ADDRESS SPACE IS A USER OF OAM  @03A               */

/* Values for field "assbttfl" */
#define assbttrc                            0x80 /* Transaction Trace has been used.@MCA               */

/* Values for field "assbwmf1" */
#define assbwini                            0x80 /* WLM Managed Batch initiator     @M8A               */
#define assbfsas                            0x40 /* WLM Managed OE Forked/Spawned   @M8A               */

/* Values for field "assbctxf" */
#define assbncl                             0x80 /* There is no limit to the number @MAM               */
#define assbmsgi                            0x40 /* The message to relax the above  @MAM               */
#define assburmx                            0x20 /* There is no limit to the number @MAA               */
#define assburmm                            0x10 /* The message to relax the above  @MAA               */

/* Values for field "assbofl0" */
#define assbomsc                            0x80 /* USS address space must remain   @DEA               */
#define assbodwt                            0x40 /* USS process awaiting dub.       @MHA               */
#define assbosdb                            0x20 /* Allow address space to dub at                      */
#define assbtdaff                           0x10 /* CInet Addr Sp Transport Dr                         */

/* Values for field "assbflg4" */
#define assb___authle                       0x80 /* Resources have been allocated.  @MLA               */

/* Values for field "assbcryp" */
#define assbsods                            0x80 /* If set to '1'b, the address     @MTA               */
#define assbcrnq                            0x40 /* If set to '1'b, the address     @NCA               */
#define assbssl                             0x01 /* If set to '1'b, the address                        */

/* Values for field "assb___cp___affinity___node" */
#define assb___entitle___nominee            0x80 /* Entitle nominee                 @NGA               */

/* Values for field "assbrmin" */
#define assbrtmi                            0x80 /* When on, the reserved RMPL pointed                 */

/* Values for field "assbsawq" */
#define assburrq                            0x80 /* -             SYSEVENT USER READY REQUIRED    @N8A */

#endif

#ifdef __open_xl__
#pragma pack()
#else
#pragma pack(reset)
#endif

#ifdef __open_xl__
#pragma pack(1)
#else
#pragma pack(packed)
#endif

#ifndef __dlaahdr__
#define __dlaahdr__

struct dlaahdr {
  union {
    int            _dlaahnumls; /* Number of DLAALS entries which follow       */
    int            _dlaah_n_ls; /* Same as DLAAHNumLS                          */
    int            _dlaahnumja; /* Number of DLAAJA entries which follow, when */
    int            _dlaah_n_ja; /* Same as DLAAHNumJA                          */
    } _dlaahdr_union1;
  union {
    int            _dlaahnumrem; /* Number of DLAALS or DLAAJA entries which were */
    int            _dlaah_n_rem; /* Same as DLAAHNumREM                           */
    } _dlaahdr_union2;
  int            dlaahtlen; /* Total length of answer area needed to contain */
  union {
    void * __ptr32 _dlaahfirstlsaddr; /* Address of first DLAALS */
    void * __ptr32 _dlaahfirstls_a_;  /* Same as FIRSTLSADDR     */
    void * __ptr32 _dlaahfirstjaaddr; /* Address of first DLAAJA */
    void * __ptr32 _dlaahfirstja_a_;  /* Same as FIRSTJAADDR     */
    } _dlaahdr_union3;
  };

#define dlaahnumls       _dlaahdr_union1._dlaahnumls
#define dlaah_n_ls       _dlaahdr_union1._dlaah_n_ls
#define dlaahnumja       _dlaahdr_union1._dlaahnumja
#define dlaah_n_ja       _dlaahdr_union1._dlaah_n_ja
#define dlaahnumrem      _dlaahdr_union2._dlaahnumrem
#define dlaah_n_rem      _dlaahdr_union2._dlaah_n_rem
#define dlaahfirstlsaddr _dlaahdr_union3._dlaahfirstlsaddr
#define dlaahfirstls_a_  _dlaahdr_union3._dlaahfirstls_a_
#define dlaahfirstjaaddr _dlaahdr_union3._dlaahfirstjaaddr
#define dlaahfirstja_a_  _dlaahdr_union3._dlaahfirstja_a_

/* Values for field "dlaahfirstja_a_" */
#define dlaahdr___len 0x10

#endif

#ifndef __dlaals__
#define __dlaals__

struct dlaals {
  union {
    void * __ptr32 _dlaalsnextaddr; /* Address of next DLAALS. DLAAHNumLS must be */
    void * __ptr32 _dlaalsnext_a_;  /* Same as NEXTADDR                           */
    } _dlaals_union1;
  union {
    void * __ptr32 _dlaalsfirstdsaddr; /* Address of first DLAADS for this DLAALS */
    void * __ptr32 _dlaalsfirstds_a_;  /* Same as FirstDSADDR                     */
    } _dlaals_union2;
  union {
    void * __ptr32 _dlaalsfirstuaddr; /* Address of first DLAAU for this DLAALS */
    void * __ptr32 _dlaalsfirstu_a_;  /* Same as FirstUADDR                     */
    } _dlaals_union3;
  unsigned char  dlaalsname[16]; /* Name of LNKLST set */
  unsigned char  dlaalsflags;    /* Flags              */
  unsigned char  _filler1[3];    /* UNUSED             */
  union {
    int            _dlaalslnklstseqnum; /* The SeqNum of this LNKLST set. Only valid */
    int            _dlaalslnklstseq_n_; /* Same as DLAALSLnklstSeqNum                */
    } _dlaals_union4;
  unsigned char  _filler2[4];    /* Unused             */
  union {
    short int      _dlaalsnumds; /* Number of DLAADS entries associated with this */
    short int      _dlaals_n_ds; /* Same as DLAALSNumDS                           */
    } _dlaals_union5;
  union {
    short int      _dlaalsnumu; /* Number of DLAAU entries associated with this */
    short int      _dlaals_n_u; /* Same as DLAALSNumU                           */
    } _dlaals_union6;
  };

#define dlaalsnextaddr     _dlaals_union1._dlaalsnextaddr
#define dlaalsnext_a_      _dlaals_union1._dlaalsnext_a_
#define dlaalsfirstdsaddr  _dlaals_union2._dlaalsfirstdsaddr
#define dlaalsfirstds_a_   _dlaals_union2._dlaalsfirstds_a_
#define dlaalsfirstuaddr   _dlaals_union3._dlaalsfirstuaddr
#define dlaalsfirstu_a_    _dlaals_union3._dlaalsfirstu_a_
#define dlaalslnklstseqnum _dlaals_union4._dlaalslnklstseqnum
#define dlaalslnklstseq_n_ _dlaals_union4._dlaalslnklstseq_n_
#define dlaalsnumds        _dlaals_union5._dlaalsnumds
#define dlaals_n_ds        _dlaals_union5._dlaals_n_ds
#define dlaalsnumu         _dlaals_union6._dlaalsnumu
#define dlaals_n_u         _dlaals_union6._dlaals_n_u

/* Values for field "dlaalsflags" */
#define dlaalscurrent    0x80 /* This is the current LNKLST              */
#define dlaalswascurrent 0x40 /* This used to be a current LNKLST and is */
#define dlaalsinusebylla 0x20 /* LLA is monitoring the LNKLST using this */

/* Values for field "dlaals_n_u" */
#define dlaals___len     0x2C

#endif

#ifndef __dlaads__
#define __dlaads__

struct dlaads {
  union {
    void * __ptr32 _dlaadsnextaddr; /* Address of next DLAADS. DLAALSNumDS must be */
    void * __ptr32 _dlaadsnext_a_;  /* Same as NEXTADDR                            */
    } _dlaads_union1;
  unsigned char  dlaadsflags;    /* Flags                                      */
  unsigned char  _filler1[3];    /* UNUSED                                     */
  unsigned char  dlaadsvolid[6]; /* Volume ID. Represents status as of last    */
  short int      dlaadsnamelen;  /* Length of name                             */
  unsigned char  dlaadsname[44]; /* Data set name. It will only occupy as much */
  };

#define dlaadsnextaddr _dlaads_union1._dlaadsnextaddr
#define dlaadsnext_a_  _dlaads_union1._dlaadsnext_a_

/* Values for field "dlaadsflags" */
#define dlaadsapf             0x80 /* APF-authorized. Represents status as of last */
#define dlaadsapfnotavailable 0x40 /* APF status is not available. Either          */
#define dlaadssmsmanaged      0x20 /* The data set is SMS-managed. Represents      */
#define dlaadssmsnotavailable 0x10 /* SMS status is not available. Either          */

/* Values for field "dlaadsname" */
#define dlaads___len          0x3C

#endif

#ifndef __dlaau__
#define __dlaau__

struct dlaau {
  union {
    void * __ptr32 _dlaaunextaddr; /* Address of next DLAAU. DLAALSNumU must be */
    void * __ptr32 _dlaaunext_a_;  /* Same as NEXTADDR                          */
    } _dlaau_union1;
  unsigned char  dlaaujobname[8]; /* Job name using this LNKLST set */
  short int      dlaauasid;       /* ASID of job                    */
  unsigned char  _filler1[2];     /* UNUSED                         */
  };

#define dlaaunextaddr _dlaau_union1._dlaaunextaddr
#define dlaaunext_a_  _dlaau_union1._dlaaunext_a_

/* Values for field "_filler1" */
#define dlaau___len 0x10

#endif

#ifndef __dlaaja__
#define __dlaaja__

struct dlaaja {
  union {
    void * __ptr32 _dlaajanextaddr; /* Address of next DLAAJA. DLAALSNumJA must be */
    void * __ptr32 _dlaajanext_a_;  /* Same as NEXTADDR                            */
    } _dlaaja_union1;
  unsigned char  dlaajajobname[8]; /* Job name        */
  short int      dlaajaasid;       /* ASID of job     */
  unsigned char  _filler1[2];      /* UNUSED          */
  unsigned char  dlaajalsname[16]; /* LNKLST set name */
  };

#define dlaajanextaddr _dlaaja_union1._dlaajanextaddr
#define dlaajanext_a_  _dlaaja_union1._dlaajanext_a_

/* Values for field "dlaajalsname" */
#define csvdynldynnotavailable              0
#define csvdynldynavailable                 1
#define csvdynlrsncodemask                  0xFFFF /* Use this mask to isolate the non    */
#define csvdynlrc___ok                      0      /* Return code 0, success              */
#define csvdynlrc___warn                    4      /* Return code 4, warning              */
#define csvdynlrsnroutinenotfound           0x402  /* For TEST request, the               */
#define csvdynlrsnnotalldatareturned        0x403  /* For LIST, the provided              */
#define csvdynlrsnnomatchingjob             0x406  /* For UPDATE request, no              */
#define csvdynlrc___invparm                 8      /* Return code 8, invalid parameter    */
#define csvdynlrsnbadparmlist               0x801  /* Error while accessing parameter     */
#define csvdynlrsnsrbmode                   0x802  /* Caller was in SRB mode              */
#define csvdynlrsnnotenabled                0x803  /* Caller was not enabled              */
#define csvdynlrsnnotauthorized             0x804  /* Caller was not authorized           */
#define csvdynlrsnhomenotprimary            0x805  /* HASN ^= PASN                        */
#define csvdynlrsnbadansareaalet            0x806  /* ALET of ANSAREA was not             */
#define csvdynlrsnbadansarea                0x807  /* Error while accessing ANSAREA       */
#define csvdynlrsnbadanslen                 0x808  /* ANSLEN was not at least as long as  */
#define csvdynlrsnbadrequesttype            0x809  /* Parameter list contains an          */
#define csvdynlrsnbadestaex                 0x80A  /* ESTAEX recovery could not be        */
#define csvdynlrsnreservednot0              0x80B  /* Parameter list contains a non-0     */
#define csvdynlrsnbadparmlistalet           0x80D  /* ALET of parameter list was          */
#define csvdynlrsnbadversion                0x80E  /* Parameter list contains an          */
#define csvdynlrsnlocked                    0x80F  /* Caller held a system lock.          */
#define csvdynlrsnbaddsnamearea             0x815  /* Error while accessing area          */
#define csvdynlrsnbadafterdsnamearea        0x816  /* Error while accessing               */
#define csvdynlrsnbadopen                   0x818  /* Unable to open supplied data set.   */
#define csvdynlrsnlnklstsetnotfound         0x819  /* LNKLST set does not exist           */
#define csvdynlrsndatasetnotfound           0x81C  /* For DELETE, data set was not        */
#define csvdynlrsnbaddsnamealet             0x820  /* ALET of area containing DSNAME      */
#define csvdynlrsnbadafterdsnamealet        0x821  /* ALET of area containing             */
#define csvdynlrsnbadlnklstname             0x822  /* LNKLST set name begins with         */
#define csvdynlrsnbaddsname                 0x823  /* DSNAME begins with blank or hex     */
#define csvdynlrsnbadafterdsname            0x824  /* AFTERDSNAME begins with blank       */
#define csvdynlrsnbadalloc                  0x829  /* Unable to allocate requested data   */
#define csvdynlrsnfunctionnotavailableerror 0x82B  /* Function requested                  */
#define csvdynlrsnreservedname              0x831  /* Reserved name "CURRENT" or          */
#define csvdynlrsnnojobasid                 0x832  /* The job name was blank (or null)    */
#define csvdynlrsnaddsysdsn                 0x833  /* A request was made to add the       */
#define csvdynlrsndeletesysdsn              0x834  /* A request was made to delete        */
#define csvdynlrsnnocopyfrom                0x835  /* Could not locate the COPYFROM       */
#define csvdynlrsnalreadyexists             0x836  /* For DEFINE request, LNKLST set      */
#define csvdynlrsnnomodname                 0x837  /* Module name was null                */
#define csvdynlrsnconcatfull                0x839  /* Attempt to ADD a data set but the   */
#define csvdynlrsnbadprobdsnamearea         0x83A  /* Error while accessing area          */
#define csvdynlrsnbadprobdsnamealet         0x83B  /* ALET of area to contain             */
#define csvdynlrsnnotpartitioned            0x83C  /* The data set is not                 */
#define csvdynlrsnbadvolid                  0x83D  /* The provided VolID does not match   */
#define csvdynlrsnmultivolume               0x83E  /* IEFDDSRV's output was not as        */
#define csvdynlrsnmissingsysdsn             0x83F  /* The LNKLST set being tested         */
#define csvdynlrsnundefinecurrent           0x840  /* An attempt was made to              */
#define csvdynlrsnbadfounddsnamearea        0x841  /* Error while accessing               */
#define csvdynlrsnbadfounddsnamealet        0x842  /* ALET of area to contain             */
#define csvdynlrsnbadsms                    0x843  /* The SMS status of the data set has  */
#define csvdynlrc___env                     12     /* Return code 12, environmental error */
#define csvdynlrsnfunctionnotavailable      0xC01
#define csvdynlrsnnostorage                 0xC02  /* Storage was not available for a     */
#define csvdynlrsnchangeinuse               0xC03  /* An attempt was made to change       */
#define csvdynlrsnundefineusers             0xC04  /* An attempt was made to              */
#define csvdynlrsnundefinella               0xC06  /* An attempt was made to UNDEFINE     */
#define csvdynlrsnbadiefddsrv               0xC07  /* Bad return code from IEFDDSRV.      */
#define csvdynlrc___comperror               16     /* Unknown, unexpected error           */
#define csvdynlrsncomperror                 0x1001 /* System error encountered by         */
#define dlaaja___len                        0x20

#endif

#ifdef __open_xl__
#pragma pack()
#else
#pragma pack(reset)
#endif

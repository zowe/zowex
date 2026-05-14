#ifdef __open_xl__
#pragma pack(1)
#else
#pragma pack(packed)
#endif

#ifndef __apfhdr__
#define __apfhdr__

struct apfhdr {
  struct {
    int            _apfh_n_rec; /* Same as APFHNumREC */
    } apfhnumrec;
  struct {
    int            _apfh_n_rem; /* Same as APFHNumREM */
    } apfhnumrem;
  int            apfhtlen; /* Total length of answer area needed to contain */
  int            apfhoff;  /* Offset from APFHDR to first APFE              */
  };

#define apfh_n_rec apfhnumrec._apfh_n_rec
#define apfh_n_rem apfhnumrem._apfh_n_rem

/* Values for field "apfhoff" */
#define apfhdr___len 0x10

#endif

#ifndef __apfe__
#define __apfe__

struct apfe {
  short int      apfelen;        /* Length of APFE record. Use this length to get */
  unsigned char  apfedslen;      /* Length of dataset name through last non-blank */
  unsigned char  apfeflags;      /* Flags                                         */
  unsigned char  apfevolume[6];  /* Volume ID                                     */
  unsigned char  apfedsname[44]; /* Dataset name                                  */
  unsigned char  _filler1[2];    /* Reserved                                      */
  };

/* Values for field "apfeflags" */
#define apfesms                       0x80   /* Dataset is SMS-managed              */

/* Values for field "_filler1" */
#define csvapfformatstatic            0      /* Format is static                    */
#define csvapfformatdynamic           1      /* Format is dynamic                   */
#define csvapfrsncodemask             0xFFFF /* Use this mask to isolate the non    */
#define csvapfrc___ok                 0      /* Return code 0, success              */
#define csvapfrc___warn               4      /* Return code 4, warning              */
#define csvapfrsnalreadyinlist        0x401
#define csvapfrsninlistsmsmanaged     0x401
#define csvapfrsnnotinlist            0x402
#define csvapfrsnnotalldatareturned   0x403
#define csvapfrc___invparm            8      /* Return code 8, invalid parameter    */
#define csvapfrsnbadparmlist          0x801
#define csvapfrsnsrbmode              0x802
#define csvapfrsnnotenabled           0x803
#define csvapfrsnnotauthorized        0x804
#define csvapfrsnhomenotprimary       0x805
#define csvapfrsnbadansareaalet       0x806
#define csvapfrsnbadansarea           0x807
#define csvapfrsnbadanslen            0x808
#define csvapfrsnbadrequesttype       0x809
#define csvapfrsnbadestae             0x80A
#define csvapfrsnreservednot0         0x80B
#define csvapfrsnbaddsname            0x80C
#define csvapfrsnbadparmlistalet      0x80D
#define csvapfrsnbadversion           0x80E
#define csvapfrsnlocked               0x80F
#define csvapfrc___env                12     /* Return code 12, environmental error */
#define csvapfrsnfunctionnotavailable 0xC01
#define csvapfrsnwrongdfplevel        0xC02  /* DFSMS/MVS 1.1 is not installed.     */
#define csvapfrsnwrongdfsmslevel      0xC02  /* DFSMS/MVS 1.1 is not                */
#define csvapfrc___comperror          16     /* Unknown, unexpected error           */
#define csvapfrsncomperror            0x1001
#define apfe___len                    0x38

#endif

#ifdef __open_xl__
#pragma pack()
#else
#pragma pack(reset)
#endif

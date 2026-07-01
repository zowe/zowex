#ifdef __open_xl__
#pragma pack(1)
#else
#pragma pack(packed)
#endif

#ifndef __csvdynlst__
#define __csvdynlst__

/* CSVDYNL REQUEST=LIST answer area header (16 bytes) */
struct csvdynlst {
  int csvdynlstnrec; /* Number of entries returned               */
  int csvdynlstnrem; /* Number of entries not returned           */
  int csvdynlsttlen; /* Total length needed to hold all entries  */
  int csvdynlstofst; /* Offset from header to first entry        */
};

#define csvdynlst___len 0x10

/* Return / reason codes */
#define csvdynlrc___ok               0    /* All data returned                */
#define csvdynlrc___warn             4    /* Not all data returned            */
#define csvdynlrsnnotalldatareturned 0x401 /* Synthetic rsn set by ZUTMDYNLQ  */

#endif

#ifndef __csvdynlste__
#define __csvdynlste__

/* CSVDYNL REQUEST=LIST per-entry record (56 bytes = 0x38) */
struct csvdynlste {
  short int     csvdynlstelen;      /* Length of this entry (use to walk)   */
  unsigned char csvdynlstedsl;      /* Length of DSN through last non-blank */
  unsigned char csvdynlsteflg;      /* Flags                                */
  unsigned char csvdynlstevol[6];   /* Volume serial                        */
  unsigned char csvdynlstedsn[44];  /* Dataset name                         */
  unsigned char _filler1[2];        /* Reserved                             */
};

#define csvdynlste___len 0x38

#endif

#ifdef __open_xl__
#pragma pack()
#else
#pragma pack(reset)
#endif

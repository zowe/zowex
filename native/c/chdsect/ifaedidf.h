#ifdef __open_xl__
#pragma pack(1)
#else
#pragma pack(packed)
#endif

#ifndef __edoi__
#define __edoi__

struct edoi {
  unsigned char  edoiflags;
  unsigned char  _filler1[3];
  int            edoineededfeatureslen; /* The featureslen size provided at */
  union {
    unsigned char  _edoiprodversrelmod[6];
    struct {
      unsigned char  _edoiprodvers[2]; /* The version information provided at   */
      unsigned char  _edoiprodrel[2];  /* The release information provided at   */
      unsigned char  _edoiprodmod[2];  /* The mod level information provided at */
      } _edoi_struct1;
    } _edoi_union1;
  unsigned char  _filler2[2];
  };

#define edoiprodversrelmod _edoi_union1._edoiprodversrelmod
#define edoiprodvers       _edoi_union1._edoi_struct1._edoiprodvers
#define edoiprodrel        _edoi_union1._edoi_struct1._edoiprodrel
#define edoiprodmod        _edoi_union1._edoi_struct1._edoiprodmod

/* Values for field "edoiflags" */
#define edoiregistered                      0x80 /* The product is registered                     */
#define edoistatusnotdefined                0x40 /* The product is not known to be enabled        */
#define edoistatusenabled                   0x20 /* The product is enabled.                       */
#define edoinotallfeaturesreturned          0x10 /* The featureslen area was too                  */

/* Values for field "_filler2" */
#define ifaedreg___type___standard          0
#define ifaedreg___type___required          2    /* The register request should complete even     */
#define ifaedreg___type___noreport          4    /* The register request should not be            */
#define ifaedreg___type___licensedunderprod 8    /* The registering feature is not                */
#define ifaedreg___type___disabledmessage   16   /* If the product is found to be                 */
#define ifaedreg___type___notfounddisabled  32   /* If no enable/disable policy                   */
#define ifaedreg___success                  0    /* Register service completed successfully       */
#define ifaedreg___disabled                 4    /* Register service found that the product is    */
#define ifaedreg___notavailable             8    /* Register service is not available on this     */
#define ifaedreg___limitexceeded            12   /* too many unauthorized registrations for       */
#define ifaedreg___nottaskmode              16   /* Register service was not called in task       */
#define ifaedreg___xm                       20   /* Register service was not called with P=H=S    */
#define ifaedreg___badfeatureslen           24   /* Features length exceeds 1024.                 */
#define ifaedreg___nostorage                28   /* The system could not obtain needed storage.   */
#define ifaedreg___badtype                  32   /* The type parameter did not specify a word     */
#define ifaedreg___locked                   36   /* Register service was called holding a system  */
#define ifaedreg___frr                      40   /* Register service was called having an FRR     */
#define ifaeddrg___success                  0    /* Deregister service completed successfully     */
#define ifaeddrg___notavailable             8    /* Deregister service is not available on        */
#define ifaeddrg___notregistered            12   /* The product that was to be deregistered       */
#define ifaeddrg___nottaskmode              16   /* deregister service was not called in task     */
#define ifaeddrg___xm                       20   /* Deregister service was not called with P=H=S  */
#define ifaeddrg___notauth                  24   /* If not supervisor state, system key, or       */
#define ifaeddrg___locked                   36   /* Deregister service was called holding a       */
#define ifaeddrg___frr                      40   /* Deregister service was called having an FRR   */
#define ifaedsta___success                  0    /* Status service completed successfully         */
#define ifaedsta___notdefined               4    /* The status service found no entry             */
#define ifaedsta___notavailable             8    /* Status service is not available on this       */
#define ifaedsta___nottaskmode              16   /* Status service was not called in task         */
#define ifaedsta___xm                       20   /* Status service was not called with P=H=S      */
#define ifaedsta___locked                   36   /* Status service was called holding a system    */
#define ifaedsta___frr                      40   /* Status service was called having an FRR       */
#define ifaedlis___type___registered        1    /* Return the registration entry/entries         */
#define ifaedlis___type___state             2    /* Return the state entry/entries corresponding  */
#define ifaedlis___type___status            4    /* Return the status entry corresponding to      */
#define ifaedlis___type___noreport          8    /* When returning registration entries,          */
#define ifaedlis___success                  0    /* List service completed successfully           */
#define ifaedlis___notalldatareturned       4    /* List service had more data to return          */
#define ifaedlis___notavailable             8    /* List service is not available on this         */
#define ifaedlis___ansareatoosmall          12   /* The answer area, indicated by the             */
#define ifaedlis___nottaskmode              16   /* List service was not called in task mode.     */
#define ifaedlis___xm                       20   /* List service was not called with P=H=S        */
#define ifaedlis___badtype                  32   /* The type parameter did not specify a word     */
#define ifaedlis___locked                   36   /* List service was called holding a system lock */
#define ifaedlis___frr                      40   /* List service was called having an FRR         */
#define edoi___len                          0x10

#endif

#ifndef __edaahdr__
#define __edaahdr__

struct edaahdr {
  int            edaahnumr;       /* Number of Edaae entries which follow          */
  int            edaahnums;       /* Number of Edaae entries which follow          */
  int            edaahtlen;       /* Total length of answer area needed to contain */
  void * __ptr32 edaahfirstraddr; /* Address of first registered entry Edaae       */
  void * __ptr32 edaahfirstsaddr; /* Address of first state entry Edaae            */
  void * __ptr32 edaahstatusaddr; /* Address of the entry that represents the      */
  unsigned char  _filler1[8];     /* Unused                                        */
  };

/* Values for field "_filler1" */
#define edaahdr___len 0x20

#endif

#ifndef __edaae__
#define __edaae__

struct edaae {
  void * __ptr32 edaaenextaddr;     /* Address of next Edaae. EdaahNumR (for the */
  union {
    unsigned char  _edaaeinfo[62];
    struct {
      unsigned char  _edaaeprodowner[16];   /* Product owner     */
      unsigned char  _edaaeprodname[16];    /* Product name      */
      unsigned char  _edaaefeaturename[16]; /* Feature name      */
      unsigned char  _edaaeprodvers[2];     /* Product version   */
      unsigned char  _edaaeprodrel[2];      /* Product release   */
      unsigned char  _edaaeprodmod[2];      /* Product mod level */
      unsigned char  _edaaeprodid[8];       /* Product ID        */
      } _edaae_struct1;
    } _edaae_union1;
  unsigned char  edaaeflags;        /* Flags                                     */
  unsigned char  _filler1;          /* Unused                                    */
  int            edaaenuminstances; /* Number of concurrent instances of this    */
  };

#define edaaeinfo        _edaae_union1._edaaeinfo
#define edaaeprodowner   _edaae_union1._edaae_struct1._edaaeprodowner
#define edaaeprodname    _edaae_union1._edaae_struct1._edaaeprodname
#define edaaefeaturename _edaae_union1._edaae_struct1._edaaefeaturename
#define edaaeprodvers    _edaae_union1._edaae_struct1._edaaeprodvers
#define edaaeprodrel     _edaae_union1._edaae_struct1._edaaeprodrel
#define edaaeprodmod     _edaae_union1._edaae_struct1._edaaeprodmod
#define edaaeprodid      _edaae_union1._edaae_struct1._edaaeprodid

/* Values for field "edaaeflags" */
#define edaaestatusnotdefined  0x80 /* This will never be on for entries on         */
#define edaaestatusenabled     0x40 /* If on, indicates that the product is         */
#define edaaenoreport          0x20 /* If on, indicates that the product registered */
#define edaaelicensedunderprod 0x10 /* If on, indicates that the product            */

/* Values for field "edaaenuminstances" */
#define edaae___len            0x48

#endif

#ifdef __open_xl__
#pragma pack()
#else
#pragma pack(reset)
#endif

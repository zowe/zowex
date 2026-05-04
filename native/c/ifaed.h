#ifndef IFAED_H
#define IFAED_H

typedef struct
{
  char value[8];
} IFAED_TOKEN;

typedef struct
{
  int type;
  char product_owner[16];
  char product_name[16];
  char feature_name[16];
  char prod_version[2];
  char prod_release[2];
  char prod_mod[2];
  char prod_id[8];
} IFAEDREG_PARMS;

typedef struct
{
  IFAED_TOKEN token;
  int return_code;
} IFAEDREG_RESPONSE;

#endif
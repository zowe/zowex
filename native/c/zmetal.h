/**
 * This program and the accompanying materials are made available under the terms of the
 * Eclipse Public License v2.0 which accompanies this distribution, and is available at
 * https://www.eclipse.org/legal/epl-v20.html
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Copyright Contributors to the Zowe Project.
 *
 */

#ifndef ZMETAL_H
#define ZMETAL_H

#include <stdio.h>
#include <string.h>
#include "ztype.h"

#if defined(__IBM_METAL__)
#include "ihapsa.h"
#include "ikjtcb.h"
#include "iezjscb.h"
#endif

#define MAX_PARM_LENGTH 100 + 1

typedef struct
{
  short int length;
  char parms[MAX_PARM_LENGTH];
} IN_DATA;

#define HI_BIT_MASK 0x7FFFFFFF
typedef struct
{
  union
  {
    IN_DATA *PTR32 addr;
    int addrValue;
  } data;
} IN_PARM;

#if defined(__IBM_METAL__)
#define S0C3(n)                                               \
  __asm(                                                      \
      "*                                                  \n" \
      " LLGF  0,%0      = Value passed by caller          \n" \
      " EXRL  0,*       Execute myself                    \n" \
      " DC    C'@S0C3'  Find by '@S0C3'                     " \
      "*                                                    " \
      :                                                       \
      : "m"(n)                                                \
      : "r0");
#else
#define S0C3(n)
#endif

static void s0c3_abend(int n)
{
  S0C3(n);
}

#if defined(__IBM_METAL__)
#define TESTAUTH(rc)                                          \
  __asm(                                                      \
      "*                                                  \n" \
      " TESTAUTH FCTN=1,"                                     \
      "RBLEVEL=1,"                                            \
      "STATE=YES,"                                            \
      "KEY=YES                                            \n" \
      "*                                                  \n" \
      " ST 15,%0               Save RC                    \n" \
      "*                                                    " \
      : "=m"(rc)::"r0", "r1", "r14", "r15");
#else
#define TESTAUTH(rc)
#endif

static int test_auth()
{
  int rc = 0;
  TESTAUTH(rc);
  return rc;
}

#if defined(__IBM_METAL__)
#define MODESET_MODE(value)                                                               \
  __asm(                                                                                  \
      "*                                                   \n"                            \
      " MODESET MODE=" #value##"                           \n"                            \
                               "*                                                    " :: \
                                   : "r0", "r1", "r14", "r15");
#else
#define MODESET_MODE(value)
#endif

#if defined(__IBM_METAL__)
#define MODESET_KEY(value)                                                               \
  __asm(                                                                                 \
      "*                                                   \n"                           \
      " MODESET KEY=" #value##"                             \n"                          \
                              "*                                                    " :: \
                                  : "r0", "r1", "r14", "r15");
#else
#define MODESET_KEY(value)
#endif

#if defined(__IBM_METAL__)
#define LOAD(name, ep, rc, rsn, save_regs)                      \
  __asm(                                                        \
      "*                                                    \n" \
      "* This can be removed for testing S0C4 on compress   \n" \
      "* Store registers for PDSMAN/IEBCOPY into save_high  \n" \
      " STMG  2,13,%3      Store for PDSMAN/IEBCOPY         \n" \
      "*                                                    \n" \
      "* Clear high-half of registers 2-15 for PDSMAN       \n" \
      " LMH   2,15,=16F'0' Clear for PDSMAN/IEBCOPY         \n" \
      "*                                                    \n" \
      " LOAD EPLOC=%4,"                                         \
      "PLISTVER=MAX,"                                           \
      "ERRET=*+4+6+4+4+4                                    \n" \
      "*                                                    \n" \
      " STG 0,%0           EP                               \n" \
      " ST  15,%1          RC = 0                           \n" \
      " ST  15,%2          RSN = 0                          \n" \
      " J   *+4+4+6+4+4                                     \n" \
      "*                                                    \n" \
      " SLGR 0,0           Clear ep                         \n" \
      " STG 0,%0           Zero                             \n" \
      " ST 1,%1            r1 = rc                          \n" \
      " ST 15,%2           r15 = rsn                        \n" \
      "*                                                    \n" \
      "* Restore saved registers from save_regs             \n" \
      " LMG   2,13,%3      Restore for PDSMAN/IEBCOPY       \n" \
      "*                                                    \n" \
      "*                                                      " \
      : "=m"(ep),                                               \
        "=m"(rc),                                               \
        "=m"(rsn),                                              \
        "=m"(save_regs)                                         \
      : "m"(name)                                               \
      : "r0", "r1", "r2", "r14", "r15");
#else
#define LOAD(name, ep, rc, rsn, save_regs)
#endif

#if defined(__IBM_METAL__)
#define DELETE(name, rc)                                         \
  __asm(                                                         \
      "*                                                     \n" \
      " DELETE EPLOC=%1                                      \n" \
      "*                                                     \n" \
      " ST 15,%0                                             \n" \
      "*                                                      "  \
      : "=m"(rc)                                                 \
      : "m"(name)                                                \
      : "r0", "r1", "r14", "r15");
#else
#define DELETE(name, rc)
#endif

#if defined(__IBM_METAL__)
#define GET_KEY(key)                                 \
  __asm(                                             \
      "*                                         \n" \
      " IPK (2)    Get key from PSW              \n" \
      " STC  2,%0  Save key                      \n" \
      "*                                          "  \
      : "=m"(key)                                    \
      :                                              \
      : "r2");
#else
#define GET_KEY(key)
#endif

#if defined(__IBM_METAL__)
#define SET_KEY(key)                                 \
  __asm(                                             \
      "*                                         \n" \
      " SLR  2,2          Clear                  \n" \
      " IC   2,%0         Get key value          \n" \
      " SPKA 0(2)         Set key in PSW         \n" \
      "*                                          "  \
      :                                              \
      : "m"(key)                                     \
      : "r2");
#else
#define SET_KEY(key)
#endif

#if defined(__IBM_METAL__)
#define GET_PSW(psw)                             \
  __asm(                                         \
      "*                                     \n" \
      " EPSW 0,1    Create stack entry       \n" \
      " STM 0,1,%0  Create stack entry       \n" \
      "*                                      "  \
      : "=m"(psw)                                \
      :                                          \
      : "r0", "r1");
#else
#define GET_PSW(psw)
#endif

typedef struct
{
  union
  {
    long double raw; // guarantees alignment of long double
    struct
    {
      unsigned int z0 : 1;      // always 0
      unsigned int r : 1;       // program event recording mask
      unsigned int z1 : 3;      // always 0
      unsigned int t : 1;       // DAT mode 0=off
      unsigned int i : 1;       // I/O mask
      unsigned int e : 1;       // extenal mask, 0 = no interupts
      unsigned int key : 4;     // program mask
      unsigned int a : 1;       // architecture 0 = z/Architecture
      unsigned int m : 1;       // machine check mask
      unsigned int w : 1;       // wait state, 0=ready
      unsigned int p : 1;       // problem state, 0=supervisor
      unsigned int as : 2;      // address space control
      unsigned int cc : 2;      // condition code
      unsigned int pgmmask : 4; // condition code
      unsigned int z2 : 7;      // always 0
      unsigned int ea : 1;      // extended addressing mode b'00' = 24, b'01' = 31, b'11' = 64

      unsigned int ba : 1;      // basic addressing mode
      unsigned int unused : 31; // future use

    } bits;
  } data;
} PSW;

static unsigned long long int get_raw_psw()
{
  unsigned long long int psw = 0;
  GET_PSW(psw);
  return psw;
}

static void get_psw(PSW *psw)
{
  unsigned long long int psw_raw = get_raw_psw();
  memcpy(psw, &psw_raw, sizeof(psw_raw));
}

static void set_key(unsigned char *key)
{
  SET_KEY(*key);
}

/**
 * @brief Unconditionally attempt to enter supervisor state
 */
static void mode_sup()
{
  MODESET_MODE(SUP);
}

/**
 * @brief Unconditionally attempt to enter problem state
 */
static void mode_prob()
{
  MODESET_MODE(PROB);
}

static void mode_zero()
{
  MODESET_KEY(ZERO);
}

static void mode_nzero()
{
  MODESET_KEY(NZERO);
}

static unsigned char get_key()
{
  unsigned char key = {0};
  GET_KEY(key);
  return key;
}

typedef struct psa PSA;
typedef struct tcb TCB;
typedef struct iezjscb IEZJSCB;

static void auth_off()
{
#if defined(__IBM_METAL__)
  PSA *psa = (PSA *)0;
  TCB *PTR32 tcb = (TCB * PTR32) psa->psatold;
  unsigned int ujjscb = 0;
  memcpy(&ujjscb, &tcb->tcbjscb, sizeof(tcb->tcbjscb));
  IEZJSCB *PTR32 jscb = (IEZJSCB * PTR32)(ujjscb & 0x00FFFFFF);

  if (0 == test_auth())
  {
    PSW psw = {0};
    get_psw(&psw);
    int mode_switch = psw.data.bits.p ? 1 : 0;
    unsigned char key = get_key();
    unsigned char key_zero = 0;
    if (mode_switch)
    {
      mode_sup();
    }
    set_key(&key_zero);
    jscb->jscbopts &= (0xFF - jscbauth);
    set_key(&key);
    if (mode_switch)
    {
      mode_prob();
    }
  }
#endif
}

/**
 * @brief Load a module into a 64-bit pointer
 *
 * @param name name of module to load
 * @return void* address of entry point or NULL if not found
 */
static void *PTR64 load_module(const char *name)
{
  int rc = 0;
  int rsn = 0;

  char name_truncated[8 + 1] = {0};
  memset(name_truncated, ' ', sizeof(name_truncated) - 1);                                                             // pad with spaces
  memcpy(name_truncated, name, strlen(name) > sizeof(name_truncated) - 1 ? sizeof(name_truncated) - 1 : strlen(name)); // truncate

  void *PTR64 ep = NULL;

  // Save 8 bytes for each register 2-13 (including 13) for PDSMAN/IEBCOPY
  unsigned long long save_registers[12] = {0};
  LOAD(name_truncated, ep, rc, rsn, save_registers[0]);
  if (0 != rc)
  {
    return NULL;
  }

  return ep;
}

typedef void (*PTR32 Z31FUNC)(void) ATTRIBUTE(amode31);
/**
 * @brief
 *
 * @param name name of module to load
 * @return Z31FUNC 31 bit function cleared for use within 64-bit routine or NULL if not found
 */
static Z31FUNC ATTRIBUTE(amode31) load_module31(const char *name)
{

  // TODO(Kelosky): test return pointer flags to validate amode??
  void *PTR64 function = load_module(name);
  Z31FUNC z31func = NULL;
  if (function)
  {
    // make 31 bit EP pointer valid in 64 bit
    long long unsigned int ifunction = (long long unsigned int)function;
    ifunction &= 0x000000007FFFFFFF; // clear high bit
    z31func = (Z31FUNC)ifunction;
  }

  return z31func;
}

/**
 * @brief Delete a module that has been loaded
 *
 * @param name name of module to delete after a successful load
 * @return int 0 for success; non zero otherwise
 */
static int delete_module(const char name[8])
{
  int rc = 0;
  char name_truncated[9] = {0};
  memset(name_truncated, ' ', sizeof(name_truncated) - 1);                                                             // pad with spaces
  memcpy(name_truncated, name, strlen(name) > sizeof(name_truncated) - 1 ? sizeof(name_truncated) - 1 : strlen(name)); // truncate
  DELETE(name_truncated, rc);
  return rc;
}

#if defined(__IBM_METAL__)
#define GET_REG64(reg, number)                                 \
  __asm(                                                       \
      "*                                                   \n" \
      " STG    " #number ",%0     Save reg                 \n" \
      "*                                                    "  \
      : "=m"(reg)                                              \
      :                                                        \
      :);
#else
#define GET_REG64(reg, number)
#endif

#if defined(__IBM_METAL__)
#define GET_ENTRY_REG64(reg, offset)                           \
  __asm(                                                       \
      "*                                                   \n" \
      " LG     1," #offset "(,13)   Offset to reg          \n" \
      " STG    1,%0                 Save reg               \n" \
      "*                                                    "  \
      : "=m"(reg)                                              \
      :                                                        \
      : "r1");
#else
#define GET_ENTRY_REG64(reg, offset)
#endif

#if defined(__IBM_METAL__)
#define GET_PREV_REG64(reg, offset)                            \
  __asm(                                                       \
      "*                                                   \n" \
      " LG     1,128(,13)           -> Prev SA             \n" \
      " LG     1," #offset "(,1)    Offset to reg          \n" \
      " STG    1,%0                 Save reg               \n" \
      "*                                                    "  \
      : "=m"(reg)                                              \
      :                                                        \
      : "r1");
#else
#define GET_PREV_REG64(reg, offset)
#endif

static unsigned long long int get_r0()
{
  unsigned long long int reg = 0;
  GET_REG64(reg, 0);
  return reg;
}

static unsigned long long int get_r1()
{
  unsigned long long int reg = 0;
  GET_REG64(reg, 1);
  return reg;
}

static unsigned long long int get_r2()
{
  unsigned long long int reg = 0;
  GET_REG64(reg, 2);
  return reg;
}

static unsigned long long int get_r5()
{
  unsigned long long int reg = 0;
  GET_REG64(reg, 5);
  return reg;
}

static unsigned long long int get_r6()
{
  unsigned long long int reg = 0;
  GET_REG64(reg, 6);
  return reg;
}

static unsigned long long int get_r7()
{
  unsigned long long int reg = 0;
  GET_REG64(reg, 7);
  return reg;
}

static unsigned long long int get_r13()
{
  unsigned long long int reg = 0;
  GET_REG64(reg, 13);
  return reg;
}

static void get_r14_by_ref(unsigned long long int *reg)
{
  GET_REG64(*reg, 14);
}

static unsigned long long int get_prev_r14()
{
  unsigned long long int reg = 0;
  GET_PREV_REG64(reg, 8);
  return reg;
}

static unsigned long long int get_prev_r15()
{
  unsigned long long int reg = 0;
  GET_PREV_REG64(reg, 16);
  return reg;
}

static unsigned long long int get_prev_r0()
{
  unsigned long long int reg = 0;
  GET_PREV_REG64(reg, 24);
  return reg;
}

static unsigned long long int get_prev_r1()
{
  unsigned long long int reg = 0;
  GET_PREV_REG64(reg, 32);
  return reg;
}

static unsigned long long int get_prev_r2()
{
  unsigned long long int reg = 0;
  GET_PREV_REG64(reg, 40);
  return reg;
}

#if defined(__IBM_METAL__)
#define GET_STACK_ENV(reg)                                        \
  __asm(                                                          \
      "*                                                      \n" \
      " LG     1,128(,13)   -> PREV SA                        \n" \
      " STG    1,%0         SAVE                              \n" \
      "*                                                       "  \
      : "=m"(reg)                                                 \
      :                                                           \
      : "r1");
#else
#define GET_STACK_ENV(regs)
#endif

static unsigned long long int get_prev_r13()
{
  unsigned long long int reg = 0;
  GET_STACK_ENV(reg);
  return reg;
}

#endif

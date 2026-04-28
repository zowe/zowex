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

#include "ihasdwa.h"
#include "zmetal.h"
#include "zrecovery.h"
#include "zsetjmp.h"
#include "zwto.h"
#include "zecb.h"
#include "zssi.h"
#include "zssitype.h"

#pragma prolog(ABEXIT, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(ABEXIT, " ZWEEPILG ")
void ABEXIT(SDWA *sdwa, void *abexit_data)
{
  zwto_debug("@TEST called on abend");
  int *i = (int *)abexit_data;
  zwto_debug("@TEST abexit_data %d", *i);
}

#pragma prolog(PERCEXIT, " ZWEPROLG NEWDSA=(YES,128) ")
#pragma epilog(PERCEXIT, " ZWEEPILG ")
void PERCEXIT(void *perc_exit_data)
{
  zwto_debug("@TEST called to percolate");
}

// Forward declarations for SSIB/SSOB initialization
static void init_ssib(SSIB *ssib);
static void init_ssob(SSOB *PTR32 ssob, SSIB *PTR32 ssib, void *PTR32 function_dependent_area, int function);
static void print_job_ids_from_ssi_data(STATJQ *PTR32 statjqp);
static int iefssreq_with_recovery(SSOB * PTR32 * PTR32 ssob);

// Test IEFSSREQ with recovery
static int test_iefssreq_with_recovery(void);
static int test_iefssreq_with_bad_pointer_recovery(void);

static void init_ssib(SSIB *ssib)
{
  memcpy(ssib->ssibid, "SSIB", sizeof(ssib->ssibid));
  ssib->ssiblen = sizeof(SSIB);
  // Simplified initialization - set JES2 as default
  ssib->ssibssid = 0x02; // JES2
  memcpy(ssib->ssibssnm, "JES2", 4);
}

static void init_ssob(SSOB *PTR32 ssob, SSIB *PTR32 ssib, void *PTR32 function_dependent_area, int function)
{
  memcpy(ssob->ssobid, "SSOB", sizeof(ssob->ssobid));
  ssob->ssoblen = sizeof(SSOB);
  ssob->ssobssib = ssib;
  ssob->ssobindv = (int)function_dependent_area;
  ssob->ssobfunc = function;
}

static void print_job_ids_from_ssi_data(STATJQ *PTR32 statjqp)
{
  int job_count = 0;

  // Walk through job queue entries and print first 3 job IDs
  while (statjqp && job_count < 3)
  {
    STATJQHD *PTR32 statjqhdp = (STATJQHD * PTR32)((unsigned char *PTR32)statjqp + statjqp->stjqohdr);
    STATJQTR *PTR32 statjqtrp = (STATJQTR * PTR32)((unsigned char *PTR32)statjqhdp + sizeof(STATJQHD));

    char jobid[9] = {0};
    memcpy(jobid, statjqtrp->sttrjid, 8);

    // Print just the job ID
    zwto_debug("%s", jobid);

    job_count++;
    statjqp = statjqp->stjqnext;
  }

  if (job_count == 0)
  {
    zwto_debug("@TEST No jobs found");
  }
}

// Our own recovery-enabled IEFSSREQ wrapper (similar to zssi.h iefssreq but with our recovery)
static int iefssreq_with_recovery(SSOB * PTR32 * PTR32 ssob)
{
  int rc = 0;
  ZRCVY_ENV zenv = {0};

  // Set up recovery environment with our ABEXIT and PERCEXIT functions
  zenv.abexit = ABEXIT;
  zenv.perc_exit = PERCEXIT;
  int recovery_data = 100; // Unique identifier for this recovery context
  zenv.abexit_data = &recovery_data;

  zwto_debug("@TEST Setting up our own recovery for IEFSSREQ call...");

  // Enable recovery before calling IEFSSREQ (unlike the commented out version in zssi.h)
  if (0 == enable_recovery(&zenv))
  {
    zwto_debug("@TEST Our recovery enabled - calling raw IEFSSREQ macro...");

    // Call the raw IEFSSREQ macro with OUR recovery protection
    // This is the key difference - we actually enable recovery, unlike zssi.h where it's commented out
    IEFSSREQ(ssob, rc);

    zwto_debug("@TEST IEFSSREQ completed under OUR recovery protection with RC=%d", rc);

    // Disable recovery after successful call
    disable_recovery(&zenv);
    return rc;
  }
  else
  {
    zwto_debug("@TEST SUCCESS: Our recovery was triggered! IEFSSREQ abend was caught and handled gracefully");
    // Recovery was triggered - we're in the recovery path
    disable_recovery(&zenv);
    return -1; // Return error code indicating recovery was used
  }
}

static void setup_ssob_for_job_list(SSOB *ssob, SSIB *ssib, STAT *stat)
{
  memset(stat, 0, sizeof(STAT));
  memcpy(stat->stateye, "STAT", sizeof(stat->stateye));
  stat->statverl = 11;   // statcvrl
  stat->statverm = 0;    // statcvrm
  stat->statlen = 0x21C; // statsize
  stat->stattype = 1;    // statters (terse/quick)
  stat->statsel1 = 0x04; // statsown (select by owner)

  // Set owner to current user IZUSVR
  memcpy(stat->statownr, "IZUSVR", 8);

  init_ssib(ssib);
  init_ssob(ssob, ssib, stat, 80);
}

static int test_iefssreq_with_recovery(void)
{
  zwto_debug("@TEST Starting IEFSSREQ test WITH our own recovery wrapper");

  int rc = 0;
  SSOB ssob = {0};
  SSIB ssib = {0};
  STAT stat = {0};
  SSOB *PTR32 ssobp = NULL;

  setup_ssob_for_job_list(&ssob, &ssib, &stat);

  zwto_debug("@TEST SSOB initialized: ID=%.4s, Len=%d, Func=%d",
             ssob.ssobid, ssob.ssoblen, ssob.ssobfunc);
  zwto_debug("@TEST SSIB initialized: ID=%.4s, Len=%d, SSID=%d",
             ssib.ssibid, ssib.ssiblen, ssib.ssibssid);
  zwto_debug("@TEST STAT configured for job listing: Owner=%.8s", stat.statownr);

  // Prepare pointer for IEFSSREQ macro
  ssobp = &ssob;
  ssobp = (SSOB * PTR32)((unsigned int)ssobp | 0x80000000);

  zwto_debug("@TEST Calling our own recovery-enabled IEFSSREQ wrapper...");

  // Call our own recovery-enabled IEFSSREQ wrapper
  rc = iefssreq_with_recovery(&ssobp);

  zwto_debug("@TEST Our recovery wrapper completed with RC=%d, SSOBRETN=%d", rc, ssob.ssobretn);

  // Check results
  if (rc == 0 && ssob.ssobretn == 0)
  {
    zwto_debug("@TEST SUCCESS: Our recovery-protected IEFSSREQ call succeeded!");

    // Display job data if available
    if (stat.statjobf != 0)
    {
      zwto_debug("@TEST Job data found - displaying first 3 job IDs from recovery-protected call:");
      STATJQ *PTR32 statjqp = (STATJQ * PTR32) stat.statjobf;
      print_job_ids_from_ssi_data(statjqp);

      // Clean up memory
      stat.stattype = 3; // statmem (free memory)
      zwto_debug("@TEST Cleaning up allocated SSI memory...");
      rc = iefssreq_with_recovery(&ssobp);
      zwto_debug("@TEST Memory cleanup completed with RC=%d", rc);
    }
  }
  else if (rc == -1)
  {
    zwto_debug("@TEST Recovery was triggered - abend was caught and handled gracefully");
  }
  else
  {
    zwto_debug("@TEST IEFSSREQ returned error - RC=%d, SSOBRETN=%d", rc, ssob.ssobretn);
  }

  return 0;
}

static int test_iefssreq_with_bad_pointer_recovery(void)
{
  zwto_debug("@TEST Starting iefssreq test with BAD pointer (tests built-in recovery)");

  int rc = 0;

  // Deliberately use a bad SSOB pointer to trigger an abend
  SSOB *PTR32 ssobp = (SSOB * PTR32)0x00000000; // NULL pointer - will cause S0C4

  zwto_debug("@TEST Calling iefssreq() with NULL pointer (should be handled by built-in recovery)...");

  // Call the recovery-enabled iefssreq function with bad pointer
  // The built-in recovery in iefssreq should handle any abend
  rc = iefssreq(&ssobp);

  zwto_debug("@TEST iefssreq with NULL pointer completed with RC=%d", rc);

  if (rc != 0)
  {
    zwto_debug("@TEST SUCCESS: iefssreq returned error code as expected");
  }
  else
  {
    zwto_debug("@TEST UNEXPECTED: iefssreq with NULL pointer succeeded! RC=%d", rc);
  }

  return 0;
}

int main()
{
  zwto_debug("@TEST Recovery test program starting");

  // Test 1: IEFSSREQ with our own recovery wrapper (the safe approach)
  zwto_debug("@TEST === Test 1: IEFSSREQ with Our Own Recovery Wrapper (safe approach) ===");
  test_iefssreq_with_recovery();

  // Test 2: IEFSSREQ with bad pointer and built-in recovery
  zwto_debug("@TEST === Test 2: IEFSSREQ with Bad Pointer (tests recovery) ===");
  test_iefssreq_with_bad_pointer_recovery();

  return 0;
}

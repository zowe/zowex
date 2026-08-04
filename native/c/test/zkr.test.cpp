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

// Unit tests for the `system cert` / `system keyring` command group (zkr service
// layer + CLI). Certificate and key ring operations require RACF authority that
// not every build user has, so the mutating tests are gated behind a runtime
// capability probe (see `can_mutate` below): where authority is missing they are
// SKIPPED, not failed, so the suite passes across a variety of user contexts.
// The command-validation and diagnostic tests need no special authority.
//
// R_datalib background and the SAF return-code conventions used here are in
// doc/certificates-code-walkthrough.md; keep assertions in sync with it.

#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>
#include <unistd.h>

#include "ztest.hpp"
#include "zutils.hpp"
#include "../zkr.hpp"
#include "../zkrtype.h"

using namespace ztst;

namespace
{

// A collision-resistant suffix for scratch ring/label names. Random() is fine in
// a normal test binary (unlike the workflow sandbox); combine it with the pid so
// concurrent test runs on the same LPAR do not clash.
std::string zkr_unique()
{
  return std::to_string(getpid()) + get_random_string(4);
}

// Delete a scratch ring, swallowing any error (best-effort cleanup).
void zkr_try_del_ring(const std::string &owner, const std::string &ring)
{
  ZKR z{};
  zkr_del_ring(&z, owner, ring);
}

// Purge a scratch certificate from the RACF database by label, swallowing errors.
void zkr_try_purge_cert(const std::string &owner, const std::string &label)
{
  ZKR z{};
  zkr_del_cert(&z, owner, "*", label, /*skip_refresh*/ true);
}

// Build a synthetic list entry for the zkr_filter_certs unit tests.
ZKRCertInfo zkr_mk_cert(const std::string &label, const std::string &usage)
{
  ZKRCertInfo c;
  c.label = label;
  c.owner = "TESTUSER";
  c.usage = usage;
  c.status = "TRUST";
  return c;
}

} // namespace

void zkr_tests()
{
  describe(
      "certificates (system cert / keyring)",
      [&]() -> void
      {
        const std::string owner = get_user();

        // ---------------------------------------------------------------------
        // CLI input validation. These exercise the argument/virtual-ring policy
        // in commands/certificates.cpp, which rejects bad input BEFORE any RACF
        // call, so they need no key ring authority and are fully deterministic.
        // ---------------------------------------------------------------------
        describe(
            "CLI input validation (no authority required)",
            [&]() -> void
            {
              it("`system cert --help` lists the certificate subcommands",
                 []() -> void
                 {
                   std::string out;
                   int rc = execute_command_with_output(zowex_command + " system cert --help", out);
                   ExpectWithContext(rc, out).ToBe(0);
                   Expect(out).ToContain("import");
                   Expect(out).ToContain("connect");
                   Expect(out).ToContain("set-default");
                   Expect(out).ToContain("rename");
                 });

              it("`system keyring --help` lists the key ring subcommands",
                 []() -> void
                 {
                   std::string out;
                   int rc = execute_command_with_output(zowex_command + " system keyring --help", out);
                   ExpectWithContext(rc, out).ToBe(0);
                   Expect(out).ToContain("create");
                   Expect(out).ToContain("list-rings");
                   Expect(out).ToContain("count");
                 });

              it("`cert delete` rejects the virtual ring '*' and points to --database",
                 []() -> void
                 {
                   std::string out;
                   int rc = execute_command_with_output(
                       zowex_command + " system cert delete TESTUSER '*' -l LBL", out);
                   Expect(rc).Not().ToBe(0);
                   Expect(out).ToContain("--database");
                 });

              it("`cert delete` requires a key ring or --database",
                 []() -> void
                 {
                   std::string out;
                   int rc = execute_command_with_output(
                       zowex_command + " system cert delete TESTUSER -l LBL", out);
                   Expect(rc).Not().ToBe(0);
                   Expect(out).ToContain("--database");
                 });

              it("`cert delete` rejects a key ring and --database together",
                 []() -> void
                 {
                   std::string out;
                   int rc = execute_command_with_output(
                       zowex_command + " system cert delete TESTUSER RING01 -l LBL --database", out);
                   Expect(rc).Not().ToBe(0);
                   Expect(out).ToContain("not both");
                 });

              it("`cert connect` rejects '*' for --from-ring and points to --from-database",
                 []() -> void
                 {
                   std::string out;
                   int rc = execute_command_with_output(
                       zowex_command + " system cert connect TESTUSER RING02 -l LBL --from-ring '*'", out);
                   Expect(rc).Not().ToBe(0);
                   Expect(out).ToContain("--from-database");
                 });

              it("`cert connect` requires --from-ring or --from-database",
                 []() -> void
                 {
                   std::string out;
                   int rc = execute_command_with_output(
                       zowex_command + " system cert connect TESTUSER RING02 -l LBL", out);
                   Expect(rc).Not().ToBe(0);
                   Expect(out).ToContain("--from-ring");
                 });
            });

        // ---------------------------------------------------------------------
        // List filter semantics: `keyring list --label/--usage` follows the
        // RACDCERT LABEL keyword -- exact, case-sensitive matches, no wildcards
        // -- and --max-entries caps MATCHING rows, so a filtered list must scan
        // past the page size. Pure functions over synthetic data; no RACF.
        // ---------------------------------------------------------------------
        describe(
            "list filter semantics (RACDCERT LABEL parity, no authority required)",
            [&]() -> void
            {
              it("finds an exact label match beyond the default page size",
                 []() -> void
                 {
                   // 12 certs; the target sits at position 11 -- past the default
                   // --max-entries page of 10 that used to truncate before filtering.
                   std::vector<ZKRCertInfo> certs;
                   for (int i = 0; i < 12; ++i)
                     certs.push_back(zkr_mk_cert("CERT" + std::to_string(i), "PERSONAL"));

                   bool more = true;
                   const std::vector<ZKRCertInfo> out = zkr_filter_certs(certs, "CERT10", "", 10, &more);
                   Expect(out.size()).ToBe(static_cast<size_t>(1));
                   Expect(out[0].label).ToBe("CERT10");
                   Expect(more).ToBe(false);
                 });

              it("label matching is case-sensitive",
                 []() -> void
                 {
                   std::vector<ZKRCertInfo> certs;
                   certs.push_back(zkr_mk_cert("MYCERT", "PERSONAL"));

                   bool more = false;
                   Expect(zkr_filter_certs(certs, "MyCert", "", 0, &more).size()).ToBe(static_cast<size_t>(0));
                   Expect(zkr_filter_certs(certs, "MYCERT", "", 0, &more).size()).ToBe(static_cast<size_t>(1));
                 });

              it("'*' is not a wildcard for labels",
                 []() -> void
                 {
                   std::vector<ZKRCertInfo> certs;
                   certs.push_back(zkr_mk_cert("CERT1", "PERSONAL"));
                   certs.push_back(zkr_mk_cert("*", "PERSONAL"));

                   const std::vector<ZKRCertInfo> out = zkr_filter_certs(certs, "*", "", 0, nullptr);
                   Expect(out.size()).ToBe(static_cast<size_t>(1));
                   Expect(out[0].label).ToBe("*");
                 });

              it("--max-entries caps matching rows and flags more",
                 []() -> void
                 {
                   // 30 certs alternating usage: 15 CERTAUTH among them.
                   std::vector<ZKRCertInfo> certs;
                   for (int i = 0; i < 30; ++i)
                     certs.push_back(zkr_mk_cert("CERT" + std::to_string(i),
                                                 (i % 2 == 0) ? "CERTAUTH" : "PERSONAL"));

                   bool more = false;
                   const std::vector<ZKRCertInfo> out = zkr_filter_certs(certs, "", "CERTAUTH", 10, &more);
                   Expect(out.size()).ToBe(static_cast<size_t>(10));
                   Expect(more).ToBe(true);
                   for (size_t i = 0; i < out.size(); ++i)
                     Expect(out[i].usage).ToBe("CERTAUTH");
                 });

              it("--max-entries 0 returns all matches",
                 []() -> void
                 {
                   std::vector<ZKRCertInfo> certs;
                   for (int i = 0; i < 30; ++i)
                     certs.push_back(zkr_mk_cert("CERT" + std::to_string(i),
                                                 (i % 2 == 0) ? "CERTAUTH" : "PERSONAL"));

                   bool more = true;
                   Expect(zkr_filter_certs(certs, "", "CERTAUTH", 0, &more).size()).ToBe(static_cast<size_t>(15));
                   Expect(more).ToBe(false);
                 });

              it("without filters the cap applies to raw entries",
                 []() -> void
                 {
                   std::vector<ZKRCertInfo> certs;
                   for (int i = 0; i < 12; ++i)
                     certs.push_back(zkr_mk_cert("CERT" + std::to_string(i), "PERSONAL"));

                   bool more = false;
                   Expect(zkr_filter_certs(certs, "", "", 10, &more).size()).ToBe(static_cast<size_t>(10));
                   Expect(more).ToBe(true);
                 });

              it("label and usage filters combine with AND",
                 []() -> void
                 {
                   std::vector<ZKRCertInfo> certs;
                   certs.push_back(zkr_mk_cert("CERT1", "PERSONAL"));
                   certs.push_back(zkr_mk_cert("CERT1", "CERTAUTH"));
                   certs.push_back(zkr_mk_cert("CERT2", "CERTAUTH"));

                   const std::vector<ZKRCertInfo> out = zkr_filter_certs(certs, "CERT1", "CERTAUTH", 0, nullptr);
                   Expect(out.size()).ToBe(static_cast<size_t>(1));
                   Expect(out[0].label).ToBe("CERT1");
                   Expect(out[0].usage).ToBe("CERTAUTH");
                 });
            });

        // ---------------------------------------------------------------------
        // Service-layer linkage + diagnostics. The probe both proves the binary
        // links IRRSDL64/GSKCMS and determines whether this user may create key
        // rings; `can_mutate` gates the lifecycle suites below.
        // ---------------------------------------------------------------------
        bool can_mutate = false;
        std::string probe_note;

        describe(
            "service diagnostics and linkage (no authority required)",
            [&]() -> void
            {
              it("links R_datalib and probes key ring create authority",
                 [&]() -> void
                 {
                   ZKR z{};
                   const std::string ring = "ZKRUT.PROBE." + zkr_unique();
                   int rc = zkr_new_ring(&z, owner, ring);
                   if (rc == 0)
                   {
                     can_mutate = true;
                     // If we can create, we must be able to clean up.
                     ExpectWithContext(zkr_del_ring(&z, owner, ring), z.diag.e_msg).ToBe(0);
                   }
                   else
                   {
                     // Not authorized (or otherwise unavailable): the failure must be
                     // a clean SAF diagnostic (>= 8), not a crash or empty message.
                     probe_note = z.diag.e_msg;
                     Expect(z.diag.e_msg.empty()).ToBe(false);
                     Expect(z.diag.saf_rc).ToBeGreaterThanOrEqualTo(8);
                   }
                   TestLog(can_mutate ? "key ring authority: AVAILABLE"
                                      : "key ring authority: DENIED (" + probe_note + ")");
                 });

              it("reports a clean diagnostic for a missing key ring",
                 [&]() -> void
                 {
                   ZKR z{};
                   std::vector<ZKRCertInfo> certs;
                   const std::string ring = "ZKRUT.NOPE." + zkr_unique();
                   int rc = zkr_list_ring(&z, owner, ring, certs);
                   Expect(rc).Not().ToBe(0);
                   Expect(z.diag.e_msg.empty()).ToBe(false);
                   // The failing service is the DataGetFirst (GETCERT) enumeration.
                   Expect(static_cast<int>(z.diag.function_code)).ToBe(static_cast<int>(ZKR_GETCERT_CODE));
                 });

              it("reads the caller's virtual key ring without crashing",
                 [&]() -> void
                 {
                   ZKR z{};
                   std::vector<ZKRCertInfo> certs;
                   // "*" is the virtual ring (all of the caller's certificates). It may
                   // succeed (possibly empty) or be denied; either way it must return a
                   // coherent result with a populated diagnostic on failure.
                   int rc = zkr_list_ring(&z, owner, "*", certs);
                   if (rc != 0)
                     Expect(z.diag.e_msg.empty()).ToBe(false);
                 });
            });

        // ---------------------------------------------------------------------
        // Key ring lifecycle. Requires authority to create key rings; skipped
        // (not failed) otherwise.
        // ---------------------------------------------------------------------
        describe(
            "key ring lifecycle (requires create authority)",
            [&]() -> void
            {
              itif(
                  "creates a key ring, lists it empty, enumerates it, and deletes it",
                  [&]() -> void
                  {
                    ZKR z{};
                    const std::string ring = "ZKRUT.LIFE." + zkr_unique();
                    ExpectWithContext(zkr_new_ring(&z, owner, ring), z.diag.e_msg).ToBe(0);

                    // A brand-new ring has no certificates (empty list, not an error).
                    std::vector<ZKRCertInfo> certs;
                    ExpectWithContext(zkr_list_ring(&z, owner, ring, certs), z.diag.e_msg).ToBe(0);
                    Expect(certs.size()).ToBe(static_cast<size_t>(0));

                    // GetRingInfo enumeration should succeed for the owner's rings.
                    std::vector<ZKRRingEntry> rings;
                    ExpectWithContext(zkr_list_rings(&z, owner, ring, rings), z.diag.e_msg).ToBe(0);

                    ExpectWithContext(zkr_del_ring(&z, owner, ring), z.diag.e_msg).ToBe(0);
                  },
                  can_mutate);

              itif(
                  "deleting a nonexistent key ring reports a clean error",
                  [&]() -> void
                  {
                    ZKR z{};
                    const std::string ring = "ZKRUT.GHOST." + zkr_unique();
                    int rc = zkr_del_ring(&z, owner, ring);
                    Expect(rc).Not().ToBe(0);
                    Expect(z.diag.e_msg.empty()).ToBe(false);
                  },
                  can_mutate);
            });

        // ---------------------------------------------------------------------
        // Certificate lifecycle. Requires create authority AND a PKCS#12 fixture
        // supplied out-of-band (ZKR_TEST_P12 / ZKR_TEST_P12_PASS) so no key
        // material is committed to the repo. Skipped when either is absent.
        // Deep PEM/PKCS#12 round-trip parity is covered by the JS integration
        // tests (testCertParity.js / testCertRpc.js); this asserts the new
        // service entry points wire up end-to-end.
        // ---------------------------------------------------------------------
        describe(
            "certificate lifecycle (requires authority + a PKCS#12 fixture)",
            [&]() -> void
            {
              const char *p12env = std::getenv("ZKR_TEST_P12");
              const std::string p12 = p12env != nullptr ? p12env : "";
              const char *passenv = std::getenv("ZKR_TEST_P12_PASS");
              const std::string p12pass = passenv != nullptr ? passenv : "password";
              const bool have_fixture = !p12.empty() && access(p12.c_str(), R_OK) == 0;
              const bool run_cert = can_mutate && have_fixture;

              // Cleanup registers: populated up front so a mid-flow failure/timeout
              // still tears down the scratch rings and any stray DB certificate.
              static std::vector<std::string> cleanup_rings;
              static std::vector<std::string> cleanup_labels;
              afterAll(
                  [&]() -> void
                  {
                    for (const auto &l : cleanup_labels)
                      zkr_try_purge_cert(owner, l);
                    for (const auto &r : cleanup_rings)
                      zkr_try_del_ring(owner, r);
                    cleanup_rings.clear();
                    cleanup_labels.clear();
                  });

              // The full sweep issues several DIGTCERT refreshes (import/delete),
              // which are slow; the 10s default is not enough.
              TEST_OPTIONS cert_opts = {false, 60};

              itif(
                  "imports, shows, exports, alters, connects, and deletes a certificate",
                  [&]() -> void
                  {
                    const std::string ring1 = "ZKRUT.CRT1." + zkr_unique();
                    const std::string ring2 = "ZKRUT.CRT2." + zkr_unique();
                    const std::string label = "ZKRUT" + zkr_unique();
                    cleanup_rings.push_back(ring1);
                    cleanup_rings.push_back(ring2);
                    cleanup_labels.push_back(label);

                    ZKR z{};
                    ExpectWithContext(zkr_new_ring(&z, owner, ring1), z.diag.e_msg).ToBe(0);
                    ExpectWithContext(zkr_new_ring(&z, owner, ring2), z.diag.e_msg).ToBe(0);

                    ZKRImportOptions imp;
                    imp.owner = owner;
                    imp.ring = ring1;
                    imp.label = label;
                    imp.usage = "PERSONAL";
                    imp.p12_path = p12;
                    imp.password = p12pass;
                    ExpectWithContext(zkr_import_cert(&z, imp), z.diag.e_msg).ToBe(0);

                    // The RACF DB may already hold this cert content under an earlier
                    // label; in that case its existing record/label is connected. Use
                    // whatever label is actually on the ring for the rest of the flow.
                    std::vector<ZKRCertInfo> certs;
                    ExpectWithContext(zkr_list_ring(&z, owner, ring1, certs), z.diag.e_msg).ToBe(0);
                    Expect(certs.size()).ToBeGreaterThanOrEqualTo(static_cast<size_t>(1));
                    const std::string real_label = certs[0].label;

                    // RACDCERT LABEL parity through the CLI: an exact --label
                    // filter finds the certificate; a case-mangled one does not
                    // (both succeed -- an empty result is not an error).
                    std::string flt_out;
                    int flt_rc = execute_command_with_output(
                        zowex_command + " system keyring list " + owner + " " + ring1 + " --label " + real_label,
                        flt_out);
                    ExpectWithContext(flt_rc, flt_out).ToBe(0);
                    Expect(flt_out).ToContain(real_label);

                    std::string lower_label = real_label;
                    for (size_t li = 0; li < lower_label.size(); ++li)
                      lower_label[li] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower_label[li])));
                    if (lower_label != real_label)
                    {
                      std::string miss_out;
                      int miss_rc = execute_command_with_output(
                          zowex_command + " system keyring list " + owner + " " + ring1 + " --label " + lower_label,
                          miss_out);
                      ExpectWithContext(miss_rc, miss_out).ToBe(0);
                      Expect(miss_out).Not().ToContain("Certificate: ");
                    }

                    // show: R_datalib fields + GSK-decoded serial/validity.
                    ZKRCertDetail detail;
                    ExpectWithContext(zkr_show_cert(&z, owner, ring1, real_label, detail), z.diag.e_msg).ToBe(0);
                    Expect(detail.label).ToBe(real_label);

                    // export (PEM): zkr returns EBCDIC PEM text (CLI/file parity).
                    ZKRExportOptions ex;
                    ex.owner = owner;
                    ex.ring = ring1;
                    ex.label = real_label;
                    ex.format = "pem";
                    std::string data;
                    ExpectWithContext(zkr_export_cert(&z, ex, data), z.diag.e_msg).ToBe(0);
                    Expect(data).ToContain("-----BEGIN CERTIFICATE-----");

                    // set-default on ring1.
                    ExpectWithContext(zkr_set_default(&z, owner, ring1, real_label), z.diag.e_msg).ToBe(0);

                    // connect the same certificate onto ring2, reading its bytes from ring1.
                    ZKRConnectOptions conn;
                    conn.owner = owner;
                    conn.ring = ring2;
                    conn.from_ring = ring1;
                    conn.label = real_label;
                    conn.usage = "PERSONAL";
                    ExpectWithContext(zkr_connect_cert(&z, conn), z.diag.e_msg).ToBe(0);

                    // trust: flip status via DataAlter (operates on the DB record).
                    ZKRAlterOptions alt;
                    alt.owner = owner;
                    alt.label = real_label;
                    alt.status = "NOTRUST";
                    ExpectWithContext(zkr_alter_cert(&z, alt), z.diag.e_msg).ToBe(0);
                    alt.status = "TRUST";
                    ExpectWithContext(zkr_alter_cert(&z, alt), z.diag.e_msg).ToBe(0);

                    // rename to a temporary label and back, so cleanup by real_label works.
                    ZKRAlterOptions ren;
                    ren.owner = owner;
                    ren.label = real_label;
                    ren.new_label = "ZKRUTREN" + zkr_unique();
                    ExpectWithContext(zkr_alter_cert(&z, ren), z.diag.e_msg).ToBe(0);
                    ZKRAlterOptions ren_back;
                    ren_back.owner = owner;
                    ren_back.label = ren.new_label;
                    ren_back.new_label = real_label;
                    ExpectWithContext(zkr_alter_cert(&z, ren_back), z.diag.e_msg).ToBe(0);

                    // Record the connected label so afterAll purges it too, then
                    // remove the certificate from the DB (and every ring).
                    cleanup_labels.push_back(real_label);
                    ExpectWithContext(zkr_del_cert(&z, owner, "*", real_label, /*skip_refresh*/ false), z.diag.e_msg)
                        .ToBe(0);
                  },
                  cert_opts, run_cert);
            });
      });
}

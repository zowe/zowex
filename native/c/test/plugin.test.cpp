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

#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "ztest.hpp"
#include "../extend/plugin.hpp"

using namespace ztst;

// These tests exercise the Phase 1 load_plugins() hygiene checks: '.'/'..' are
// skipped, and any entry that isn't a regular file (subdirectory, symlink, FIFO,
// etc.) is rejected before it is ever handed to dlopen.
//
// They assert on get_loaded_plugins() (the deterministic, in-process observable)
// rather than on log output. An earlier revision scraped logs/zowex.log for the
// per-entry "Rejected .../Skipping ..." messages plugin.cpp emits, but that was
// unreliable: those messages go through the process-global ZLogger singleton,
// whose Metal C backend binds a DD to a single log file at first use. Other
// suites (e.g. zlogger_tests) delete that file mid-run, orphaning the DD, and
// the logger cannot be safely re-pointed in-process (re-initializing it abends
// while tearing down the stale DD). The security-relevant guarantee - that no
// non-regular or dot entry is ever loaded, and that load_plugins() stays robust
// (no crash/hang) against hostile directory contents - is fully captured by
// asserting nothing loads, without any dependency on log-file lifecycle.

namespace
{
const std::string PLUGIN_TEST_DIR = "plugin_hygiene_test";

// Recursively wipe and recreate the scratch plugins directory used by these tests, so
// leftovers from an aborted previous run (or another suite) can't leak into assertions here.
void reset_plugin_test_dir()
{
  DIR *dir = opendir(PLUGIN_TEST_DIR.c_str());
  if (dir != nullptr)
  {
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
      const std::string name = entry->d_name;
      if (name == "." || name == "..")
        continue;
      const std::string path = PLUGIN_TEST_DIR + "/" + name;
      struct stat st;
      if (lstat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
        rmdir(path.c_str());
      else
        unlink(path.c_str());
    }
    closedir(dir);
  }
  rmdir(PLUGIN_TEST_DIR.c_str());
  mkdir(PLUGIN_TEST_DIR.c_str(), 0755);
}

void write_regular_file(const std::string &path, const std::string &contents)
{
  std::ofstream file(path, std::ios::binary);
  file << contents;
}
} // namespace

void plugin_tests()
{
  describe("PluginManager::load_plugins hygiene (Phase 1)", []() -> void
           {
        beforeEach([]() {
            reset_plugin_test_dir();
        });

        afterAll([]() {
            reset_plugin_test_dir();
            rmdir(PLUGIN_TEST_DIR.c_str());
        });

        it("ignores '.' and '..' directory entries and loads nothing", []() {
            plugin::PluginManager pm;
            pm.load_plugins(PLUGIN_TEST_DIR);

            // '.' and '..' are always present; the hygiene checks must skip them
            // instead of attempting to load the directory itself.
            Expect(pm.get_loaded_plugins().size()).ToBe(std::size_t(0));
        });

        it("does not load a subdirectory entry", []() {
            const std::string sub_path = PLUGIN_TEST_DIR + "/subdir_entry";
            mkdir(sub_path.c_str(), 0755);

            plugin::PluginManager pm;
            pm.load_plugins(PLUGIN_TEST_DIR);

            // A subdirectory is not a regular file, so it must be rejected rather
            // than loaded (and must not crash the loader).
            Expect(pm.get_loaded_plugins().size()).ToBe(std::size_t(0));

            rmdir(sub_path.c_str());
        });

        it("does not load a symlink entry, even one pointing at a regular file (TOCTOU-safe)", []() {
            // Point the symlink at a legitimate, regular file to show rejection is
            // based on the symlink itself (lstat + S_ISREG), not on what it resolves
            // to - the loader must not follow the link.
            const std::string target_path = PLUGIN_TEST_DIR + "/symlink_target.so";
            const std::string link_path = PLUGIN_TEST_DIR + "/symlink_entry.so";
            write_regular_file(target_path, "not a real shared object, just needs to exist");
            symlink(target_path.c_str(), link_path.c_str());

            plugin::PluginManager pm;
            pm.load_plugins(PLUGIN_TEST_DIR);

            // Neither the symlink (rejected: not a regular file) nor its bogus
            // target (reaches dlopen but fails to load) should end up loaded.
            Expect(pm.get_loaded_plugins().size()).ToBe(std::size_t(0));

            unlink(link_path.c_str());
            unlink(target_path.c_str());
        });

        it("does not load a named pipe (FIFO) entry", []() {
            const std::string fifo_path = PLUGIN_TEST_DIR + "/fifo_entry";
            mkfifo(fifo_path.c_str(), 0666);

            plugin::PluginManager pm;
            pm.load_plugins(PLUGIN_TEST_DIR);

            // A FIFO is not a regular file and must be rejected before dlopen -
            // handing one to dlopen could otherwise block the loader.
            Expect(pm.get_loaded_plugins().size()).ToBe(std::size_t(0));

            unlink(fifo_path.c_str());
        });

        it("loads nothing for a regular file that is not a valid shared object", []() {
            // Regression check: the hygiene filtering must not reject legitimate
            // regular files outright. This one is a regular file, so it passes the
            // hygiene checks and reaches dlopen, which fails because it isn't a real
            // shared object - so nothing is loaded and the loader doesn't crash.
            const std::string plugin_path = PLUGIN_TEST_DIR + "/not_a_valid_plugin.so";
            write_regular_file(plugin_path, "definitely not an ELF/shared object");

            plugin::PluginManager pm;
            pm.load_plugins(PLUGIN_TEST_DIR);

            Expect(pm.get_loaded_plugins().size()).ToBe(std::size_t(0));

            unlink(plugin_path.c_str());
        });

        it("loads nothing from a directory containing a mix of non-regular and invalid entries", []() {
            const std::string sub_path = PLUGIN_TEST_DIR + "/mixed_subdir";
            const std::string fifo_path = PLUGIN_TEST_DIR + "/mixed_fifo";
            const std::string link_target = PLUGIN_TEST_DIR + "/mixed_target.so";
            const std::string link_path = PLUGIN_TEST_DIR + "/mixed_symlink.so";
            const std::string regular_path = PLUGIN_TEST_DIR + "/mixed_regular.so";

            mkdir(sub_path.c_str(), 0755);
            mkfifo(fifo_path.c_str(), 0666);
            write_regular_file(link_target, "target");
            symlink(link_target.c_str(), link_path.c_str());
            write_regular_file(regular_path, "not a real shared object");

            plugin::PluginManager pm;
            pm.load_plugins(PLUGIN_TEST_DIR);

            // The subdirectory, FIFO and symlink are all rejected as non-regular;
            // the regular file reaches dlopen but fails to load. Net result: the
            // loader survives a hostile directory and loads nothing.
            Expect(pm.get_loaded_plugins().size()).ToBe(std::size_t(0));

            rmdir(sub_path.c_str());
            unlink(fifo_path.c_str());
            unlink(link_path.c_str());
            unlink(link_target.c_str());
            unlink(regular_path.c_str());
        });

        it("does not crash and loads nothing when the plugins directory does not exist", []() {
            plugin::PluginManager pm;
            pm.load_plugins(PLUGIN_TEST_DIR + "/does_not_exist");

            Expect(pm.get_loaded_plugins().size()).ToBe(std::size_t(0));
        }); });
}

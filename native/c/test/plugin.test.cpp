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
#include "../zlogger.hpp"

using namespace ztst;

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

std::string plugin_test_read_log()
{
  std::ifstream file("logs/zowex.log");
  if (!file.is_open())
    return "";
  return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
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
            ZLogger::get_instance().set_log_level(ZLOGLEVEL_DEBUG);
        });

        afterAll([]() {
            reset_plugin_test_dir();
            rmdir(PLUGIN_TEST_DIR.c_str());
        });

        it("does not attempt to dlopen '.' or '..' directory entries", []() {
            plugin::PluginManager pm;
            pm.load_plugins(PLUGIN_TEST_DIR);
            usleep(1000);

            Expect(pm.get_loaded_plugins().size()).ToBe(std::size_t(0));

            const std::string log = plugin_test_read_log();
            Expect(log).ToContain("Skipping plugin directory entry: .");
            Expect(log).ToContain("Skipping plugin directory entry: ..");
            Expect(log).Not().ToContain("Failed to open handle to plugin located at: " + PLUGIN_TEST_DIR + "/.");
            Expect(log).Not().ToContain("Failed to open handle to plugin located at: " + PLUGIN_TEST_DIR + "/..");
        });

        it("rejects a subdirectory entry as not a regular file, without dlopen", []() {
            const std::string sub_path = PLUGIN_TEST_DIR + "/subdir_entry";
            mkdir(sub_path.c_str(), 0755);

            plugin::PluginManager pm;
            pm.load_plugins(PLUGIN_TEST_DIR);
            usleep(1000);

            Expect(pm.get_loaded_plugins().size()).ToBe(std::size_t(0));

            const std::string log = plugin_test_read_log();
            Expect(log).ToContain("Rejected plugin entry " + sub_path + ": not a regular file");
            Expect(log).Not().ToContain("Failed to open handle to plugin located at: " + sub_path);

            rmdir(sub_path.c_str());
        });

        it("rejects a symlink entry without following it (TOCTOU-safe)", []() {
            // Point the symlink at a legitimate, otherwise-loadable-looking regular file to
            // prove rejection is based on the symlink itself, not on what it resolves to.
            const std::string target_path = PLUGIN_TEST_DIR + "/symlink_target.so";
            const std::string link_path = PLUGIN_TEST_DIR + "/symlink_entry.so";
            write_regular_file(target_path, "not a real shared object, just needs to exist");
            symlink(target_path.c_str(), link_path.c_str());

            plugin::PluginManager pm;
            pm.load_plugins(PLUGIN_TEST_DIR);
            usleep(1000);

            Expect(pm.get_loaded_plugins().size()).ToBe(std::size_t(0));

            const std::string log = plugin_test_read_log();
            Expect(log).ToContain("Rejected plugin entry " + link_path + ": not a regular file");
            Expect(log).Not().ToContain("Failed to open handle to plugin located at: " + link_path);

            unlink(link_path.c_str());
            unlink(target_path.c_str());
        });

        it("rejects a named pipe (FIFO) entry as not a regular file", []() {
            const std::string fifo_path = PLUGIN_TEST_DIR + "/fifo_entry";
            mkfifo(fifo_path.c_str(), 0666);

            plugin::PluginManager pm;
            pm.load_plugins(PLUGIN_TEST_DIR);
            usleep(1000);

            Expect(pm.get_loaded_plugins().size()).ToBe(std::size_t(0));

            const std::string log = plugin_test_read_log();
            Expect(log).ToContain("Rejected plugin entry " + fifo_path + ": not a regular file");
            Expect(log).Not().ToContain("Failed to open handle to plugin located at: " + fifo_path);

            unlink(fifo_path.c_str());
        });

        it("still attempts dlopen for a regular file, unaffected by the hygiene checks", []() {
            // Regression check: the Phase 1 filtering must not reject legitimate regular files.
            // This one isn't a real shared object, so it should reach and fail at the existing
            // dlopen step with the pre-existing error message, not a "not a regular file" rejection.
            const std::string plugin_path = PLUGIN_TEST_DIR + "/not_a_valid_plugin.so";
            write_regular_file(plugin_path, "definitely not an ELF/shared object");

            plugin::PluginManager pm;
            pm.load_plugins(PLUGIN_TEST_DIR);
            usleep(1000);

            Expect(pm.get_loaded_plugins().size()).ToBe(std::size_t(0));

            const std::string log = plugin_test_read_log();
            Expect(log).ToContain("Failed to open handle to plugin located at: " + plugin_path);
            Expect(log).Not().ToContain("Rejected plugin entry " + plugin_path + ": not a regular file");

            unlink(plugin_path.c_str());
        });

        it("rejects every non-regular entry in a mixed directory while still trying the regular one", []() {
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
            usleep(1000);

            Expect(pm.get_loaded_plugins().size()).ToBe(std::size_t(0));

            const std::string log = plugin_test_read_log();
            Expect(log).ToContain("Rejected plugin entry " + sub_path + ": not a regular file");
            Expect(log).ToContain("Rejected plugin entry " + fifo_path + ": not a regular file");
            Expect(log).ToContain("Rejected plugin entry " + link_path + ": not a regular file");
            Expect(log).ToContain("Failed to open handle to plugin located at: " + regular_path);

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

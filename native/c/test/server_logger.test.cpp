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
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "ztest.hpp"
#include "../server/logger.hpp"

using namespace ztst;

namespace
{

bool file_exists(const std::string &path)
{
  struct stat buffer;
  return stat(path.c_str(), &buffer) == 0;
}

long file_size(const std::string &path)
{
  struct stat buffer;
  if (stat(path.c_str(), &buffer) != 0)
  {
    return -1;
  }
  return static_cast<long>(buffer.st_size);
}

std::string read_file_contents(const std::string &path)
{
  std::ifstream file(path);
  if (!file.is_open())
  {
    return "";
  }
  return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void cleanup_test_dir(const std::string &dir)
{
  const std::string log_path = dir + "/zowex_server.log";
  unlink(log_path.c_str());
  for (int i = 1; i <= 9; i++)
  {
    unlink((log_path + "." + std::to_string(i)).c_str());
  }
  rmdir(dir.c_str());
}

// Temporarily overrides ZOWEX_LOGS_DIR for the duration of a test, restoring
// the previous value (set by ztest_runner.cpp) on destruction.
class ScopedLogsDirOverride
{
public:
  explicit ScopedLogsDirOverride(const std::string &dir)
  {
    const char *previous = std::getenv("ZOWEX_LOGS_DIR");
    if (previous)
    {
      had_previous = true;
      previous_value = previous;
    }
    setenv("ZOWEX_LOGS_DIR", dir.c_str(), 1);
  }

  ~ScopedLogsDirOverride()
  {
    if (had_previous)
    {
      setenv("ZOWEX_LOGS_DIR", previous_value.c_str(), 1);
    }
    else
    {
      unsetenv("ZOWEX_LOGS_DIR");
    }
  }

private:
  bool had_previous = false;
  std::string previous_value;
};

} // namespace

void server_logger_tests()
{
  describe("server::Logger ZOWEX_LOGS_DIR tests", []() -> void
           {
        it("should write logs to the directory specified by ZOWEX_LOGS_DIR", []() {
            const std::string test_dir = "logs/server_logger_env_test";
            const std::string log_path = test_dir + "/zowex_server.log";

            ScopedLogsDirOverride override_dir(test_dir);

            server::Logger::init_logger(false, true);
            LOG_INFO("Message written for ZOWEX_LOGS_DIR test");
            server::Logger::shutdown();

            Expect(file_exists(log_path)).ToBe(true);

            const std::string contents = read_file_contents(log_path);
            Expect(contents).ToContain("Message written for ZOWEX_LOGS_DIR test");

            cleanup_test_dir(test_dir);
        }); });

  describe("server::Logger log rolling tests", []() -> void
           {
        it("should roll the active log file once it exceeds 100KB", []() {
            const std::string test_dir = "logs/server_logger_rolling_test";
            const std::string log_path = test_dir + "/zowex_server.log";

            ScopedLogsDirOverride override_dir(test_dir);

            server::Logger::init_logger(false, true);

            // Each message is padded so a modest number of writes pushes the
            // log file past the 100KB-per-file rotation threshold.
            const std::string padding(1024, 'x');
            for (int i = 0; i < 150; i++)
            {
              LOG_ERROR("Simulated failure #%d: %s", i, padding.c_str());
            }

            server::Logger::shutdown();

            // The active file should have rolled at least once, leaving it
            // small, with the overflow preserved in the first backup slot.
            Expect(file_exists(log_path)).ToBe(true);
            Expect(file_size(log_path)).ToBeLessThan(100L * 1024L);
            Expect(file_exists(log_path + ".1")).ToBe(true);

            const std::string backup_contents = read_file_contents(log_path + ".1");
            Expect(backup_contents).ToContain("Simulated failure #0:");

            cleanup_test_dir(test_dir);
        });

        it("should retain at most 10 generations via FIFO rotation, dropping the oldest", []() {
            const std::string test_dir = "logs/server_logger_fifo_test";
            const std::string log_path = test_dir + "/zowex_server.log";

            ScopedLogsDirOverride override_dir(test_dir);

            server::Logger::init_logger(false, true);

            // Write far more than the full 10-file/100KB-per-file budget (~1MB)
            // so several rotations occur, forcing the oldest backups to be
            // evicted rather than accumulating indefinitely.
            const std::string padding(2048, 'x');
            const int message_count = 1500;
            for (int i = 0; i < message_count; i++)
            {
              LOG_ERROR("Simulated failure #%d: %s", i, padding.c_str());
            }

            server::Logger::shutdown();

            // Active file plus backups .1 through .9 should all exist and stay
            // bounded (never allowed to grow past a single generation's worth)
            Expect(file_exists(log_path)).ToBe(true);
            Expect(file_size(log_path)).ToBeLessThan(150L * 1024L);
            for (int i = 1; i <= 9; i++)
            {
              const std::string backup_path = log_path + "." + std::to_string(i);
              Expect(file_exists(backup_path)).ToBe(true);
              Expect(file_size(backup_path)).ToBeLessThan(150L * 1024L);
            }

            // No 10th backup should ever be created - FIFO caps total files at 10
            Expect(file_exists(log_path + ".10")).ToBe(false);

            // The very first messages should have been evicted from every
            // retained file, while the most recent message should survive
            std::string all_contents = read_file_contents(log_path);
            for (int i = 1; i <= 9; i++)
            {
              all_contents += read_file_contents(log_path + "." + std::to_string(i));
            }

            Expect(all_contents).Not().ToContain("Simulated failure #0:");
            char last_message[64];
            snprintf(last_message, sizeof(last_message), "Simulated failure #%d:", message_count - 1);
            Expect(all_contents).ToContain(std::string(last_message));

            cleanup_test_dir(test_dir);
        }); });
}

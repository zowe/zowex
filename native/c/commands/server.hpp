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

#ifndef COMMANDS_SERVER_HPP
#define COMMANDS_SERVER_HPP

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include "../parser.hpp"

class WorkerPool;

namespace server
{

struct Options
{
  long long num_workers = 10;
  bool verbose = false;
  long long request_timeout = 60;
  std::string exec_dir = ".";
};

static std::string get_overrides_dir(const std::string &exec_dir)
{
  std::string overrides_path = std::string(getenv("ZOWEX_OVERRIDES_DIR"));
  if (overrides_path.empty())
  {
    return exec_dir + "/overrides";
  }
  else if (overrides_path.back() == '/')
  {
    overrides_path.pop_back(); // Remove trailing slash if present
  }

  return overrides_path;
}

void register_commands(parser::Command &root_command);

} // namespace server

class ZServer
{
private:
  server::Options options;
  std::string exec_dir = ".";
  std::unique_ptr<WorkerPool> worker_pool;
  std::atomic<bool> shutdown_requested{false};
  std::once_flag shutdown_flag;

  static void signal_handler(int sig);
  void setup_signal_handlers();
  void request_shutdown();
  std::map<std::string, std::string> load_checksums();
  void print_ready_message();
  void log_worker_count();

  ZServer() = default;

public:
  ZServer(const ZServer &) = delete;
  ZServer &operator=(const ZServer &) = delete;
  ~ZServer();

  static ZServer &get_instance();
  void set_exec_dir(const std::string &dir)
  {
    exec_dir = dir;
  }
  const std::string &get_exec_dir() const
  {
    return exec_dir;
  }
  void run(const server::Options &opts);
};

#endif

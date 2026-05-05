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

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
#include "core.hpp"
#include <vector>
#include "server.hpp"
#include "../zut.hpp"
#include "../ifaed.h"
#include "../zjson.hpp"
#include "../zusf.hpp"
#include "../server/rpc_server.hpp"
#include "../server/rpc_commands.hpp"
#include "../server/dispatcher.hpp"
#include "../server/logger.hpp"
#include "../server/worker.hpp"
#include "../server/plugin_bridge.hpp"

using namespace parser;

struct StatusMessage
{
  std::string status;
  std::string message;
  std::optional<zjson::Value> data;
};
ZJSON_DERIVE(StatusMessage, status, message, data);

// ZServer implementation

ZServer::~ZServer() = default;

ZServer &ZServer::get_instance()
{
  static ZServer instance;
  return instance;
}

void ZServer::signal_handler(int sig __attribute__((unused)))
{
  get_instance().request_shutdown();
}

void ZServer::setup_signal_handlers()
{
  signal(SIGHUP, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGQUIT, signal_handler);
  signal(SIGABRT, signal_handler);
  signal(SIGTERM, signal_handler);
}

void ZServer::request_shutdown()
{
  std::call_once(shutdown_flag, [this]()
                 {
          shutdown_requested = true;
          if (worker_pool) {
              worker_pool->shutdown();
          }
          close(STDIN_FILENO); });
}

std::map<std::string, std::string> ZServer::load_checksums()
{
  std::map<std::string, std::string> checksums;
  ZUSF zusf = {.encoding_opts = {.source_codepage = "IBM-1047", .data_type = eDataTypeText}};
  std::string checksums_file = options.exec_dir + "/checksums.asc";
  std::string checksums_content;

  int rc = zusf_read_from_uss_file(&zusf, checksums_file, checksums_content);
  if (rc != 0)
  {
    LOG_DEBUG("Failed to read checksums file: %s (expected for dev builds)", checksums_file.c_str());
    return checksums;
  }

  std::istringstream infile(checksums_content);
  std::string line{};
  std::string checksum{};
  std::string filename{};
  while (std::getline(infile, line))
  {
    std::istringstream iss(line);
    if (iss >> checksum >> filename)
    {
      checksums[filename] = checksum;
    }
  }

  return checksums;
}

void ZServer::print_ready_message()
{
  zjson::Value data = zjson::Value::create_object();
  const auto checksums = load_checksums();
  zjson::Value checksums_obj = zjson::Value::create_object();
  for (const auto &pair : checksums)
  {
    checksums_obj.add_to_object(pair.first, zjson::Value(pair.second));
  }
  data.add_to_object("checksums", checksums.empty() ? zjson::Value() : checksums_obj);
  data.add_to_object("version", zjson::Value(core::get_version()));

  StatusMessage status_msg{
      .status = "ready",
      .message = "zowex server is ready to accept input",
      .data = std::optional<zjson::Value>(data),
  };

  std::string json_string = RpcServer::serialize_json(zjson::to_value(status_msg).value());
  std::cout << json_string << std::endl;
}

void ZServer::log_worker_count()
{
  if (!server::Logger::is_verbose_logging())
    return;

  std::thread([this]()
              {
          while (!shutdown_requested) {
              if (worker_pool) {
                  int32_t count = worker_pool->get_available_workers_count();
                  LOG_DEBUG("Available workers: %d/%lld", count, options.num_workers);
                  std::this_thread::sleep_for(std::chrono::milliseconds(500));
                  if (count == options.num_workers) {
                      break;
                  }
              }
              std::this_thread::sleep_for(std::chrono::milliseconds(100));
          } })
      .detach();
}

void ZServer::run(const server::Options &opts)
{
  options = opts;

  server::Logger::init_logger(options.exec_dir.c_str(), options.verbose);
  LOG_INFO("Starting zowex server with %lld workers and %lld seconds until request timeout (verbose=%s)", options.num_workers, options.request_timeout, options.verbose ? "true" : "false");

  setup_signal_handlers();

  CommandDispatcher &dispatcher = CommandDispatcher::get_instance();

  LOG_DEBUG("Registering command handlers");
  register_all_commands(dispatcher);
  plugin::register_commands_with_server(dispatcher);

  worker_pool.reset(new WorkerPool(options.num_workers, std::chrono::seconds(options.request_timeout)));

  std::atexit([]()
              { get_instance().request_shutdown(); });

  log_worker_count();
  print_ready_message();

  LOG_DEBUG("Entering main input processing loop");
  std::string line{};
  while (std::getline(std::cin, line) && !shutdown_requested)
  {
    if (!line.empty())
    {
      worker_pool->distribute_request(line);
    }
  }

  LOG_INFO("Input stream closed, shutting down");
  request_shutdown();

  server::Logger::shutdown();
}

// server namespace implementation

namespace server
{

static int handle_server(plugin::InvocationContext &context)
{
  server::Options opts;
  opts.num_workers = context.get<long long>("num-workers", opts.num_workers);
  opts.verbose = context.get<bool>("verbose", opts.verbose);
  opts.request_timeout = context.get<long long>("request-timeout", opts.request_timeout);
  opts.exec_dir = ZServer::get_instance().get_exec_dir();
  std::string client_name = context.get<std::string>("client-name", "ZRS VSCE");

  const auto *num_workers_env = getenv("ZOWEX_NUM_WORKERS");
  if (num_workers_env != nullptr)
  {
    try
    {
      std::size_t pos{};
      auto value = std::stoll(num_workers_env, &pos);
      if (pos != std::string(num_workers_env).size())
      {
        context.error_stream() << "Invalid value for num workers: '" << num_workers_env << "'" << std::endl;
        return 1;
      }
      opts.num_workers = value;
    }
    catch (const std::invalid_argument &)
    {
      context.error_stream() << "Invalid value for num workers: '" << num_workers_env << "'" << std::endl;
      return 1;
    }
    catch (const std::out_of_range &)
    {
      context.error_stream() << "Value out of range for num workers: '" << num_workers_env << "'" << std::endl;
      return 1;
    }
  }

  if (opts.num_workers <= 0)
  {
    context.error_stream() << "Number of workers must be greater than 0" << std::endl;
    return 1;
  }

  if (opts.request_timeout <= 0)
  {
    context.error_stream() << "Request timeout must be greater than 0 seconds" << std::endl;
    return 1;
  }

  if (!client_name.empty())
  {
    std::transform(client_name.begin(), client_name.end(), client_name.begin(), ::toupper);
  }

  auto is_valid_client_name = [](const std::string &client_name) -> bool
  {
    static const std::set<std::string> valid_clients = {
        "ZRS VSCE",
        "ZRS CLI",
        "ZRS MCP"};
    return valid_clients.find(client_name) != valid_clients.end();
  };

  if (!client_name.empty() && !is_valid_client_name(client_name))
  {
    static const std::set<std::string> valid_clients = {
        "ZRS VSCE",
        "ZRS CLI",
        "ZRS MCP"};

    std::cerr << "Error: Invalid client name '" << client_name << "', must be one of: ";
    bool first = true;
    for (const auto &valid_name : valid_clients)
    {
      if (!first)
        std::cerr << " | ";
      std::cerr << valid_name;
      first = false;
    }
    std::cerr << std::endl;
    return 1;
  }

  std::vector<IFAED_TOKEN> tokens;
  const auto exec_dir = ZServer::get_instance().get_exec_dir();
  zut_register_service(tokens, client_name, core::get_version(), server::get_overrides_dir(exec_dir));

  try
  {
    ZServer::get_instance().run(opts);
  }
  catch (const std::exception &e)
  {
    zut_deregister_service(tokens);
    std::cerr << "Fatal error: " << e.what() << std::endl;
    LOG_FATAL("Fatal error: %s", e.what());
    return 1;
  }

  zut_deregister_service(tokens);

  return 0;
}

void register_commands(Command &root_command)
{
  auto server_cmd = std::make_shared<Command>("server", "start the Zowe Remote SSH I/O server");
  server_cmd->add_keyword_arg("num-workers",
                              make_aliases("-w", "--num-workers"),
                              "number of worker threads",
                              ArgType_Single, false,
                              ArgValue(10LL));
  server_cmd->add_keyword_arg("verbose",
                              make_aliases("-v", "--verbose"),
                              "enable verbose logging",
                              ArgType_Flag, false,
                              ArgValue(false));
  server_cmd->add_keyword_arg("request-timeout",
                              make_aliases("-t", "--request-timeout"),
                              "request timeout in seconds before a worker is restarted",
                              ArgType_Single, false,
                              ArgValue(60LL));
  server_cmd->add_keyword_arg("client-name",
                              make_aliases("-c", "--client-name"),
                              "name of the client",
                              ArgType_Single, false);
  server_cmd->set_handler(handle_server);
  root_command.add_command(server_cmd);
}

} // namespace server

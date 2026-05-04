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

#pragma runopts("TRAP(ON,NOSPIE)")

#define _UNIX03_SOURCE
#include <dirent.h>
#include <iostream>
#include <string>
#include "commands/console.hpp"
#include "commands/core.hpp"
#include "commands/ds.hpp"
#include "commands/job.hpp"
#include "commands/server.hpp"
#include "commands/system.hpp"
#include "commands/tool.hpp"
#include "commands/tso.hpp"
#include "commands/uss.hpp"
#include "extend/plugin.hpp"
#include "zut.hpp"

// Version information
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

static std::string get_executable_dir(const char *argv0)
{
  std::string full_path(argv0);
  size_t last_slash = full_path.find_last_of('/');
  if (last_slash != std::string::npos)
    return full_path.substr(0, last_slash);
  return ".";
}

static std::string get_plugins_dir(const std::string &exec_dir)
{
  std::string plugins_path = std::string(getenv("ZOWEX_PLUGINS_DIR"));
  if (plugins_path.empty())
  {
    return exec_dir + "/plugins";
  }
  else if (plugins_path.back() == '/')
  {
    plugins_path.pop_back(); // Remove trailing slash if present
  }

  return plugins_path;
}

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

int main(int argc, char *argv[])
{
  const auto exec_dir = get_executable_dir(argv[0]);
  ZServer::get_instance().set_exec_dir(exec_dir);

  try
  {
    auto &root_cmd = core::setup_root_command(argv);
    core::set_version(PACKAGE_VERSION);

    plugin::PluginManager pm;
    core::set_plugin_manager(&pm);
    pm.load_plugins(get_plugins_dir(exec_dir));

    console::register_commands(root_cmd);
    ds::register_commands(root_cmd);
    job::register_commands(root_cmd);
    server::register_commands(root_cmd);
    sys::register_commands(root_cmd);
    tool::register_commands(root_cmd);
    tso::register_commands(root_cmd);
    uss::register_commands(root_cmd);

    pm.register_commands(root_cmd);

    // TODO(Kelosky): support other features, MCP, ZRS, CLI, etc.
    std::vector<IFAED_TOKEN> tokens;
    zut_register_service(tokens, "ZOWEX CLI", core::get_version(), get_overrides_dir(exec_dir));
    auto rc = core::execute_command(argc, argv);
    zut_deregister_service(tokens);
    return rc;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Fatal error encountered in zowex: " << e.what() << std::endl;
    return 1;
  }
}

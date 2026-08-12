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

#ifndef DISPATCHER_HPP
#define DISPATCHER_HPP

#include "../extend/plugin.hpp"
#include "../singleton.hpp"
#include "builder.hpp"
#include <string>
#include <vector>
#include <map>

// Forward declarations
struct RpcNotification;
class MiddlewareContext;

class CommandDispatcher : public Singleton<CommandDispatcher>
{
  friend class Singleton<CommandDispatcher>;

public:
  // CommandHandler type from plugin.hpp
  typedef plugin::CommandProviderImpl::CommandRegistrationContext::CommandHandler CommandHandler;

  // Register a new command using CommandBuilder
  bool register_command(const std::string &command_name, const CommandBuilder &builder);

  // Dispatch a command by name using the provided context
  int dispatch(const std::string &command_name, MiddlewareContext &context);

  // Check if a command is registered
  bool has_command(const std::string &command_name) const;

  // Get list of registered command names
  std::vector<std::string> get_registered_commands() const;

  // Remove a command registration
  bool unregister_command(const std::string &command_name);

  // Clear all registered commands
  void clear();

  // Get the builders map (for accessing validators)
  const std::map<std::string, CommandBuilder> &get_builders() const
  {
    return m_commands;
  }

private:
  // Private constructor for singleton
  CommandDispatcher() = default;

  std::map<std::string, CommandBuilder> m_commands;
};

#endif

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

#ifndef _UNIX03_SOURCE
#define _UNIX03_SOURCE
#endif
#include <dirent.h>
#include <dlfcn.h>
#include "plugin.hpp"
#include "../parser.hpp"

namespace plugin
{
class RegistrationContextImpl
    : public CommandProviderImpl::CommandRegistrationContext
{

public:
  explicit RegistrationContextImpl(parser::Command &root)
      : m_root(root), m_root_record(root)
  {
  }

  CommandHandle create_command(const char *name, const char *help)
  {
    std::string cmd_name = name ? name : "";
    std::string cmd_help = help ? help : "";

    parser::command_ptr command(new parser::Command(cmd_name, cmd_help));
    m_records.push_back(std::make_unique<CommandRecord>(command));
    return static_cast<CommandHandle>(m_records.back().get());
  }

  CommandHandle get_root_command()
  {
    return static_cast<CommandHandle>(&m_root_record);
  }

  void add_alias(CommandHandle command, const char *alias)
  {
    CommandRecord *record = to_record(command);
    if (record == nullptr || alias == nullptr)
      return;

    record->get().add_alias(alias);
  }

  void add_keyword_arg(CommandHandle command,
                       const char *name,
                       const char **aliases,
                       unsigned int alias_count,
                       const char *help,
                       ArgumentType type,
                       int required,
                       const DefaultValue *default_value)
  {
    CommandRecord *record = to_record(command);
    if (record == nullptr || name == nullptr)
      return;

    std::vector<std::string> alias_vector;
    if (aliases)
    {
      for (unsigned int i = 0; i < alias_count; ++i)
      {
        if (aliases[i])
          alias_vector.push_back(aliases[i]);
      }
    }

    parser::ArgValue defaultArg = convert_default(default_value);
    record->get().add_keyword_arg(std::string(name), alias_vector, std::string(help ? help : ""), convert_type(type), required != 0, defaultArg);
  }

  void add_positional_arg(CommandHandle command,
                          const char *name,
                          const char *help,
                          ArgumentType type,
                          int required,
                          const DefaultValue *default_value)
  {
    CommandRecord *record = to_record(command);
    if (record == nullptr || name == nullptr)
      return;

    parser::ArgValue default_arg = convert_default(default_value);

    record->get().add_positional_arg(std::string(name),
                                     help ? std::string(help) : std::string(),
                                     parser::ArgType_Single, required != 0,
                                     default_arg);
  }

  void set_handler(CommandHandle command, CommandHandler handler)
  {
    CommandRecord *record = to_record(command);
    if (record == nullptr)
      return;

    record->get().set_handler(handler);
  }

  void add_subcommand(CommandHandle parent, CommandHandle child)
  {
    CommandRecord *parent_record = to_record(parent);
    CommandRecord *child_record = to_record(child);
    if (parent_record == nullptr || child_record == nullptr)
      return;

    if (parent_record->is_root())
    {
      m_root.add_command(child_record->get_command_ptr());
    }
    else
    {
      parent_record->get()
          .add_command(child_record->get_command_ptr());
    }
  }

  void add_to_server(CommandHandle command)
  {
    CommandRecord *record = to_record(command);
    if (record == nullptr)
      return;

    m_server_commands.insert(record->get_command_ptr());
  }

  const std::set<parser::command_ptr> &get_server_commands() const
  {
    return m_server_commands;
  }

private:
  struct CommandRecord
  {
    parser::command_ptr pointer;
    parser::Command *raw;
    bool root;

    explicit CommandRecord(parser::Command &existing)
        : raw(&existing), root(true)
    {
    }

    explicit CommandRecord(const parser::command_ptr &cmd)
        : pointer(cmd), raw(cmd.get()), root(false)
    {
    }

    parser::Command &get()
    {
      return *raw;
    }

    const parser::command_ptr &get_command_ptr() const
    {
      return pointer;
    }

    bool is_root() const
    {
      return root;
    }
  };

  CommandRecord *to_record(CommandHandle handle)
  {
    return static_cast<CommandRecord *>(handle);
  }

  parser::ArgValue convert_default(const DefaultValue *value)
  {
    if (value == nullptr || value->kind == DefaultValue::ValueKind_None)
    {
      return parser::ArgValue();
    }

    switch (value->kind)
    {
    case DefaultValue::ValueKind_Bool:
      return parser::ArgValue(value->value.bool_value != 0);
    case DefaultValue::ValueKind_Int:
      return parser::ArgValue(static_cast<long long>(value->value.int_value));
    case DefaultValue::ValueKind_Double:
      return parser::ArgValue(value->value.double_value);
    case DefaultValue::ValueKind_String:
      if (value->value.string_value)
        return parser::ArgValue(std::string(value->value.string_value));
      return parser::ArgValue();
    default:
      return parser::ArgValue();
    }
  }

  parser::ArgType convert_type(ArgumentType type)
  {
    switch (type)
    {
    case ArgumentType_Flag:
      return parser::ArgType_Flag;
    case ArgumentType_Single:
      return parser::ArgType_Single;
    case ArgumentType_Multiple:
      return parser::ArgType_Multiple;
    }
    return parser::ArgType_Flag;
  }

  parser::Command &m_root;
  CommandRecord m_root_record;
  std::vector<std::unique_ptr<CommandRecord>> m_records;
  std::set<parser::command_ptr> m_server_commands;
};

void PluginManager::load_plugins(const std::string &plugins_path)
{
  m_plugins_path = plugins_path;
  auto *plugins_dir = opendir(m_plugins_path.c_str());
  if (plugins_dir != nullptr)
  {
    struct dirent *entry;
    void (*register_plugin)(plugin::PluginManager &);
    while ((entry = readdir(plugins_dir)) != nullptr)
    {
      std::string plugin_path = m_plugins_path + "/" + entry->d_name;
      void *plugin = dlopen(plugin_path.c_str(), RTLD_LAZY);
      if (plugin == nullptr)
      {
        ZLOG_ERROR("Failed to open handle to plugin located at: %s", plugin_path.c_str());
        continue;
      }

      *(void **)(&register_plugin) = dlsym(plugin, "register_plugin");
      if (register_plugin)
      {
        m_metadata_pending = false;
        m_pending_metadata = PluginMetadata();
        m_registration_rejected = false;
        m_duplicate_display_name.clear();
        m_provider_snapshot = m_command_providers.size();

        register_plugin(*this);

        if (m_registration_rejected)
        {
          ZLOG_ERROR("Plugin %s rejected because display name '%s' is already registered",
                     plugin_path.c_str(),
                     m_duplicate_display_name.c_str());
          discard_command_providers_from(m_provider_snapshot);
          dlclose(plugin);
          continue;
        }

        record_loaded_plugin(plugin, entry->d_name);
      }
      else
      {
        ZLOG_ERROR("Plugin %s loaded but no entrypoint available (register_plugin missing)", plugin_path.c_str());
        dlclose(plugin);
      }
    }
    closedir(plugins_dir);
  }
}

void PluginManager::unload_plugins()
{
  m_command_providers.clear();
  m_server_commands.clear();
  for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it)
  {
    if (it->handle)
    {
      dlclose(it->handle);
      it->handle = nullptr;
    }
  }
}

void PluginManager::record_loaded_plugin(void *plugin_handle, const std::string &plugin_identifier)
{
  PluginMetadata metadata;
  if (m_metadata_pending)
  {
    metadata = m_pending_metadata;
  }
  else
  {
    metadata.display_name = plugin_identifier;
    metadata.version = "";
  }

  metadata.filename = plugin_identifier;

  m_plugins.push_back(LoadedPlugin(metadata, plugin_handle));
  m_metadata_pending = false;
  m_pending_metadata = PluginMetadata();
}

void PluginManager::register_commands(parser::Command &rootCommand)
{
  RegistrationContextImpl context(rootCommand);

  for (const auto &factory : m_command_providers)
  {
    if (!factory)
      continue;

    auto provider = factory->create();
    if (!provider)
      continue;

    provider->register_commands(context);
  }

  for (const auto &command : context.get_server_commands())
  {
    m_server_commands.insert(command);
  }
}
} // namespace plugin

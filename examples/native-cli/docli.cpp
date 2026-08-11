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

#include <iostream>
#include <string>
#include <vector>
#include <memory> // For std::shared_ptr

#include "parser.hpp"

using namespace parser;
using namespace plugin;

int handle_awesome(InvocationContext &context)
{

  // std::cout << "context: " << context.arguments().size() << std::endl;
  // for (const auto &arg : context.arguments())
  // {
  //   if (arg.second.is_dynamic())
  //   {
  //     std::cout << "argument is dynamic" << std::endl;
  //   }
  //   else
  //   {
  //     std::cout << "argument is not dynamic" << std::endl;
  //   }
  //   std::cout << "arg: " << arg.first << " " << std::endl;
  //   arg.second.print(std::cout);
  // }

  const plugin::ArgumentMap &dynamic_args = context.dynamic_arguments();
  for (plugin::ArgumentMap::const_iterator it = dynamic_args.begin(); it != dynamic_args.end(); ++it)
  {
    const std::pair<const std::string, plugin::Argument> &arg = *it;
    std::cout << "dynamic arg: " << arg.first << " " << arg.second.get_string_value() << std::endl;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  auto arg_parser = std::make_shared<parser::ArgumentParser>(argv[0], "do things CLI");
  parser::Command &root_command = arg_parser->get_root_command();

  parser::command_ptr awesome_cmd = parser::command_ptr(new parser::Command("awesome", "do things that are awesome"));
  awesome_cmd->enable_dynamic_keywords(parser::ArgType_Single, "example", "placeholder description");
  awesome_cmd->set_handler(handle_awesome);
  root_command.add_command(awesome_cmd);

  auto result = arg_parser->parse(argc, argv);
  return result.exit_code;
}
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

#ifndef PLUGIN_BRIDGE_HPP
#define PLUGIN_BRIDGE_HPP

#include "../extend/plugin.hpp"
#include "../parser.hpp"

// Forward declarations
class CommandDispatcher;

namespace plugin
{

/**
 * @brief Register all plugin commands to the middleware dispatcher
 *
 * @param dispatcher The CommandDispatcher to register commands to
 */
void register_commands_with_server(CommandDispatcher &dispatcher);

} // namespace plugin

#endif

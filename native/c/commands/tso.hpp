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

#include "../parser.hpp"
#include "../extend/plugin.hpp"

namespace tso
{
using namespace plugin;
int handle_tso_issue(InvocationContext &result);
void register_commands(parser::Command &root_command);
} // namespace tso
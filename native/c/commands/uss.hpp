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

namespace uss
{
using namespace plugin;
int handle_uss_copy(InvocationContext &result);
int handle_uss_create_file(InvocationContext &result);
int handle_uss_create_dir(InvocationContext &result);
int handle_uss_move(InvocationContext &result);
int handle_uss_list(InvocationContext &result);
int handle_uss_view(InvocationContext &result);
int handle_uss_write(InvocationContext &result);
int handle_uss_delete(InvocationContext &result);
int handle_uss_chmod(InvocationContext &result);
int handle_uss_chown(InvocationContext &result);
int handle_uss_chtag(InvocationContext &result);
int handle_uss_issue_cmd(InvocationContext &context);
void register_commands(parser::Command &root_command);
} // namespace uss
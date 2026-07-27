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

#ifndef COMMANDS_CERTIFICATES_HPP
#define COMMANDS_CERTIFICATES_HPP

#include "../parser.hpp"

namespace certificates
{
using namespace plugin;

int handle_cert_list(InvocationContext &context);
int handle_cert_export(InvocationContext &context);
int handle_cert_import(InvocationContext &context);
int handle_cert_delete(InvocationContext &context);
int handle_create_ring(InvocationContext &context);
int handle_delete_ring(InvocationContext &context);
int handle_refresh(InvocationContext &context);
int handle_cert_show(InvocationContext &context);
int handle_list_rings(InvocationContext &context);
int handle_ring_count(InvocationContext &context);
int handle_cert_connect(InvocationContext &context);
int handle_cert_set_default(InvocationContext &context);
int handle_cert_trust(InvocationContext &context);
int handle_cert_rename(InvocationContext &context);

void register_commands(parser::Command &root_command);
} // namespace certificates

#endif // COMMANDS_CERTIFICATES_HPP

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

namespace job
{
using namespace plugin;
int handle_job_list(InvocationContext &result);
int handle_job_list_files(InvocationContext &result);
int handle_job_view_status(InvocationContext &result);
int handle_job_view_file(InvocationContext &result);
int handle_job_view_file_by_id(InvocationContext &result);
int handle_job_view_jcl(InvocationContext &result);
int handle_job_submit(InvocationContext &result);
int handle_job_submit_jcl(InvocationContext &result);
int handle_job_submit_uss(InvocationContext &result);
int handle_job_delete(InvocationContext &result);
int handle_job_cancel(InvocationContext &result);
int handle_job_hold(InvocationContext &result);
int handle_job_release(InvocationContext &result);
int job_submit_common(InvocationContext &result, const std::string &jcl, std::string &jobid, const std::string &identifier, bool strip_crlf = false);
void register_commands(parser::Command &root_command);
} // namespace job

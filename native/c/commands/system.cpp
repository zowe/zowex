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

#include "system.hpp"
#include "common_args.hpp"
#include "../zut.hpp"
#include "../zjb.hpp"
#include "../zlogger.hpp"
#include <unistd.h>
#include <ctime>

using namespace parser;
using namespace std;
using namespace commands::common;
using namespace ast;

namespace sys
{

int handle_system_display_symbol(InvocationContext &context)
{
  int rc = 0;
  string symbol = context.get<std::string>("symbol", "");
  transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);
  symbol = "&" + symbol;
  string value;
  rc = zut_substitute_symbol(symbol, value);
  if (0 != rc)
  {
    context.error_stream() << "Error: asasymbf with parm '" << symbol << "' rc: '" << rc << "'" << endl;
    return RTNCD_FAILURE;
  }
  context.output_stream() << value << endl;

  return RTNCD_SUCCESS;
}

int handle_system_list_parmlib(InvocationContext &context)
{
  int rc = 0;
  ZDIAG diag = {};
  std::vector<std::string> parmlibs;
  rc = zut_list_parmlib(diag, parmlibs);
  if (0 != rc)
  {
    context.error_stream() << "Error: could not list parmlibs rc: '" << rc << "'" << endl;
    context.error_stream() << "  Details: " << diag.e_msg << endl;
    return RTNCD_FAILURE;
  }

  for (vector<string>::iterator it = parmlibs.begin(); it != parmlibs.end(); ++it)
  {
    context.output_stream() << *it << endl;
  }

  return rc;
}

int handle_system_list_proclib(InvocationContext &context)
{
  int rc = 0;
  ZJB zjb = {};

  vector<string> proclib;
  rc = zjb_list_proclib(&zjb, proclib);
  if (0 != rc)
  {
    context.error_stream() << "Error: could not list proclib, rc: '" << rc << "'" << endl;
    context.error_stream() << "  Details: " << zjb.diag.e_msg << endl;
    return RTNCD_FAILURE;
  }

  for (vector<string>::iterator it = proclib.begin(); it != proclib.end(); it++)
  {
    context.output_stream() << *it << endl;
  }

  return RTNCD_SUCCESS;
}

int handle_system_list_subsystems(InvocationContext &context)
{
  int rc = 0;

  string filter = context.get<std::string>("filter", "*");
  transform(filter.begin(), filter.end(), filter.begin(), ::toupper);

  vector<string> subsystems;
  ZDIAG diag = {};
  rc = zut_list_subsystems(diag, subsystems, filter);
  if (0 != rc)
  {
    context.error_stream() << "Error: could not list subsystems, rc: '" << rc << "'" << endl;
    context.error_stream() << "  Details: " << diag.e_msg << endl;
    return RTNCD_FAILURE;
  }

  for (vector<string>::iterator it = subsystems.begin(); it != subsystems.end(); it++)
  {
    context.output_stream() << *it << endl;
  }

  return RTNCD_SUCCESS;
}

#define MAX_LINES 10000
#define DEFAULT_MAX_LINES_ABSOLUTE 100
#define DEFAULT_MAX_LINES_RELATIVE MAX_LINES
#define DEFAULT_SECONDS_AGO 30

int handle_system_view_syslog(InvocationContext &context)
{
  int rc = 0;

  string time_value = context.get<std::string>("time", "");
  string date_value = context.get<std::string>("date", "");
  auto seconds_ago = context.get<long long>("seconds-ago", -1);
  bool has_explicit_date = !date_value.empty();
  bool has_explicit_time = !time_value.empty();
  bool has_seconds_ago = (seconds_ago >= 0);

  // mutual exclusivity: --seconds-ago vs --date/--time
  if (has_seconds_ago && (has_explicit_date || has_explicit_time))
  {
    context.error_stream() << "Error: --seconds-ago is mutually exclusive with --date and --time." << endl;
    context.error_stream() << "Use either --seconds-ago for relative time or --date/--time for absolute time." << endl;
    return RTNCD_FAILURE;
  }

  // pick the right default max-lines based on query mode
  bool using_relative_time = has_seconds_ago || (!has_explicit_date && !has_explicit_time);
  long long default_max_lines = using_relative_time ? DEFAULT_MAX_LINES_RELATIVE : DEFAULT_MAX_LINES_ABSOLUTE;
  auto max_lines = context.get<long long>("max-lines", default_max_lines);

  if (max_lines < 1 || max_lines > MAX_LINES)
  {
    context.error_stream() << "Error: invalid max-lines '" << max_lines << "', expected 1-" << MAX_LINES << endl;
    return RTNCD_FAILURE;
  }

  if (has_seconds_ago && (seconds_ago < 1 || seconds_ago > 86400))
  {
    context.error_stream() << "Error: invalid seconds-ago '" << seconds_ago << "', expected 1-86400" << endl;
    return RTNCD_FAILURE;
  }

  time_t now = time(nullptr);
  struct tm tm_now_storage;
  struct tm *tm_now = localtime_r(&now, &tm_now_storage);
  if (tm_now == nullptr)
  {
    context.error_stream() << "Error: could not obtain local time" << endl;
    return RTNCD_FAILURE;
  }
  char buf[32];

  // when no args provided, default to --seconds-ago 30
  if (!has_explicit_date && !has_explicit_time && !has_seconds_ago)
  {
    seconds_ago = DEFAULT_SECONDS_AGO;
    has_seconds_ago = true;
  }

  if (has_seconds_ago)
  {
    time_t start_time = now - seconds_ago;
    struct tm tm_start_storage;
    struct tm *tm_start = localtime_r(&start_time, &tm_start_storage);
    if (tm_start == nullptr)
    {
      context.error_stream() << "Error: could not compute start time" << endl;
      return RTNCD_FAILURE;
    }
    strftime(buf, sizeof(buf), "%Y-%m-%d", tm_start);
    date_value = buf;
    strftime(buf, sizeof(buf), "%H:%M:%S.00", tm_start);
    time_value = buf;
  }
  else
  {
    // resolve defaults and validate explicit values
    if (time_value.empty())
    {
      strftime(buf, sizeof(buf), "%H:%M:%S", tm_now);
      time_value = buf;
    }
    else
    {
      // Check for invalid formats first (too many colons, dots, etc.)
      size_t colon_count = std::count(time_value.begin(), time_value.end(), ':');
      size_t dot_count = std::count(time_value.begin(), time_value.end(), '.');

      if (colon_count != 2 || dot_count > 1)
      {
        context.error_stream() << "Error: invalid time format '" << time_value << "', expected hh:mm:ss e.g. 12:01:30" << endl;
        return RTNCD_FAILURE;
      }

      int hh = -1, mm = -1, ss = -1, cs = -1;
      int parsed = sscanf(time_value.c_str(), "%d:%d:%d.%d", &hh, &mm, &ss, &cs);

      // Reject centiseconds format since it's not used in output
      if (parsed == 4)
      {
        context.error_stream() << "Error: centiseconds not supported, use hh:mm:ss format e.g. 12:01:30" << endl;
        return RTNCD_FAILURE;
      }

      // Validate HH:MM:SS format
      if (parsed != 3 || hh < 0 || hh > 23 || mm < 0 || mm > 59 || ss < 0 || ss > 59)
      {
        context.error_stream() << "Error: invalid time '" << time_value << "', expected hh:mm:ss e.g. 12:01:30" << endl;
        return RTNCD_FAILURE;
      }
    }

    if (date_value.empty())
    {
      strftime(buf, sizeof(buf), "%Y-%m-%d", tm_now);
      date_value = buf;
    }
    else
    {
      int yyyy = -1, mm = -1, dd = -1;
      int parsed = sscanf(date_value.c_str(), "%d-%d-%d", &yyyy, &mm, &dd);
      if (parsed != 3 || yyyy < 1900 || yyyy > 2185 || mm < 1 || mm > 12 || dd < 1 || dd > 31)
      {
        context.error_stream() << "Error: invalid date '" << date_value << "', expected yyyy-mm-dd e.g. 2026-03-13" << endl;
        return RTNCD_FAILURE;
      }
    }
  }

  ZLOG_DEBUG("view-syslog options in effect: date='%s', time='%s', max_lines=%lld",
             date_value.c_str(), time_value.c_str(), static_cast<long long>(max_lines));

  string response;
  ZJB zjb = {};
  ZJBSyslogOptions opts;
  opts.date = date_value;
  opts.time = time_value;
  opts.max_lines = static_cast<int>(max_lines);

  rc = zjb_read_syslog(&zjb, response, opts);
  if (0 != rc)
  {
    context.error_stream() << "Error: could not view syslog, rc: '" << rc << "'" << endl;
    context.error_stream() << "  Details: " << zjb.diag.e_msg << endl;
    return RTNCD_FAILURE;
  }

  const auto result = obj();
  result->set("startDate", str(date_value));
  result->set("startTime", str(time_value));
  result->set("endDate", str(opts.end_date));
  result->set("endTime", str(opts.end_time));
  result->set("returnedLines", i64(opts.returned_lines));
  result->set("hasMore", boolean(opts.has_more));
  context.set_object(result);

  context.output_stream() << response;

  if (!context.is_redirecting_output())
  {
    context.error_stream() << endl
                           << "Start: " << date_value << " " << time_value
                           << " | End: " << opts.end_date << " " << opts.end_time
                           << " | Lines: " << opts.returned_lines
                           << " | Has more: " << (opts.has_more ? "yes" : "no") << endl;
  }

  return RTNCD_SUCCESS;
}

void register_commands(parser::Command &root_command)
{
  auto system_cmd = command_ptr(new Command("system", "system operations"));

  // Display symbol subcommand
  auto system_display_symbol_cmd = command_ptr(new Command("display-symbol", "display system symbol"));
  system_display_symbol_cmd->add_positional_arg("symbol", "symbol to display", ArgType_Single, true);
  system_display_symbol_cmd->set_handler(handle_system_display_symbol);
  system_cmd->add_command(system_display_symbol_cmd);

  // List-parmlib subcommand
  auto system_list_parmlib_cmd = command_ptr(new Command("list-parmlib", "list parmlib"));
  system_list_parmlib_cmd->set_handler(handle_system_list_parmlib);
  system_cmd->add_command(system_list_parmlib_cmd);

  // List-proclib subcommand
  auto system_list_proclib_cmd = command_ptr(new Command("list-proclib", "list proclib"));
  system_list_proclib_cmd->set_handler(handle_system_list_proclib);
  system_cmd->add_command(system_list_proclib_cmd);

  // List-subsystems subcommand
  auto system_list_subsystems_cmd = command_ptr(new Command("list-subsystems", "list subsystems"));
  system_list_subsystems_cmd->set_handler(handle_system_list_subsystems);
  system_list_subsystems_cmd->add_keyword_arg("filter", make_aliases("--filter", "-f"), "filter subsystems", ArgType_Single, false);
  system_cmd->add_command(system_list_subsystems_cmd);

  // View-syslog subcommand
  auto system_view_syslog_cmd = command_ptr(new Command("view-syslog", "view syslog"));
  system_view_syslog_cmd->set_handler(handle_system_view_syslog);
  system_view_syslog_cmd->add_keyword_arg("time", make_aliases("--time", "-t"), "specify time hh:mm:ss, e.g. -t 10:41:00. Mutually exclusive with --seconds-ago", ArgType_Single, false);
  system_view_syslog_cmd->add_keyword_arg("date", make_aliases("--date", "-d"), "specify date yyyy-mm-dd, e.g. --date 2026-01-20. Mutually exclusive with --seconds-ago", ArgType_Single, false, ArgValue(""), true);
  system_view_syslog_cmd->add_keyword_arg("seconds-ago", make_aliases("--seconds-ago", "-s"), "relative offset in seconds from now, e.g. --seconds-ago 300 for last 5 minutes. Mutually exclusive with --date/--time", ArgType_Single, false);
  system_view_syslog_cmd->add_keyword_arg("max-lines", make_aliases("--max-lines", "--ml"), "maximum number of lines to display (1-10000), e.g. --max-lines 100", ArgType_Single, false);
  system_view_syslog_cmd->add_example("View last 30 seconds of syslog (default)", "zowex system view-syslog");
  system_view_syslog_cmd->add_example("View last 5 minutes of syslog", "zowex system view-syslog --seconds-ago 300");
  system_view_syslog_cmd->add_example("View syslog for a specific date and time", "zowex system view-syslog --date 2026-03-13 --time 10:41:00 --max-lines 100");
  system_cmd->add_command(system_view_syslog_cmd);

  root_command.add_command(system_cmd);
}
} // namespace sys

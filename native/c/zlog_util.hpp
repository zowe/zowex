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

#ifndef ZLOG_UTIL_HPP
#define ZLOG_UTIL_HPP

#include <cerrno>
#include <cstdlib>
#include <pwd.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

// Shared helpers for resolving where zowex should write its log files.
//
// Logs are written per-user (defaulting to `~/.zowex/logs`) rather than to a
// location shared by all users of the binary (e.g. next to the executable),
// so that concurrent users of a shared zowex installation don't contend for
// the same log file.
namespace zlog_util
{

inline std::string get_home_dir()
{
  const char *home = std::getenv("HOME");
  if (home && *home)
  {
    return std::string(home);
  }

  const struct passwd *pw = getpwuid(getuid());
  if (pw && pw->pw_dir)
  {
    return std::string(pw->pw_dir);
  }

  return std::string();
}

inline std::string strip_trailing_slashes(std::string path)
{
  while (path.size() > 1 && path.back() == '/')
  {
    path.pop_back();
  }
  return path;
}

// Creates each missing directory component of `path`, similar to `mkdir -p`.
inline bool make_dirs(const std::string &path)
{
  if (path.empty())
  {
    return false;
  }

  std::string current;
  size_t pos = 0;
  if (path[0] == '/')
  {
    current = "/";
    pos = 1;
  }

  while (pos <= path.size())
  {
    size_t next = path.find('/', pos);
    if (next == std::string::npos)
    {
      next = path.size();
    }

    current += path.substr(pos, next - pos);

    if (!current.empty() && current != "/" && mkdir(current.c_str(), 0700) != 0 && errno != EEXIST)
    {
      return false;
    }

    current += "/";
    pos = next + 1;
  }

  return true;
}

// Resolves the directory zowex should write log files to. Honors
// `ZOWEX_LOGS_DIR` if set, otherwise defaults to `<home>/.zowex/logs`.
inline std::string resolve_logs_dir()
{
  const char *override_dir = std::getenv("ZOWEX_LOGS_DIR");
  if (override_dir && *override_dir)
  {
    return strip_trailing_slashes(std::string(override_dir));
  }

  const std::string home = get_home_dir();
  if (!home.empty())
  {
    return strip_trailing_slashes(home) + "/.zowex/logs";
  }

  // Last resort if the home directory cannot be determined
  return "logs";
}

} // namespace zlog_util

#endif // ZLOG_UTIL_HPP

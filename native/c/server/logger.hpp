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

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fstream>
#include <string>
#include <mutex>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cstdlib>
#include <cerrno>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include "../zlog_util.hpp"

namespace server
{
class Logger
{
private:
  // Buffer size for log messages
  static constexpr size_t LOG_BUFFER_SIZE = 4096;
  // Maximum size of a single log file before it is rolled over (100KB)
  static constexpr size_t MAX_LOG_FILE_SIZE = 100 * 1024;
  // Number of log files retained via FIFO rotation (1 active + up to 9 backups),
  // bounding total disk usage per user to roughly MAX_LOG_FILES * MAX_LOG_FILE_SIZE (1MB)
  static constexpr int MAX_LOG_FILES = 10;

  static std::ofstream &get_log_file()
  {
    static std::ofstream log_file;
    return log_file;
  }

  static bool &get_verbose_logging()
  {
    static bool verbose_logging = false;
    return verbose_logging;
  }

  static std::mutex &get_log_mutex()
  {
    static std::mutex log_mutex;
    return log_mutex;
  }

  static bool &get_initialized()
  {
    static bool initialized = false;
    return initialized;
  }

  static std::string &get_log_file_path()
  {
    static std::string log_file_path;
    return log_file_path;
  }

  /**
   * Get the current timestamp as a formatted string
   */
  static std::string get_current_timestamp()
  {
    const auto now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::string(buffer);
  }

  /**
   * Shift existing backup log files (log.1 -> log.2, log.2 -> log.3, ...),
   * dropping the oldest backup, then move the active log into the log.1 slot.
   */
  static void rotate_log_files()
  {
    const auto &log_file_path = get_log_file_path();

    // Drop the oldest backup, if it exists
    const std::string oldest = log_file_path + "." + std::to_string(MAX_LOG_FILES - 1);
    unlink(oldest.c_str());

    // Shift remaining backups up by one slot
    for (int i = MAX_LOG_FILES - 2; i >= 1; i--)
    {
      const std::string from = log_file_path + "." + std::to_string(i);
      const std::string to = log_file_path + "." + std::to_string(i + 1);
      rename(from.c_str(), to.c_str());
    }

    // Move the active log into the first backup slot
    const std::string first_backup = log_file_path + ".1";
    rename(log_file_path.c_str(), first_backup.c_str());
  }

  /**
   * Check log file size and roll over to a new file if necessary, retaining
   * up to MAX_LOG_FILES generations in FIFO order (oldest backup is dropped
   * to make room for the newest).
   */
  static void check_and_rotate_log_file()
  {
    const auto &log_file_path = get_log_file_path();
    std::ofstream &log_file = get_log_file();

    if (log_file_path.empty() || !log_file.is_open())
    {
      return;
    }

    struct stat st;
    if (stat(log_file_path.c_str(), &st) == 0)
    {
      if (static_cast<size_t>(st.st_size) > MAX_LOG_FILE_SIZE)
      {
        log_file.close();

        rotate_log_files();

        // Create/open a fresh active log file with restricted permissions (0600)
        // 0600 = read/write for the owning user only, no access for anyone else
        int fd = open(log_file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (fd == -1)
        {
          std::cerr << "Failed to create log file after rotation: " << log_file_path << std::endl;
          return;
        }
        close(fd);

        log_file.open(log_file_path.c_str(), std::ios::out | std::ios::trunc);
        if (!log_file.is_open())
        {
          std::cerr << "Failed to reopen log file after rotation: " << log_file_path << std::endl;
          return;
        }

        log_file << get_current_timestamp() << " [INFO] Log file rolled over due to size limit\n";
        log_file.flush();
      }
    }
  }

  /**
   * Common logging implementation
   */
  static void log_message(const char *level, const char *format, va_list args)
  {
    std::mutex &log_mutex = get_log_mutex();
    std::ofstream &log_file = get_log_file();
    const std::lock_guard<std::mutex> lock(log_mutex);

    char buffer[LOG_BUFFER_SIZE];
    vsnprintf(buffer, sizeof(buffer), format, args);

    log_file << get_current_timestamp() << " [" << level << "] " << buffer << std::endl;

    check_and_rotate_log_file();
  }

public:
  /**
   * Initialize the logger with specified options
   * @param verbose Whether to enable verbose logging
   * @param truncate Whether to truncate existing log file
   */
  static void init_logger(bool verbose = false, bool truncate = false)
  {
    std::mutex &log_mutex = get_log_mutex();
    std::ofstream &log_file = get_log_file();
    bool &verbose_logging = get_verbose_logging();
    bool &initialized = get_initialized();
    std::string &log_file_path = get_log_file_path();

    {
      const std::lock_guard<std::mutex> lock(log_mutex);

      verbose_logging = verbose;

      // Resolve and create the per-user logs directory (honors ZOWEX_LOGS_DIR)
      const std::string logs_dir = zlog_util::resolve_logs_dir();

      if (!zlog_util::make_dirs(logs_dir))
      {
        std::cerr << "Failed to create logs directory: " << logs_dir << std::endl;
        return;
      }

      // Set log file path
      log_file_path = logs_dir + "/zowex_server.log";

      // Open log file
      auto mode = std::ios::out;
      if (truncate)
      {
        mode |= std::ios::trunc;
      }
      else
      {
        mode |= std::ios::app;
      }

      if (log_file.is_open())
      {
        log_file.close();
      }

      // Create/open file with restricted permissions (0600)
      int fd = open(log_file_path.c_str(), O_WRONLY | O_CREAT | (truncate ? O_TRUNC : O_APPEND), 0600);
      if (fd == -1)
      {
        std::cerr << "Failed to create log file: " << log_file_path << std::endl;
        return;
      }
      close(fd);

      // Now open with fstream
      log_file.open(log_file_path.c_str(), mode);
      if (!log_file.is_open())
      {
        std::cerr << "Failed to initialize logger: could not open " << log_file_path << std::endl;
        return;
      }

      initialized = true;
    }
    if (verbose)
    {
      log_debug("Verbose logging enabled");
    }
  }

  /**
   * Check if verbose logging is enabled
   */
  static bool is_verbose_logging()
  {
    return get_verbose_logging();
  }

  /**
   * Log a debug message (only if verbose logging is enabled)
   */
  static void log_debug(const char *format, ...)
  {
    if (!get_initialized() || !is_verbose_logging())
    {
      return;
    }
    va_list args;
    va_start(args, format);
    log_message("DEBUG", format, args);
    va_end(args);
  }

  /**
   * Log an info message
   */
  static void log_info(const char *format, ...)
  {
    if (!get_initialized())
    {
      return;
    }
    va_list args;
    va_start(args, format);
    log_message("INFO", format, args);
    va_end(args);
  }

  /**
   * Log a warning message
   */
  static void log_warn(const char *format, ...)
  {
    if (!get_initialized())
    {
      return;
    }
    va_list args;
    va_start(args, format);
    log_message("WARN", format, args);
    va_end(args);
  }

  /**
   * Log an error message
   */
  static void log_error(const char *format, ...)
  {
    if (!get_initialized())
    {
      return;
    }
    va_list args;
    va_start(args, format);
    log_message("ERROR", format, args);
    va_end(args);
  }

  /**
   * Log a fatal error and exit the program
   */
  static void log_fatal(const char *format, ...)
  {
    char buffer[LOG_BUFFER_SIZE];
    va_list args;
    va_start(args, format);

    va_list args_copy;
    va_copy(args_copy, args);

    vsnprintf(buffer, sizeof(buffer), format, args);

    if (get_initialized())
    {
      log_message("FATAL", format, args_copy);
    }

    va_end(args_copy);
    va_end(args);

    // Also print to stderr
    std::cerr << "FATAL: " << buffer << std::endl;

    exit(1);
  }

  /**
   * Cleanup and close the log file
   */
  static void shutdown()
  {
    std::mutex &log_mutex = get_log_mutex();
    std::ofstream &log_file = get_log_file();
    bool &initialized = get_initialized();

    const std::lock_guard<std::mutex> lock(log_mutex);
    if (log_file.is_open())
    {
      log_file.close();
    }
    initialized = false;
  }
};

} // namespace server

// Convenience macros for easier usage
#define LOG_DEBUG(...) server::Logger::log_debug(__VA_ARGS__)
#define LOG_INFO(...) server::Logger::log_info(__VA_ARGS__)
#define LOG_WARN(...) server::Logger::log_warn(__VA_ARGS__)
#define LOG_ERROR(...) server::Logger::log_error(__VA_ARGS__)
#define LOG_FATAL(...) server::Logger::log_fatal(__VA_ARGS__)

#endif // LOGGER_HPP

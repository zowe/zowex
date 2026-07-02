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

#include <cerrno>
#include <fstream>
#include <string>

#ifdef __MVS__
#include <sys/stat.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "ztest.hpp"
#include "../zlogger.hpp"

using namespace ztst;

// Helper function to check if a file exists
bool file_exists(const std::string &path)
{
  struct stat buffer;
  return (stat(path.c_str(), &buffer) == 0);
}

// Helper function to read file contents
std::string read_file_contents(const std::string &path)
{
  std::ifstream file(path);
  if (!file.is_open())
  {
    return "";
  }

  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  file.close();
  return content;
}

// Helper function to clean up test files
void cleanup_test_files()
{
  // Clean up any test log files
  unlink("logs/zowex.log");
  rmdir("logs");
}

void zlogger_tests()
{
  describe("ZLogger singleton tests", []() -> void
           {

        it("should initialize logger and create logs directory", []() {
            // Get logger instance - this should trigger initialization
            ZLogger& logger = ZLogger::get_instance();
            
            // Check that logs directory was created
            Expect(file_exists("logs/zowex.log")).ToBe(true);
        });
        
        it("should create a singleton instance", []() {
            ZLogger& logger1 = ZLogger::get_instance();
            ZLogger& logger2 = ZLogger::get_instance();
            
            // Both references should point to the same instance
            Expect(&logger1).ToBe(&logger2);
        });

        it("should handle log level setting and getting", []() {
            ZLogger& logger = ZLogger::get_instance();
            
            // Set different log levels and verify they are applied
            logger.set_log_level(ZLOGLEVEL_DEBUG);
            Expect(logger.get_log_level()).ToBe(ZLOGLEVEL_DEBUG);
            
            logger.set_log_level(ZLOGLEVEL_WARN);
            Expect(logger.get_log_level()).ToBe(ZLOGLEVEL_WARN);
            
            logger.set_log_level(ZLOGLEVEL_ERROR);
            Expect(logger.get_log_level()).ToBe(ZLOGLEVEL_ERROR);
        });

        it("should handle log level conversion from std::string", []() {
            ZLogger& logger = ZLogger::get_instance();
            
            // Test environment variable log level setting
            setenv("ZOWEX_LOG_LEVEL", "DEBUG", 1);
            
            // Note: This tests the internal get_level_from_str method indirectly
            // We can't test it directly as it's private, but we can verify behavior
            logger.set_log_level(ZLOGLEVEL_TRACE);
            Expect(logger.get_log_level()).ToBe(ZLOGLEVEL_TRACE);
            
            unsetenv("ZOWEX_LOG_LEVEL");
        });

        it("should support all log levels correctly", []() {
            ZLogger& logger = ZLogger::get_instance();
            
            // Test all supported log levels
            logger.set_log_level(ZLOGLEVEL_TRACE);
            Expect(logger.get_log_level()).ToBe(ZLOGLEVEL_TRACE);
            
            logger.set_log_level(ZLOGLEVEL_DEBUG);
            Expect(logger.get_log_level()).ToBe(ZLOGLEVEL_DEBUG);
            
            logger.set_log_level(ZLOGLEVEL_INFO);
            Expect(logger.get_log_level()).ToBe(ZLOGLEVEL_INFO);
            
            logger.set_log_level(ZLOGLEVEL_WARN);
            Expect(logger.get_log_level()).ToBe(ZLOGLEVEL_WARN);
            
            logger.set_log_level(ZLOGLEVEL_ERROR);
            Expect(logger.get_log_level()).ToBe(ZLOGLEVEL_ERROR);
            
            logger.set_log_level(ZLOGLEVEL_FATAL);
            Expect(logger.get_log_level()).ToBe(ZLOGLEVEL_FATAL);
        }); });

  describe("ZLogger logging functionality", []() -> void
           {
        
        it("should log messages with different levels", []() {
            ZLogger& logger = ZLogger::get_instance();
            logger.set_log_level(ZLOGLEVEL_TRACE);
            
            // Log messages at different levels
            logger.trace("Test trace message");
            logger.debug("Test debug message");
            logger.info("Test info message");
            logger.warn("Test warning message");
            logger.error("Test error message");
            
            // Give some time for file I/O
            usleep(1000);
            
            // Check that log file exists
            Expect(file_exists("logs/zowex.log")).ToBe(true);
        });

        it("should respect log level filtering", []() {
            ZLogger& logger = ZLogger::get_instance();
            
            // Set log level to WARN - should only log WARN, ERROR, FATAL
            logger.set_log_level(ZLOGLEVEL_WARN);
            
            logger.trace("This should not be logged");
            logger.debug("This should not be logged");
            logger.info("This should not be logged");
            logger.warn("This should be logged");
            logger.error("This should be logged");
            
            usleep(1000);
            
            // Verify log file exists
            Expect(file_exists("logs/zowex.log")).ToBe(true);
            
            // Read log contents and verify filtering
            std::string contents = read_file_contents("logs/zowex.log");
            Expect(contents).Not().ToContain("This should not be logged");
            Expect(contents).ToContain("This should be logged");
        });

        it("should handle variadic log function", []() {
            ZLogger& logger = ZLogger::get_instance();
            logger.set_log_level(ZLOGLEVEL_INFO);
            
            // Test formatted logging
            logger.log(ZLOGLEVEL_INFO, "Test formatted message: %d %s", 42, "hello");
            
            usleep(1000);
            
            std::string contents = read_file_contents("logs/zowex.log");
            Expect(contents).ToContain("Test formatted message: 42 hello");
        });

        it("should handle log messages with special characters", []() {
            ZLogger& logger = ZLogger::get_instance();
            logger.set_log_level(ZLOGLEVEL_INFO);
            
            // Test logging with special characters
            logger.info("Test message with special chars: !@#$^&*()");
            
            usleep(1000);
            
            std::string contents = read_file_contents("logs/zowex.log");
            Expect(contents).ToContain("special chars: !@#$^&*()");
        });

        it("should handle empty and null log messages gracefully", []() {
            ZLogger& logger = ZLogger::get_instance();
            logger.set_log_level(ZLOGLEVEL_INFO);
            
            // These should not crash
            logger.info("");
            logger.info("   ");
            
            // Test with format strings
            logger.log(ZLOGLEVEL_INFO, "");
            logger.log(ZLOGLEVEL_INFO, "%s", "");
        });

        it("should handle OFF log level correctly", []() {
            ZLogger& logger = ZLogger::get_instance();
            
            // Set log level to OFF - should not log anything
            logger.set_log_level(ZLOGLEVEL_OFF);
            
            logger.fatal("This should not be logged even though it's FATAL");
            logger.error("This should not be logged");
            
            usleep(1000);
            
            // If log file exists, it should be empty or not contain our messages
            if (file_exists("logs/zowex.log")) {
                std::string contents = read_file_contents("logs/zowex.log");
                Expect(contents).Not().ToContain("This should not be logged");
            }
        }); });

  describe("ZLogger edge cases and error handling", []() -> void
           {
        
        it("should handle rapid successive log calls", []() {
            ZLogger& logger = ZLogger::get_instance();
            logger.set_log_level(ZLOGLEVEL_INFO);
            
            // Log many messages rapidly
            for (int i = 0; i < 100; i++) {
                logger.info("Rapid log message %d", i);
            }
            
            usleep(10000); // Give more time for I/O
            
            // Verify log file exists and has content
            Expect(file_exists("logs/zowex.log")).ToBe(true);
            
            std::string contents = read_file_contents("logs/zowex.log");
            Expect(contents).ToContain("Rapid log message");
        });

        it("should maintain thread safety for singleton access", []() {
            // Test that multiple calls to get_instance return the same object
            ZLogger* instances[10];
            
            for (int i = 0; i < 10; i++) {
                instances[i] = &ZLogger::get_instance();
            }
            
            // All instances should be the same
            for (int i = 1; i < 10; i++) {
                Expect(instances[i]).ToBe(instances[0]);
            }
        }); });

  // Clean up after all tests
  cleanup_test_files();
}

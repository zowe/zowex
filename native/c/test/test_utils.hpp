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

#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <iostream>
#include <sstream>
#include <string>

namespace test_utils
{

/**
 * @brief RAII utility class for capturing stderr output during tests
 *
 * This class redirects std::cerr to an internal buffer for the duration
 * of its lifetime, automatically restoring the original stderr stream
 * when the object is destroyed. This is particularly useful for testing
 * error conditions where you want to suppress expected error messages
 * from appearing in test output.
 *
 * @note This class is not thread-safe and should only be used in
 *       single-threaded test contexts.
 *
 * @example Usage in a test:
 * @code
 *   {
 *     test_utils::ErrorStreamCapture error_capture;
 *     // Code that writes to std::cerr
 *     some_function_that_logs_errors();
 *     // Can optionally inspect captured output
 *     std::string errors = error_capture.get_output();
 *   } // std::cerr automatically restored here
 * @endcode
 */
class ErrorStreamCapture
{
public:
  /**
   * @brief Constructs the error capture and redirects std::cerr
   *
   * Saves the current std::cerr buffer and redirects it to an internal
   * stringstream for capturing output.
   */
  ErrorStreamCapture()
      : original_cerr_(std::cerr.rdbuf())
  {
    std::cerr.rdbuf(buffer_.rdbuf());
  }

  /**
   * @brief Destructor that automatically restores std::cerr
   *
   * Restores the original std::cerr buffer, ensuring that subsequent
   * output goes to the correct stream even if an exception is thrown.
   */
  ~ErrorStreamCapture()
  {
    std::cerr.rdbuf(original_cerr_);
  }

  // Delete copy constructor and assignment operator to prevent misuse
  ErrorStreamCapture(const ErrorStreamCapture &) = delete;
  ErrorStreamCapture &operator=(const ErrorStreamCapture &) = delete;

  /**
   * @brief Returns the captured error output as a string
   *
   * @return std::string The complete output written to std::cerr since
   *         construction or last clear()
   */
  std::string get_output() const
  {
    return buffer_.str();
  }

  /**
   * @brief Clears the captured buffer
   *
   * Resets the internal buffer, allowing the capture object to be
   * reused for capturing a new sequence of error output.
   */
  void clear()
  {
    buffer_.str("");
    buffer_.clear();
  }

private:
  std::stringstream buffer_;      ///< Internal buffer for capturing output
  std::streambuf *original_cerr_; ///< Original std::cerr buffer to restore
};

/**
 * @brief RAII utility class for capturing stdout output during tests
 */
class OutputStreamCapture
{
public:
  OutputStreamCapture()
      : original_cout_(std::cout.rdbuf())
  {
    std::cout.rdbuf(buffer_.rdbuf());
  }

  ~OutputStreamCapture()
  {
    std::cout.rdbuf(original_cout_);
  }

  OutputStreamCapture(const OutputStreamCapture &) = delete;
  OutputStreamCapture &operator=(const OutputStreamCapture &) = delete;

  std::string get_output() const
  {
    return buffer_.str();
  }

  void clear()
  {
    buffer_.str("");
    buffer_.clear();
  }

private:
  std::stringstream buffer_;
  std::streambuf *original_cout_;
};

} // namespace test_utils

#endif // TEST_UTILS_HPP

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

#ifndef ZTEST_HPP
#define ZTEST_HPP

#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif

#define _POSIX_SOURCE
#include <iostream>
#include <vector>
#include <csetjmp>
#include <csignal>
#include <type_traits>
#include <cxxabi.h>
#include <typeinfo>
#include <sstream>
#include <cstring> // Required for memset
#include <chrono>
#include <iomanip>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/select.h>
#include <algorithm>
#include <fstream>
#include <unistd.h>
#include <cstdlib>
#include <regex>
#include <functional>

// TODO(Kelosky): handle test not run
// TODO(Kelosky): handle running individual test and/or suite

#define Expect(x) expect((x), EXPECT_CONTEXT{__LINE__, __FILE__, "", false})
#define ExpectWithContext(x, context) expect((x), EXPECT_CONTEXT{__LINE__, __FILE__, std::string(context), true})
#define TestLog(message) Globals::get_instance().test_log(message)
#define TrimChars(str) Globals::get_instance().trim_chars(str)

namespace ztst
{

// Forward declarations
class Globals;

// Default timeout for individual tests and hooks (in seconds)
constexpr unsigned int DEFAULT_TEST_TIMEOUT_SECONDS = 10;

struct TEST_OPTIONS
{
  bool remove_signal_handling;
  unsigned int timeout_sec; // Timeout in seconds (0 = use default)
};

inline std::string get_indent(int level)
{
  return std::string(level * 2, ' ');
}

// Forward declaration of signal handlers
inline void signal_handler(int code, siginfo_t *info, void *context);
inline void timeout_handler(int sig);

// ANSI color codes
struct Colors
{
  const char *green;
  const char *red;
  const char *yellow;
  const char *reset;
  const char *check; // Unicode check mark or ASCII alternative
  const char *cross; // Unicode X or ASCII alternative
  const char *warn;  // Unicode warning or ASCII alternative
  const char *skip;  // Unicode circle or ASCII alternative
  const char *arrow; // Unicode corner arrow or ASCII alternative

  Colors()
  {
    bool use_color = false;
    bool use_unicode = false;

    // Check if colors are explicitly forced on
    if (getenv("FORCE_COLOR") != nullptr)
    {
      use_color = true;
    }
    else
    {
      // Check if we're on z/OS - disable colors by default
#if defined(__MVS__) || defined(__NATIVE_EBCDIC__)
      use_color = false;
#else
      // Check if colors are explicitly disabled
      if (getenv("NO_COLOR") != nullptr)
      {
        use_color = false;
      }
      // Check if we're in a terminal that supports color
      else if (const char *term = getenv("TERM"))
      {
        use_color = isatty(STDOUT_FILENO) &&
                    (strstr(term, "xterm") != nullptr ||
                     strstr(term, "vt100") != nullptr ||
                     strstr(term, "ansi") != nullptr ||
                     strstr(term, "color") != nullptr ||
                     strstr(term, "linux") != nullptr);
      }
      // Check if we're in a CI environment that supports color
      else if (getenv("CI") != nullptr || getenv("GITHUB_ACTIONS") != nullptr)
      {
        use_color = true;
      }
#endif
    }

    // Check if we can use Unicode characters
    const char *lang = getenv("LANG");
    if (lang && (strstr(lang, "UTF-8") != nullptr || strstr(lang, "UTF8") != nullptr))
    {
      use_unicode = true;
    }

    if (use_color)
    {
      green = "\x1B[32m";
      red = "\x1B[31m";
      yellow = "\x1B[33m";
      reset = "\x1B[0m";
    }
    else
    {
      green = "";
      red = "";
      yellow = "";
      reset = "";
    }

    if (use_unicode)
    {
      check = "✓";
      cross = "✗";
      warn = "!";
      skip = "○";
      arrow = "└─";
    }
    else
    {
      check = "+";
      cross = "-";
      warn = "!";
      skip = "/";
      arrow = "|-";
    }
  }
};

static Colors colors;

struct TEST_CASE
{
  bool success;
  bool skipped;
  std::string description;
  std::string fail_message;
  std::chrono::high_resolution_clock::time_point start_time;
  std::chrono::high_resolution_clock::time_point end_time;
};

using hook_callback = std::function<void()>;

struct HOOK_WITH_OPTIONS
{
  hook_callback callback;
  TEST_OPTIONS options;
};

struct TEST_SUITE
{
  std::string description;
  std::vector<TEST_CASE> tests;
  int nesting_level;
  std::vector<HOOK_WITH_OPTIONS> before_all_hooks;
  std::vector<HOOK_WITH_OPTIONS> before_each_hooks;
  std::vector<HOOK_WITH_OPTIONS> after_each_hooks;
  std::vector<HOOK_WITH_OPTIONS> after_all_hooks;
  bool before_all_executed = false;
  bool skipped = false;
  bool hook_failed = false;
  std::string hook_error;
};

struct EXPECT_CONTEXT
{
  int line_number;
  std::string file_name;
  std::string message;
  bool initialized;
};

class Globals
{

private:
  std::vector<TEST_SUITE> suites;
  int suite_index = -1;
  int current_nesting = 0;
  jmp_buf jump_buf = {0};
  std::string matcher = "";
  std::vector<int> suite_stack;
  std::string znp_test_log = "";
  bool timeout_occurred = false;

  Globals()
  {
  }
  ~Globals()
  {
  }
  Globals(const Globals &) = delete;
  Globals &operator=(const Globals &) = delete;

  static void setup_signal_handlers(struct sigaction &sa)
  {
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;

#if defined(__MVS__)
    sigaction(SIGABND, &sa, nullptr);
#endif
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
  }

  static void reset_signal_handlers(struct sigaction &sa)
  {
    sa.sa_flags = 0;
    sa.sa_handler = SIG_DFL;
#if defined(__MVS__)
    sigaction(SIGABND, &sa, nullptr);
#endif
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
  }

  static void setup_timeout_handler(struct sigaction &sa, struct sigaction &old_sa)
  {
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = timeout_handler;
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, &old_sa); // Save old handler
  }

  static void reset_timeout_handler(const struct sigaction &old_sa)
  {
    alarm(0);                             // Cancel any pending alarm
    sigaction(SIGALRM, &old_sa, nullptr); // Restore old handler
  }

  template <typename Callable>
  static void execute_test(Callable &test, TEST_CASE &tc, bool &abend)
  {
    try
    {
      test();
      tc.success = true;
    }
    catch (const std::exception &e)
    {
      tc.success = false;
      tc.fail_message = e.what();
    }
  }

  static std::pair<std::string, const char *> format_test_status(const TEST_CASE &tc, bool abend)
  {
    std::string icon;
    const char *color;

    if (tc.success)
    {
      icon = std::string(colors.check) + " PASS"; // Single space after status character
      color = colors.green;
    }
    else if (abend)
    {
      icon = std::string(colors.warn) + " ABEND"; // Single space after status character
      color = colors.yellow;
    }
    else
    {
      icon = std::string(colors.cross) + " FAIL"; // Single space after status character
      color = colors.red;
    }
    return std::make_pair(icon, color);
  }

  static void print_test_output(const TEST_CASE &tc, const std::string &description,
                                int current_nesting, const std::string &icon,
                                const char *color)
  {
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        tc.end_time - tc.start_time);

    // Print exactly current_nesting * 2 spaces
    for (int i = 0; i < current_nesting * 2; i++)
    {
      std::cout << " ";
    }

    if (color[0] != '\0')
    { // Only use color if it's enabled
      std::cout << color << icon << colors.reset << " ";
    }
    else
    {
      std::cout << icon << " ";
    }
    std::cout << description;

    if (duration.count() >= 1e6)
    {
      std::cout << " (" << std::fixed << std::setprecision(3)
                << duration.count() / 1000000.0 << "s)";
    }
    std::cout << std::endl;

    if (!tc.success)
    {
      print_failure_details(tc, current_nesting, color);
    }
  }

  static void print_failure_details(const TEST_CASE &tc, int current_nesting,
                                    const char *color)
  {
    std::string main_error = tc.fail_message;
    std::string context = "";

    size_t pos = main_error.find("\n");
    if (pos != std::string::npos)
    {
      context = main_error.substr(pos);
      main_error = main_error.substr(0, pos);
    }

    // Print exactly current_nesting * 2 spaces
    Globals::get_instance().pad_nesting(current_nesting);
    std::cout << color << colors.arrow << colors.reset << " " << main_error << std::endl;

    if (!context.empty())
    {
      std::string indent = get_indent(current_nesting + 1);
      std::istringstream lines(context.substr(1));
      std::string line;
      while (std::getline(lines, line))
        std::cout << indent << line << std::endl;
    }
  }

public:
  static Globals &get_instance()
  {
    static Globals instance;
    return instance;
  }

  std::string &get_matcher()
  {
    return matcher;
  }
  void set_matcher(const std::string &m)
  {
    matcher = m;
  }
  std::vector<TEST_SUITE> &get_suites()
  {
    return suites;
  }
  int get_suite_index()
  {
    return suite_index;
  }
  void push_suite_index(int si)
  {
    suite_stack.push_back(si);
    suite_index = si;
  }
  void pop_suite_index()
  {
    // Check if stack has elements before popping last suite index (otherwise pop_back is considered UB for empty vectors)
    if (!suite_stack.empty())
    {
      suite_stack.pop_back();
    }

    // After removing the last suite stack, re-assign the suite index to the next/parent suite if the stack still has suites
    if (!suite_stack.empty())
    {
      suite_index = suite_stack.back();
    }
  }
  jmp_buf &get_jmp_buf()
  {
    return jump_buf;
  }
  void set_timeout_occurred(bool value)
  {
    timeout_occurred = value;
  }
  bool get_timeout_occurred()
  {
    return timeout_occurred;
  }
  void increment_nesting()
  {
    current_nesting++;
  }
  void decrement_nesting()
  {
    if (current_nesting > 0)
      current_nesting--;
  }
  int get_nesting()
  {
    return current_nesting;
  }
  std::string &trim_chars(std::string &str, const std::string &chars = " \t\n\r\f\v")
  {
    str.erase(0, str.find_first_not_of(chars));
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
  }
  void pad_nesting(int level)
  {
    for (int i = 0; i < level * 2; i++)
    {
      std::cout << " ";
    }
  }
  void test_log(const std::string &message)
  {
    static bool show_test_log = false;
    if (znp_test_log.empty())
    {
      const char *debug = getenv("ZNP_TEST_LOG");
      znp_test_log = debug == nullptr || strstr(debug, "ON") != nullptr ? "ON" : "OFF";
      show_test_log = znp_test_log == "ON";
    }

    if (show_test_log)
    {
      pad_nesting(get_nesting());
      std::cout << "[TEST_INFO] " << message << std::endl;
    }
  }

  // Execute a vector of hooks, returning false on failure.
  // On failure, error_message is populated with the details.
  bool execute_hooks(const std::vector<HOOK_WITH_OPTIONS> &hooks, const std::string &hook_type, std::string &error_message)
  {
    for (const auto &hook : hooks)
    {
      unsigned int timeout_seconds = hook.options.timeout_sec > 0 ? hook.options.timeout_sec : DEFAULT_TEST_TIMEOUT_SECONDS;

      bool abend = false;
      bool timeout = false;
      struct sigaction sa, timeout_sa, old_timeout_sa;

      timeout_occurred = false;

      if (0 != setjmp(get_jmp_buf()))
      {
        if (timeout_occurred)
        {
          timeout = true;
        }
        else
        {
          abend = true;
        }
      }

      if (!hook.options.remove_signal_handling)
      {
        setup_signal_handlers(sa);
      }

      setup_timeout_handler(timeout_sa, old_timeout_sa);

      if (!abend && !timeout)
      {
        alarm(timeout_seconds);

        try
        {
          hook.callback();
        }
        catch (const std::exception &e)
        {
          alarm(0);
          reset_timeout_handler(old_timeout_sa);
          if (!hook.options.remove_signal_handling)
          {
            reset_signal_handlers(sa);
          }
          error_message = hook_type + " hook failed: " + std::string(e.what());
          return false;
        }
        catch (...)
        {
          alarm(0);
          reset_timeout_handler(old_timeout_sa);
          if (!hook.options.remove_signal_handling)
          {
            reset_signal_handlers(sa);
          }
          error_message = hook_type + " hook failed with unknown error";
          return false;
        }

        alarm(0);
      }

      reset_timeout_handler(old_timeout_sa);
      if (!hook.options.remove_signal_handling)
      {
        reset_signal_handlers(sa);
      }

      if (timeout)
      {
        error_message = hook_type + " hook timed out after " + std::to_string(timeout_seconds) + " seconds";
        return false;
      }
      else if (abend)
      {
        error_message = hook_type + " hook failed with ABEND";
        return false;
      }
    }
    return true;
  }

  // Collect beforeEach hooks from current suite and all parent suites (parent -> child order)
  std::vector<HOOK_WITH_OPTIONS> collect_before_each_hooks()
  {
    std::vector<HOOK_WITH_OPTIONS> hooks;
    if (suite_stack.empty())
    {
      return hooks;
    }

    for (const auto &idx : suite_stack)
    {
      const auto &suite_hooks = suites[idx].before_each_hooks;
      hooks.insert(hooks.end(), suite_hooks.begin(), suite_hooks.end());
    }

    return hooks;
  }

  // Collect afterEach hooks from current suite and all parent suites (child -> parent order)
  std::vector<HOOK_WITH_OPTIONS> collect_after_each_hooks()
  {
    std::vector<HOOK_WITH_OPTIONS> hooks;
    if (suite_stack.empty())
    {
      return hooks;
    }

    for (auto it = suite_stack.rbegin(); it != suite_stack.rend(); ++it)
    {
      const auto &suite_hooks = suites[*it].after_each_hooks;
      hooks.insert(hooks.end(), suite_hooks.begin(), suite_hooks.end());
    }

    return hooks;
  }

  template <typename Callable,
            typename = typename std::enable_if<
                std::is_same<void, decltype(std::declval<Callable>()())>::value>::type>
  void run_test(const std::string &description, Callable test, TEST_OPTIONS &opts)
  {
    TEST_CASE tc = {0};
    tc.description = description;

    // Determine timeout to use (0 means use default)
    unsigned int timeout_seconds = opts.timeout_sec > 0 ? opts.timeout_sec : DEFAULT_TEST_TIMEOUT_SECONDS;

    int current_nesting = get_nesting();

    if (get_matcher() != "")
    {
      try
      {
        std::regex pattern(get_matcher());
        if (!std::regex_search(description, pattern))
        {
          return;
        }
      }
      catch (const std::regex_error &e)
      {
        // If regex is invalid, fall back to exact string match
        if (get_matcher() != description)
        {
          return;
        }
      }
    }

    int suite_idx = get_suite_index();

    // Check if the current suite already has a hook failure (skip remaining tests)
    if (suite_idx >= 0 && suite_idx < static_cast<int>(suites.size()) && suites[suite_idx].hook_failed)
    {
      tc.success = false;
      tc.fail_message = suites[suite_idx].hook_error;
      auto status = format_test_status(tc, false);
      print_test_output(tc, description, current_nesting, status.first, status.second);
      suites[suite_idx].tests.push_back(tc);
      return;
    }

    // Execute beforeAll hooks for the current suite stack (parent -> child order)
    for (const auto &idx : suite_stack)
    {
      if (idx < 0 || idx >= static_cast<int>(suites.size()))
      {
        continue;
      }

      TEST_SUITE &suite = suites[idx];
      if (suite.hook_failed)
      {
        tc.success = false;
        tc.fail_message = suite.hook_error;
        auto status = format_test_status(tc, false);
        print_test_output(tc, description, current_nesting, status.first, status.second);
        suites[suite_idx].tests.push_back(tc);
        return;
      }

      if (!suite.before_all_executed && !suite.before_all_hooks.empty())
      {
        suite.before_all_executed = true;
        std::string error_message;
        if (!execute_hooks(suite.before_all_hooks, "beforeAll", error_message))
        {
          suite.hook_failed = true;
          suite.hook_error = error_message;
          tc.success = false;
          tc.fail_message = error_message;
          auto status = format_test_status(tc, false);
          print_test_output(tc, description, current_nesting, status.first, status.second);
          suites[suite_idx].tests.push_back(tc);
          return;
        }
      }
    }

    // Collect and execute beforeEach hooks
    std::vector<HOOK_WITH_OPTIONS> before_each_hooks = collect_before_each_hooks();
    if (!before_each_hooks.empty())
    {
      std::string error_message;
      if (!execute_hooks(before_each_hooks, "beforeEach", error_message))
      {
        tc.success = false;
        tc.fail_message = error_message;

        // Mark suite as failed so remaining tests are skipped
        if (suite_idx >= 0 && suite_idx < static_cast<int>(suites.size()))
        {
          suites[suite_idx].hook_failed = true;
          suites[suite_idx].hook_error = error_message;
        }

        // Still run afterEach hooks even if beforeEach failed
        std::vector<HOOK_WITH_OPTIONS> after_each_hooks = collect_after_each_hooks();
        if (!after_each_hooks.empty())
        {
          std::string after_error;
          if (!execute_hooks(after_each_hooks, "afterEach", after_error))
          {
            tc.fail_message += " (afterEach also failed: " + after_error + ")";
          }
        }

        auto status = format_test_status(tc, false);
        print_test_output(tc, description, current_nesting, status.first, status.second);
        get_suites()[suite_idx].tests.push_back(tc);
        return;
      }
    }

    bool abend = false;
    bool timeout = false;
    struct sigaction sa;
    struct sigaction timeout_sa, old_timeout_sa;

    // Initialize timeout flag before setjmp
    timeout_occurred = false;

    tc.start_time = std::chrono::high_resolution_clock::now();

    // Call setjmp FIRST before setting up any signal handlers
    if (0 != setjmp(get_jmp_buf()))
    {
      if (timeout_occurred)
      {
        timeout = true;
      }
      else
      {
        abend = true;
      }
    }

    // Setup signal handlers AFTER setjmp to avoid race condition
    if (!opts.remove_signal_handling)
    {
      setup_signal_handlers(sa);
    }

    // Setup timeout handler (always active)
    setup_timeout_handler(timeout_sa, old_timeout_sa);

    if (!abend && !timeout)
    {
      alarm(timeout_seconds);
      execute_test(test, tc, abend);
      alarm(0);
    }

    tc.end_time = std::chrono::high_resolution_clock::now();

    // Reset timeout handler
    reset_timeout_handler(old_timeout_sa);

    if (!opts.remove_signal_handling)
    {
      reset_signal_handlers(sa);
    }

    if (timeout)
    {
      tc.success = false;
      tc.fail_message = "test timed out after " + std::to_string(timeout_seconds) + " seconds";
    }
    else if (abend)
    {
      tc.success = false;
      tc.fail_message = "unexpected ABEND occurred. Add `TEST_OPTIONS.remove_signal_handling = true` to `it(...)` to capture abend dump";
    }

    // Execute afterEach hooks (always run, even if test failed)
    std::vector<HOOK_WITH_OPTIONS> after_each_hooks = collect_after_each_hooks();
    if (!after_each_hooks.empty())
    {
      std::string error_message;
      if (!execute_hooks(after_each_hooks, "afterEach", error_message))
      {
        // Mark suite as failed so remaining tests are skipped
        if (suite_idx >= 0 && suite_idx < static_cast<int>(suites.size()))
        {
          suites[suite_idx].hook_failed = true;
          suites[suite_idx].hook_error = error_message;
        }

        if (tc.success)
        {
          tc.success = false;
          tc.fail_message = error_message;
        }
        else
        {
          tc.fail_message += " (afterEach also failed: " + error_message + ")";
        }
      }
    }

    auto status = format_test_status(tc, abend);
    print_test_output(tc, description, current_nesting, status.first, status.second);

    get_suites()[get_suite_index()].tests.push_back(tc);
  }
};

// Signal handlers need to be after Globals class definition
inline void signal_handler(int code, siginfo_t *info, void *context)
{
  Globals &g = Globals::get_instance();
  longjmp(g.get_jmp_buf(), 1);
}

inline void timeout_handler(int sig)
{
  Globals &g = Globals::get_instance();
  g.set_timeout_occurred(true);
  longjmp(g.get_jmp_buf(), 1);
}

template <class T>
class RESULT_CHECK
{
  T result;
  bool inverse;
  EXPECT_CONTEXT ctx;

  void fail_if(bool condition, const std::string &msg)
  {
    if (condition)
      throw std::runtime_error(msg + append_error_details());
  }

  static const size_t LARGE_STRING = 80;

  static bool verbose_strings()
  {
    return std::getenv("ZTEST_VERBOSE_STRINGS") != nullptr;
  }

  std::string string_diff_hint(const std::string &expected, const std::string &actual)
  {
    std::stringstream out;
    out << "sizes: expected=" << expected.size() << " actual=" << actual.size();
    out << "\nfirst 5 differences (byte offset: expected -> actual):";
    size_t len = std::max(expected.size(), actual.size());
    int found = 0;
    for (size_t i = 0; i < len && found < 5; ++i)
    {
      char ce = i < expected.size() ? expected[i] : '\0';
      char ca = i < actual.size() ? actual[i] : '\0';
      if (ce != ca)
      {
        out << "\n  [" << i << "]: "
            << (int)(unsigned char)ce << " (0x" << std::hex << (int)(unsigned char)ce << std::dec << ")"
            << " -> "
            << (int)(unsigned char)ca << " (0x" << std::hex << (int)(unsigned char)ca << std::dec << ")";
        ++found;
      }
    }
    if (found == 0)
      out << "\n  (strings are equal up to shorter length)";
    return out.str();
  }

  static std::string type_name_of()
  {
    int status;
    char *demangled = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status);
    std::string name = (status == 0) ? demangled : typeid(T).name();
    if (demangled)
      free(demangled);
    return name;
  }

public:
  void ToBe(T val)
  {
    if constexpr (std::is_same<T, std::string>::value)
    {
      bool large = !verbose_strings() && (result.size() >= LARGE_STRING || val.size() >= LARGE_STRING);
      if (inverse)
      {
        if (!large)
          fail_if(result == val, "expected string '" + result + "' NOT to be '" + val + "'");
        else
          fail_if(result == val, "strings unexpectedly equal\n" + string_diff_hint(val, result));
      }
      else
      {
        if (!large)
          fail_if(result != val, "expected string '" + result + "' to be '" + val + "'");
        else if (result != val)
          fail_if(true, "strings differ (too large to print)\n" + string_diff_hint(val, result));
      }
    }
    else
    {
      std::string name = type_name_of();
      std::stringstream ss;
      ss << result;
      std::stringstream ss2;
      ss2 << val;
      if (inverse)
        fail_if(result == val, "expected " + name + " '" + ss.str() + "' NOT to be '" + ss2.str() + "'");
      else
        fail_if(result != val, "expected " + name + " '" + ss.str() + "' to be '" + ss2.str() + "'");
    }
  }

  template <typename U = T>
  typename std::enable_if<std::is_pointer<U>::value>::type
  ToBeNull()
  {
    std::stringstream ss;
    ss << result;
    if (inverse)
      fail_if(nullptr == result, "expected '" + ss.str() + "' NOT to be null");
    else
      fail_if(nullptr != result, "expected pointer '" + ss.str() + "' to be null");
  }

  template <typename U = T>
  typename std::enable_if<std::is_arithmetic<U>::value>::type
  ToBeGreaterThan(U val)
  {
    std::string name = type_name_of();
    std::stringstream ss;
    ss << result;
    std::stringstream ss2;
    ss2 << val;
    if (inverse)
      fail_if(result > val, "expected " + name + " '" + ss.str() + "' NOT to be greater than '" + ss2.str() + "'");
    else
      fail_if(result <= val, "expected " + name + " '" + ss.str() + "' to be greater than '" + ss2.str() + "'");
  }

  template <typename U = T>
  typename std::enable_if<std::is_arithmetic<U>::value>::type
  ToBeGreaterThanOrEqualTo(U val)
  {
    std::string name = type_name_of();
    std::stringstream ss;
    ss << result;
    std::stringstream ss2;
    ss2 << val;
    if (inverse)
      fail_if(result >= val, "expected " + name + " '" + ss.str() + "' NOT to be greater than or equal to '" + ss2.str() + "'");
    else
      fail_if(result < val, "expected " + name + " '" + ss.str() + "' to be greater than or equal to '" + ss2.str() + "'");
  }

  template <typename U = T>
  typename std::enable_if<std::is_arithmetic<U>::value>::type
  ToBeLessThan(U val)
  {
    std::string name = type_name_of();
    std::stringstream ss;
    ss << result;
    std::stringstream ss2;
    ss2 << val;
    if (inverse)
      fail_if(result < val, "expected " + name + " '" + ss.str() + "' NOT to be less than '" + ss2.str() + "'");
    else
      fail_if(result >= val, "expected " + name + " '" + ss.str() + "' to be less than '" + ss2.str() + "'");
  }

  template <typename U = T>
  typename std::enable_if<std::is_same<U, std::string>::value>::type
  ToContain(const std::string &substring)
  {
    bool large = !verbose_strings() && result.size() >= LARGE_STRING;
    std::string snippet = large ? result.substr(0, LARGE_STRING) : result;
    std::string escaped;
    for (char c : snippet)
      escaped += (c == '\n') ? "\\n" : std::string(1, c);
    std::string haystack = large
                               ? escaped + "...(truncated, total " + std::to_string(result.size()) + " chars)"
                               : escaped;
    if (inverse)
      fail_if(result.find(substring) != std::string::npos, "expected string '" + haystack + "' to NOT contain '" + substring + "'");
    else
      fail_if(result.find(substring) == std::string::npos, "expected string '" + haystack + "' to contain '" + substring + "'");
  }

  std::string append_error_details()
  {
    std::string error = "";
    if (ctx.initialized)
    {
      error = "\n" + std::string(2, ' ') + "at " + ctx.file_name + ":" + std::to_string(ctx.line_number);
      if (ctx.message.size() > 0)
      {
        error += " (" + TrimChars(ctx.message) + ")";
      }
    }
    return error;
  }

  template <typename U = T>
  auto ToAbend() -> decltype(std::declval<U>()(), void())
  {
    Globals &g = Globals::get_instance();
    jmp_buf old_buf;
    std::memcpy(old_buf, g.get_jmp_buf(), sizeof(jmp_buf));

    bool abend = false;

    if (0 != setjmp(g.get_jmp_buf()))
    {
      if (g.get_timeout_occurred())
      {
        std::memcpy(g.get_jmp_buf(), old_buf, sizeof(jmp_buf));
        std::string error = "expected to ABEND, but timed out instead";
        error += append_error_details();
        throw std::runtime_error(error);
      }
      abend = true;
    }

    if (!abend)
    {
      result();
    }

    std::memcpy(g.get_jmp_buf(), old_buf, sizeof(jmp_buf));

    if (inverse && abend)
    {
      std::string error = "expected NOT to ABEND";
      error += append_error_details();
      throw std::runtime_error(error);
    }
    else if (!abend)
    {
      std::string error = "expected to ABEND";
      error += append_error_details();
      throw std::runtime_error(error);
    }
  }

  RESULT_CHECK Not()
  {
    RESULT_CHECK copy(result);
    copy.set_inverse(true);
    set_inverse(false);
    copy.set_context(ctx);
    return copy;
  }
  RESULT_CHECK(T r)
      : result(std::move(r)), inverse(false)
  {
  }
  ~RESULT_CHECK()
  {
  }

  void set_inverse(bool v)
  {
    inverse = v;
  }

  void set_context(EXPECT_CONTEXT &c)
  {
    ctx = c;
  }

  void set_result(T r)
  {
    result = r;
  }
};

typedef void (*cb)();

template <typename Callable,
          typename = typename std::enable_if<
              std::is_same<void, decltype(std::declval<Callable>()())>::value>::type>
void describe(const std::string &description, Callable suite)
{
  Globals &g = Globals::get_instance();
  TEST_SUITE ts;
  ts.description = description;
  ts.nesting_level = g.get_nesting();
  g.get_suites().push_back(ts);
  int current_suite_idx = static_cast<int>(g.get_suites().size()) - 1;
  g.push_suite_index(current_suite_idx);

  std::cout << get_indent(ts.nesting_level) << description << std::endl;
  g.increment_nesting();

  auto cleanup = [&]()
  {
    g.decrement_nesting();
    g.pop_suite_index();
  };

  // Execute suite (this is where beforeAll/beforeEach/afterEach/afterAll/it calls happen)
  try
  {
    suite();
  }
  catch (...)
  {
    cleanup();
    throw;
  }

  // Execute afterAll hooks for this suite
  if (current_suite_idx >= 0 && current_suite_idx < static_cast<int>(g.get_suites().size()))
  {
    TEST_SUITE &current_suite = g.get_suites()[current_suite_idx];
    const std::vector<HOOK_WITH_OPTIONS> &after_all_hooks = current_suite.after_all_hooks;
    if (!after_all_hooks.empty())
    {
      std::string error_message;
      if (!g.execute_hooks(after_all_hooks, "afterAll", error_message))
      {
        g.pad_nesting(g.get_nesting());
        std::cout << colors.red << colors.cross << " " << error_message << colors.reset << std::endl;
      }
    }
    current_suite.before_all_hooks.clear();
    current_suite.before_each_hooks.clear();
    current_suite.after_each_hooks.clear();
    current_suite.after_all_hooks.clear();
    current_suite.before_all_executed = false;
  }

  cleanup();
}

template <typename Callable,
          typename = typename std::enable_if<
              std::is_same<void, decltype(std::declval<Callable>()())>::value>::type>
void xit(const std::string &description, Callable)
{
  Globals &g = Globals::get_instance();
  int suite_idx = g.get_suite_index();

  TEST_CASE tc = {0};
  tc.description = description;
  tc.skipped = true;

  if (suite_idx >= 0 && suite_idx < static_cast<int>(g.get_suites().size()))
  {
    g.get_suites()[suite_idx].tests.push_back(tc);
  }

  std::cout << get_indent(g.get_nesting()) << colors.skip << " SKIP " << description << std::endl;
}

template <typename Callable,
          typename = typename std::enable_if<
              std::is_same<void, decltype(std::declval<Callable>()())>::value>::type>
void xdescribe(const std::string &description, Callable)
{
  Globals &g = Globals::get_instance();

  TEST_SUITE suite{};
  suite.description = description;
  suite.nesting_level = g.get_nesting();
  suite.skipped = true;
  g.get_suites().push_back(suite);

  std::cout << get_indent(g.get_nesting()) << colors.skip << " SKIP " << description << std::endl;
}

template <typename Callable,
          typename = typename std::enable_if<
              std::is_same<void, decltype(std::declval<Callable>()())>::value>::type>
void it(const std::string &description, Callable test)
{
  TEST_OPTIONS opts = {false, 0};
  it(description, test, opts);
}

template <typename Callable,
          typename = typename std::enable_if<
              std::is_same<void, decltype(std::declval<Callable>()())>::value>::type>
void it(const std::string &description, Callable test, TEST_OPTIONS &opts)
{
  Globals::get_instance().run_test(description, test, opts);
}

template <typename Callable,
          typename = typename std::enable_if<
              std::is_same<void, decltype(std::declval<Callable>()())>::value>::type>
void itif(const std::string &description, Callable test, bool should_run)
{
  TEST_OPTIONS opts = {false, 0};
  if (should_run)
  {
    it(description, test, opts);
  }
  else
  {
    xit(description, test);
  }
}

template <typename Callable,
          typename = typename std::enable_if<
              std::is_same<void, decltype(std::declval<Callable>()())>::value>::type>
void itif(const std::string &description, Callable test, TEST_OPTIONS &opts, bool should_run)
{
  if (should_run)
  {
    it(description, test, opts);
  }
  else
  {
    xit(description, test);
  }
}

template <typename T>
RESULT_CHECK<T> expect(T val, EXPECT_CONTEXT ctx = {0, "", "", false})
{
  RESULT_CHECK<T> res(std::move(val));
  res.set_context(ctx);
  return res;
}

// Hook registration functions
template <typename Callable,
          typename = typename std::enable_if<
              std::is_same<void, decltype(std::declval<Callable>()())>::value>::type>
void beforeAll(Callable hook)
{
  TEST_OPTIONS opts = {false, 0};
  beforeAll(hook, opts);
}

template <typename Callable,
          typename = typename std::enable_if<
              std::is_same<void, decltype(std::declval<Callable>()())>::value>::type>
void beforeAll(Callable hook, TEST_OPTIONS &opts)
{
  Globals &g = Globals::get_instance();
  int suite_idx = g.get_suite_index();
  if (suite_idx >= 0 && suite_idx < static_cast<int>(g.get_suites().size()))
  {
    HOOK_WITH_OPTIONS hook_with_opts = {hook, opts};
    g.get_suites()[suite_idx].before_all_hooks.push_back(hook_with_opts);
  }
}

template <typename Callable,
          typename = typename std::enable_if<
              std::is_same<void, decltype(std::declval<Callable>()())>::value>::type>
void beforeEach(Callable hook)
{
  TEST_OPTIONS opts = {false, 0};
  beforeEach(hook, opts);
}

template <typename Callable,
          typename = typename std::enable_if<
              std::is_same<void, decltype(std::declval<Callable>()())>::value>::type>
void beforeEach(Callable hook, TEST_OPTIONS &opts)
{
  Globals &g = Globals::get_instance();
  int suite_idx = g.get_suite_index();
  if (suite_idx >= 0 && suite_idx < static_cast<int>(g.get_suites().size()))
  {
    HOOK_WITH_OPTIONS hook_with_opts = {hook, opts};
    g.get_suites()[suite_idx].before_each_hooks.push_back(hook_with_opts);
  }
}

template <typename Callable,
          typename = typename std::enable_if<
              std::is_same<void, decltype(std::declval<Callable>()())>::value>::type>
void afterEach(Callable hook)
{
  TEST_OPTIONS opts = {false, 0};
  afterEach(hook, opts);
}

template <typename Callable,
          typename = typename std::enable_if<
              std::is_same<void, decltype(std::declval<Callable>()())>::value>::type>
void afterEach(Callable hook, TEST_OPTIONS &opts)
{
  Globals &g = Globals::get_instance();
  int suite_idx = g.get_suite_index();
  if (suite_idx >= 0 && suite_idx < static_cast<int>(g.get_suites().size()))
  {
    HOOK_WITH_OPTIONS hook_with_opts = {hook, opts};
    g.get_suites()[suite_idx].after_each_hooks.push_back(hook_with_opts);
  }
}

template <typename Callable,
          typename = typename std::enable_if<
              std::is_same<void, decltype(std::declval<Callable>()())>::value>::type>
void afterAll(Callable hook)
{
  TEST_OPTIONS opts = {false, 0};
  afterAll(hook, opts);
}

template <typename Callable,
          typename = typename std::enable_if<
              std::is_same<void, decltype(std::declval<Callable>()())>::value>::type>
void afterAll(Callable hook, TEST_OPTIONS &opts)
{
  Globals &g = Globals::get_instance();
  int suite_idx = g.get_suite_index();
  if (suite_idx >= 0 && suite_idx < static_cast<int>(g.get_suites().size()))
  {
    HOOK_WITH_OPTIONS hook_with_opts = {hook, opts};
    g.get_suites()[suite_idx].after_all_hooks.push_back(hook_with_opts);
  }
}

inline void print_failed_tests()
{
  Globals &g = Globals::get_instance();
  bool has_failures = false;

  // First pass: check if there are any failures
  for (const auto &suite : g.get_suites())
  {
    for (const auto &test : suite.tests)
    {
      if (!test.success && !test.skipped)
      {
        has_failures = true;
        break;
      }
    }
    if (has_failures)
      break;
  }

  if (!has_failures)
    return;

  std::cout << "\n======== FAILED TESTS ========" << std::endl;

  // Build a map of suite paths for proper nesting display
  std::vector<std::string> suite_paths;
  std::vector<int> suite_nesting_levels;

  for (const auto &suite : g.get_suites())
  {
    bool suite_has_failures = false;

    // Check if this suite has any failures
    for (const auto &test : suite.tests)
    {
      if (!test.success && !test.skipped)
      {
        suite_has_failures = true;
        break;
      }
    }

    if (suite_has_failures)
    {
      // Build the complete path for this suite
      std::string full_path = suite.description;
      int current_nesting = suite.nesting_level;

      // Find the most recent parent suite by looking at nesting levels
      // We need to find the last suite that has a lower nesting level
      for (int i = suite_paths.size() - 1; i >= 0; i--)
      {
        if (suite_nesting_levels[i] < current_nesting)
        {
          full_path = suite_paths[i] + " > " + suite.description;
          break;
        }
      }

      std::cout << colors.red << colors.cross << " FAIL " << full_path << colors.reset << std::endl;

      for (const auto &test : suite.tests)
      {
        if (!test.success && !test.skipped)
        {
          std::cout << "  " << colors.red << colors.cross << " FAIL " << test.description << colors.reset << std::endl;
          if (!test.fail_message.empty())
          {
            std::string main_error = test.fail_message;
            std::string context;
            size_t nl = main_error.find("\n");
            if (nl != std::string::npos)
            {
              context = main_error.substr(nl + 1);
              main_error = main_error.substr(0, nl);
            }
            std::cout << "    " << colors.arrow << " " << main_error << std::endl;
            if (!context.empty())
            {
              std::istringstream ctx_lines(context);
              std::string ctx_line;
              while (std::getline(ctx_lines, ctx_line))
                std::cout << "      " << ctx_line << std::endl;
            }
          }
        }
      }

      // Store this suite for future path building
      suite_paths.push_back(full_path);
      suite_nesting_levels.push_back(current_nesting);
    }
    else
    {
      // Even if this suite doesn't have failures, we need to track it for path building
      // This is important for nested suites that don't have failures themselves
      std::string full_path = suite.description;
      int current_nesting = suite.nesting_level;

      // Find the most recent parent suite
      for (int i = suite_paths.size() - 1; i >= 0; i--)
      {
        if (suite_nesting_levels[i] < current_nesting)
        {
          full_path = suite_paths[i] + " > " + suite.description;
          break;
        }
      }

      suite_paths.push_back(full_path);
      suite_nesting_levels.push_back(current_nesting);
    }
  }
}

inline int report()
{
  int suite_fail = 0;
  int suite_skip = 0;
  int suite_total = 0;
  int tests_total = 0;
  int tests_fail = 0;
  int tests_skip = 0;
  std::chrono::microseconds total_duration(0);

  Globals &g = Globals::get_instance();

  // Print failed tests summary before the main summary
  print_failed_tests();

  std::cout << "\n======== TESTS SUMMARY ========" << std::endl;

  for (const auto &suite : g.get_suites())
  {
    suite_total++;

    if (suite.skipped)
    {
      suite_skip++;
      continue;
    }

    bool suite_success = true;
    for (const auto &test : suite.tests)
    {
      tests_total++;
      if (test.skipped)
      {
        tests_skip++;
      }
      else if (!test.success)
      {
        suite_success = false;
        tests_fail++;
      }
      total_duration += std::chrono::duration_cast<std::chrono::microseconds>(test.end_time - test.start_time);
    }

    if (!suite_success)
    {
      suite_fail++;
    }
  }

  const int width = 13;
  int suite_pass = suite_total - suite_fail - suite_skip;
  std::cout << std::left << std::setw(width) << "Suites:";
  if (suite_fail > 0)
    std::cout << colors.red << suite_fail << " failed" << colors.reset << ", ";
  if (suite_skip > 0)
    std::cout << colors.yellow << suite_skip << " skipped" << colors.reset << ", ";
  if (suite_pass > 0)
    std::cout << colors.green << suite_pass << " passed" << colors.reset << ", ";
  std::cout << suite_total << " total" << std::endl;

  int tests_pass = tests_total - tests_fail - tests_skip;
  std::cout << std::left << std::setw(width) << "Tests:";
  if (tests_fail > 0)
    std::cout << colors.red << tests_fail << " failed" << colors.reset << ", ";
  if (tests_skip > 0)
    std::cout << colors.yellow << tests_skip << " skipped" << colors.reset << ", ";
  if (tests_pass > 0)
    std::cout << colors.green << tests_pass << " passed" << colors.reset << ", ";
  std::cout << tests_total << " total" << std::endl;

  std::cout << std::left << std::setw(width) << "Time:"
            << std::fixed << std::setprecision(3)
            << total_duration.count() / 1000000.0 << "s" << std::endl;

  return tests_fail > 0 ? 1 : 0;
}

inline void escape_xml(std::string &data)
{
  std::string result;
  result.reserve(data.size());

  for (auto byte : data)
  {
    auto ch = static_cast<unsigned char>(byte);
    switch (ch)
    {
    case '\"':
      result += "&quot;";
      break;
    case '&':
      result += "&amp;";
      break;
    case '\'':
      result += "&apos;";
      break;
    case '<':
      result += "&lt;";
      break;
    case '>':
      result += "&gt;";
      break;
    case '\t':
      result += "&#9;";
      break;
    case '\n':
      result += "&#10;";
      break;
    case '\r':
      result += "&#13;";
      break;
    default:
      if (std::iscntrl(ch))
      {
        char esc[9];
        std::snprintf(esc, sizeof(esc), "&#x%02X;", ch);
        result += esc;
      }
      else
      {
        result += byte;
      }
      break;
    }
  }

  data = std::move(result);
}

inline void report_xml(const std::string &filename = "test-results.xml")
{
  Globals &g = Globals::get_instance();
  std::ofstream xml_file(filename);

  int total_tests = 0;
  int total_failures = 0;
  int total_skipped = 0;
  std::chrono::microseconds total_time(0);

  // Count totals first
  for (const auto &suite : g.get_suites())
  {
    for (const auto &test : suite.tests)
    {
      total_tests++;
      if (test.skipped)
        total_skipped++;
      else if (!test.success)
        total_failures++;
      total_time += std::chrono::duration_cast<std::chrono::microseconds>(
          test.end_time - test.start_time);
    }
  }

  // XML Header
  xml_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  xml_file << "<testsuites tests=\"" << total_tests
           << "\" failures=\"" << total_failures
           << "\" skipped=\"" << total_skipped
           << "\" time=\"" << std::fixed << std::setprecision(3)
           << total_time.count() / 1000000.0 << "\">\n";

  // Build hierarchical paths for all suites
  std::vector<std::string> suite_paths;
  std::vector<int> suite_nesting_levels;

  for (const auto &suite : g.get_suites())
  {
    std::string full_path = suite.description;
    for (int i = suite_paths.size() - 1; i >= 0; i--)
    {
      if (suite_nesting_levels[i] < suite.nesting_level)
      {
        full_path = suite_paths[i] + " > " + suite.description;
        break;
      }
    }
    suite_paths.push_back(full_path);
    suite_nesting_levels.push_back(suite.nesting_level);
  }

  // Write test suites (one per top-level suite, containing all descendant tests)
  for (size_t i = 0; i < g.get_suites().size(); i++)
  {
    if (g.get_suites()[i].nesting_level != 0)
      continue;

    bool suite_is_skipped = g.get_suites()[i].skipped;

    // Collect all descendant tests with their paths
    std::vector<std::pair<std::string, const TEST_CASE *>> all_tests;
    if (!suite_is_skipped)
    {
      for (size_t j = i; j < g.get_suites().size(); j++)
      {
        if (j > i && g.get_suites()[j].nesting_level == 0)
          break;
        for (const auto &test : g.get_suites()[j].tests)
          all_tests.push_back(std::make_pair(suite_paths[j], &test));
      }

      if (all_tests.empty())
        continue;
    }

    // Calculate suite totals
    int suite_failures = 0;
    int suite_skipped = 0;
    std::chrono::microseconds suite_time(0);
    for (const auto &test_pair : all_tests)
    {
      if (test_pair.second->skipped)
        suite_skipped++;
      else if (!test_pair.second->success)
        suite_failures++;
      suite_time += std::chrono::duration_cast<std::chrono::microseconds>(
          test_pair.second->end_time - test_pair.second->start_time);
    }

    // Write suite header
    std::string suite_name = g.get_suites()[i].description;
    escape_xml(suite_name);

    if (suite_is_skipped)
    {
      // Skipped suite
      xml_file << "  <testsuite name=\"" << suite_name
               << "\" tests=\"0"
               << "\" failures=\"0"
               << "\" skipped=\"0"
               << "\" time=\"0.000\">\n";
    }
    else
    {
      xml_file << "  <testsuite name=\"" << suite_name
               << "\" tests=\"" << all_tests.size()
               << "\" failures=\"" << suite_failures
               << "\" skipped=\"" << suite_skipped
               << "\" time=\"" << std::fixed << std::setprecision(3)
               << suite_time.count() / 1000000.0 << "\">\n";
    }

    // Write test cases
    for (const auto &test_pair : all_tests)
    {
      std::string classname = test_pair.first;
      std::string test_name = test_pair.second->description;
      std::string failure_message = test_pair.second->fail_message;
      escape_xml(classname);
      escape_xml(test_name);
      escape_xml(failure_message);

      auto test_time = std::chrono::duration_cast<std::chrono::microseconds>(
          test_pair.second->end_time - test_pair.second->start_time);

      xml_file << "    <testcase classname=\"" << classname
               << "\" name=\"" << test_name
               << "\" time=\"" << std::fixed << std::setprecision(3)
               << test_time.count() / 1000000.0 << "\"";

      if (test_pair.second->skipped)
      {
        xml_file << ">\n";
        xml_file << "      <skipped/>\n";
        xml_file << "    </testcase>\n";
      }
      else if (!test_pair.second->success)
      {
        xml_file << ">\n";
        xml_file << "      <failure message=\"" << failure_message << "\"/>\n";
        xml_file << "    </testcase>\n";
      }
      else
      {
        xml_file << "/>\n";
      }
    }

    xml_file << "  </testsuite>\n";
  }

  xml_file << "</testsuites>\n";
  xml_file.close();
}

/**
 * Execute a command and capture stdout and stderr separately
 * @param command The command string to execute
 * @param stdout_output Reference to string that will contain stdout
 * @param stderr_output Reference to string that will contain stderr
 * @return Exit code of the executed command
 */
inline int execute_command(const std::string &command, std::string &stdout_output, std::string &stderr_output)
{
  stdout_output = "";
  stderr_output = "";

  // Create pipes for stdout and stderr
  int stdout_pipe[2], stderr_pipe[2];
  if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1)
  {
    throw std::runtime_error("Failed to create pipes");
  }

  // Fork the process
  pid_t pid = fork();
  if (pid == -1)
  {
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[0]);
    close(stderr_pipe[1]);
    throw std::runtime_error("Failed to fork process");
  }

  if (pid == 0)
  {
    // Child process
    close(stdout_pipe[0]); // Close read ends
    close(stderr_pipe[0]);

    // Redirect stdout and stderr to respective pipes
    dup2(stdout_pipe[1], STDOUT_FILENO);
    dup2(stderr_pipe[1], STDERR_FILENO);
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    // Execute command via shell
    execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
    _exit(127); // If execl fails
  }
  else
  {
    // Parent process
    close(stdout_pipe[1]); // Close write ends
    close(stderr_pipe[1]);

    // Set pipes to non-blocking
    fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(stderr_pipe[0], F_SETFL, O_NONBLOCK);

    // Read from both pipes using select
    fd_set read_fds;
    char buffer[256];
    ssize_t bytes_read;
    bool stdout_open = true, stderr_open = true;
    int status = 0;
    bool child_exited = false;

    while (stdout_open || stderr_open)
    {
      FD_ZERO(&read_fds);
      int max_fd = 0;

      if (stdout_open)
      {
        FD_SET(stdout_pipe[0], &read_fds);
        max_fd = std::max(max_fd, stdout_pipe[0]);
      }
      if (stderr_open)
      {
        FD_SET(stderr_pipe[0], &read_fds);
        max_fd = std::max(max_fd, stderr_pipe[0]);
      }

      struct timeval timeout = {1, 0}; // 1 second timeout
      int select_result = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);

      if (select_result > 0)
      {
        // Check stdout
        if (stdout_open && FD_ISSET(stdout_pipe[0], &read_fds))
        {
          bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer) - 1);
          if (bytes_read > 0)
          {
            buffer[bytes_read] = '\0';
            stdout_output += buffer;
          }
          else if (bytes_read == 0)
          {
            stdout_open = false;
          }
        }

        // Check stderr
        if (stderr_open && FD_ISSET(stderr_pipe[0], &read_fds))
        {
          bytes_read = read(stderr_pipe[0], buffer, sizeof(buffer) - 1);
          if (bytes_read > 0)
          {
            buffer[bytes_read] = '\0';
            stderr_output += buffer;
          }
          else if (bytes_read == 0)
          {
            stderr_open = false;
          }
        }
      }
      else if (select_result == 0)
      {
        // Timeout - check if child is still running
        if (waitpid(pid, &status, WNOHANG) != 0)
        {
          // Child has exited, do final reads
          child_exited = true;
          break;
        }
      }
      else
      {
        // Error in select
        break;
      }
    }

    // Final non-blocking reads to get any remaining data
    while ((bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer) - 1)) > 0)
    {
      buffer[bytes_read] = '\0';
      stdout_output += buffer;
    }
    while ((bytes_read = read(stderr_pipe[0], buffer, sizeof(buffer) - 1)) > 0)
    {
      buffer[bytes_read] = '\0';
      stderr_output += buffer;
    }

    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    // Wait for child process to complete if it didn't already exit
    if (!child_exited)
    {
      if (waitpid(pid, &status, 0) == -1)
      {
        throw std::runtime_error("Failed to wait for child process");
      }
    }

    // Return exit code
    if (WIFEXITED(status))
    {
      return WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status))
    {
      return 128 + WTERMSIG(status);
    }
    else
    {
      return -1;
    }
  }

  // Should never reach here, but add return to satisfy compiler
  return -1;
}

inline int tests(int argc, char *argv[], ztst::cb tests)
{
  std::cout << "======== TESTS ========" << std::endl;

  if (argc > 1)
  {
    std::cout << "Running tests matching: " << argv[1] << std::endl;
    Globals::get_instance().set_matcher(argv[1]);
  }
  else
  {
    std::cout << "Running all tests" << std::endl;
  }

  tests();
  int rc = report();
  report_xml();
  return rc;
}

} // namespace ztst

#endif

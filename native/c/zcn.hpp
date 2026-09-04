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

#ifndef ZCN_HPP
#define ZCN_HPP

#include <string>

#include "zcntype.h"

/**
 * @brief Build the default extended console name: the current user ID
 * truncated to 7 characters plus a rotating digit
 *
 * @param name string where the console name will be stored
 * @return int 0 for success; non zero if the user ID is unavailable
 */
int zcn_build_default_console_name(std::string &name);

/**
 * @brief Activate extended console
 * @note Prefer `ZcnSession` for RAII-managed usage. When using this API directly you MUST call `zcn_deactivate` to prevent resource leaks.
 * @param zcn extended console returned attributes and error information
 * @param name console name, max of 8 characters e.g. MYCONSOL
 * @return int 0 for success; non zero otherwise
 */
int zcn_activate(ZCN *zcn, const std::string &name);

/**
 * @brief Deactivate extended console
 *
 * @param zcn extended console returned attributes and error information
 * @return int 0 for success; non zero otherwise
 */
int zcn_deactivate(ZCN *zcn);

/**
 * @brief Write command to extended console
 *
 * @param zcn extended console returned attributes and error information
 * @param command command to run, e.g. D IPLINFO
 * @return int
 */
int zcn_put(ZCN *zcn, const std::string &command);

/**
 * @brief Obtain data from extended console
 *
 * @param zcn extended console returned attributes and error information
 * @param response command response
 * @return int
 */
int zcn_get(ZCN *zcn, std::string &response);

// Lightweight RAII helper that ensures `zcn_deactivate` is invoked.
class ZcnSession
{
public:
  ZcnSession()
      : active_(false), zcn_()
  {
  }

  ~ZcnSession()
  {
    deactivate();
  }

  ZcnSession(const ZcnSession &) = delete;
  ZcnSession &operator=(const ZcnSession &) = delete;
  ZcnSession(ZcnSession &&) = delete;
  ZcnSession &operator=(ZcnSession &&) = delete;

  /**
   * @brief Activates a console with the given name
   *
   * @param name The name of the console to activate
   * @return `0` (`RTNCD_SUCCESS`) if activation was successful, non-zero otherwise
   */
  int activate(const std::string &name)
  {
    if (active_)
    {
      int rc = deactivate();
      if (0 != rc)
      {
        return rc;
      }
    }

    int rc = zcn_activate(&zcn_, name);
    active_ = (0 == rc);
    return rc;
  }

  /**
   * @brief Deactivate the console session once finished
   *
   * @return `0` (`RTNCD_SUCCESS`) if activation was successful or already completed, non-zero otherwise
   */
  int deactivate()
  {
    if (!active_)
    {
      return 0;
    }

    int rc = zcn_deactivate(&zcn_);
    active_ = false;
    return rc;
  }

  /**
   * @brief Attempts to execute/put the given command in the console
   *
   * @param command The command to execute/put
   * @return `0` (`RTNCD_SUCCESS`) if successful, non-zero otherwise
   */
  int put(const std::string &command)
  {
    return zcn_put(&zcn_, command);
  }

  /**
   * @brief Obtain the response from the console after a command has been executed
   *
   * @param response The string variable to store the response text in
   * @return `0` (`RTNCD_SUCCESS`) if successful, non-zero otherwise
   */
  int get(std::string &response)
  {
    return zcn_get(&zcn_, response);
  }

  /**
   * @brief Whether the console session is active (console still activated)
   *
   * @return `true` if active, `false` otherwise
   */
  bool is_active() const
  {
    return active_;
  }

  /**
   * @brief Low-level structure containing diagnostic info and control-block specific variables
   *
   * @return Reference to said low-level structure
   */
  ZCN &control_block()
  {
    return zcn_;
  }

  /**
   * @brief Low-level structure containing diagnostic info and control-block specific variables
   *
   * @return Constant (read-only) reference to said low-level structure
   */
  const ZCN &control_block() const
  {
    return zcn_;
  }

private:
  bool active_;
  ZCN zcn_;
};

#endif

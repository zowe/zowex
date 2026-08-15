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

#ifndef RPCIO_HPP
#define RPCIO_HPP

#include "../extend/plugin.hpp"
#include "../zjson.hpp"
#include <map>
#include <memory>
#include <optional>
#include <sstream>

// JSON-RPC 2.0 Standard Error Codes
namespace RpcErrorCode
{
enum
{
  PARSE_ERROR = -32700,      // Invalid JSON was received
  INVALID_REQUEST = -32600,  // The JSON sent is not a valid Request object
  METHOD_NOT_FOUND = -32601, // The method does not exist / is not available
  INVALID_PARAMS = -32602,   // Invalid method parameter(s)
  INTERNAL_ERROR = -32603,   // Internal JSON-RPC error
  // -32000 to -32099 are reserved for implementation-defined server-errors
  REQUEST_TIMEOUT = -32001 // Request exceeded timeout limit
};
}

struct RpcNotification
{
  std::string jsonrpc;
  std::string method;
  std::optional<zjson::Value> params;
};
ZJSON_DERIVE(RpcNotification, jsonrpc, method, params);

struct RpcRequest : RpcNotification
{
  int id;
};
ZJSON_DERIVE(RpcRequest, jsonrpc, method, params, id);

struct ErrorDetails
{
  int code;
  std::string message;
  std::optional<zjson::Value> data;
};
ZJSON_DERIVE(ErrorDetails, code, message, data);

struct RpcResponse
{
  std::string jsonrpc;
  std::optional<zjson::Value> result;
  std::optional<ErrorDetails> error;
  std::optional<int> id;
};
ZJSON_SERIALIZABLE(RpcResponse,
                   ZJSON_FIELD(RpcResponse, jsonrpc),
                   ZJSON_FIELD(RpcResponse, result).skip_serializing_if_none(),
                   ZJSON_FIELD(RpcResponse, error).skip_serializing_if_none(),
                   ZJSON_FIELD(RpcResponse, id));

static constexpr size_t LARGE_DATA_THRESHOLD = 16 * 1024 * 1024; // 16MB

class MiddlewareContext : public plugin::InvocationContext
{
public:
  MiddlewareContext(const std::string &command_path, const plugin::ArgumentMap &args);

  // Get access to the string streams for reading/writing content
  std::stringstream &get_input_stream();
  std::stringstream &get_output_stream();
  std::stringstream &get_error_stream();

  // Helper methods to set input content and get output/error content
  void set_input_content(const std::string &content);
  std::string get_output_content() const;
  std::string get_error_content() const;

  // Provide mutable access to arguments for transforms
  plugin::ArgumentMap &mutable_arguments()
  {
    return m_args;
  }

  // Set content length and send pending notification if present
  void set_content_len(size_t content_length);

  // Store pending notification for delayed sending
  void set_pending_notification(const RpcNotification &notification);

  // Get large data map
  std::map<std::string, std::string> &get_large_data()
  {
    return m_large_data;
  }

  // Store large data to optimize JSON serialization
  void store_large_data(const std::string &field_name, const std::string &data);

private:
  std::stringstream m_input_stream;
  std::stringstream m_output_stream;
  std::stringstream m_error_stream;
  std::unique_ptr<RpcNotification> m_pending_notification;
  std::map<std::string, std::string> m_large_data;
};

#endif

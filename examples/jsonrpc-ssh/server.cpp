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

#include <cctype>
#include <cstdio>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>

// Naive JSON helpers - just enough for flat objects with string/int values.
// Not a general-purpose parser; only covers the JSON-RPC 2.0 subset used here.

namespace json
{

using Object = std::unordered_map<std::string, std::string>;

static void skip_ws(const std::string &s, size_t &i)
{
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
        ++i;
}

static std::optional<std::string> parse_string(const std::string &s, size_t &i)
{
    if (i >= s.size() || s[i] != '"')
        return std::nullopt;
    ++i;
    std::string out;
    while (i < s.size() && s[i] != '"')
    {
        if (s[i] == '\\' && i + 1 < s.size())
        {
            ++i;
            switch (s[i])
            {
            case '"':
                out += '"';
                break;
            case '\\':
                out += '\\';
                break;
            case '/':
                out += '/';
                break;
            case 'n':
                out += '\n';
                break;
            case 't':
                out += '\t';
                break;
            case 'r':
                out += '\r';
                break;
            default:
                out += s[i];
                break;
            }
        }
        else
        {
            out += s[i];
        }
        ++i;
    }
    if (i >= s.size())
        return std::nullopt;
    ++i; // closing quote
    return out;
}

// Parse a raw JSON token (number, true, false, null) or a nested object/array
// as a raw string. Nested structures are captured verbatim.
static std::optional<std::string> parse_raw_value(const std::string &s, size_t &i)
{
    skip_ws(s, i);
    if (i >= s.size())
        return std::nullopt;

    if (s[i] == '{' || s[i] == '[')
    {
        char open = s[i];
        char close = (open == '{') ? '}' : ']';
        int depth = 1;
        size_t start = i;
        ++i;
        bool in_str = false;
        while (i < s.size() && depth > 0)
        {
            if (in_str)
            {
                if (s[i] == '\\')
                    ++i;
                else if (s[i] == '"')
                    in_str = false;
            }
            else
            {
                if (s[i] == '"')
                    in_str = true;
                else if (s[i] == open)
                    ++depth;
                else if (s[i] == close)
                    --depth;
            }
            ++i;
        }
        return s.substr(start, i - start);
    }

    size_t start = i;
    while (i < s.size() && s[i] != ',' && s[i] != '}' && s[i] != ']' && !std::isspace(static_cast<unsigned char>(s[i])))
        ++i;
    return s.substr(start, i - start);
}

// Parse a flat-ish JSON object. All values are stored as raw strings
// (quoted strings have their quotes stripped; numbers/bools/objects kept verbatim).
static std::optional<Object> parse_object(const std::string &s)
{
    size_t i = 0;
    skip_ws(s, i);
    if (i >= s.size() || s[i] != '{')
        return std::nullopt;
    ++i;

    Object obj;
    skip_ws(s, i);
    if (i < s.size() && s[i] == '}')
        return obj;

    while (i < s.size())
    {
        skip_ws(s, i);
        auto key = parse_string(s, i);
        if (!key)
            return std::nullopt;
        skip_ws(s, i);
        if (i >= s.size() || s[i] != ':')
            return std::nullopt;
        ++i;
        skip_ws(s, i);

        if (i < s.size() && s[i] == '"')
        {
            auto val = parse_string(s, i);
            if (!val)
                return std::nullopt;
            obj[*key] = *val;
        }
        else
        {
            auto val = parse_raw_value(s, i);
            if (!val)
                return std::nullopt;
            obj[*key] = *val;
        }

        skip_ws(s, i);
        if (i < s.size() && s[i] == ',')
        {
            ++i;
            continue;
        }
        break;
    }
    return obj;
}

static std::string escape(const std::string &s)
{
    std::string out;
    for (char c : s)
    {
        switch (c)
        {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += c;
        }
    }
    return out;
}

} // namespace json

// JSON-RPC helpers

static void send_response(const std::string &json)
{
    std::cout << json << std::endl;
}

static void send_error(int id, int code, const std::string &message)
{
    send_response("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":" + std::to_string(code) +
                  ",\"message\":\"" + json::escape(message) + "\"},\"id\":" + std::to_string(id) + "}");
}

static void send_result(int id, const std::string &result_json)
{
    send_response("{\"jsonrpc\":\"2.0\",\"result\":" + result_json + ",\"id\":" + std::to_string(id) + "}");
}

static void handle_add(int id, const json::Object &params_obj)
{
    auto a_it = params_obj.find("a");
    auto b_it = params_obj.find("b");
    if (a_it == params_obj.end() || b_it == params_obj.end())
    {
        send_error(id, -32602, "Invalid parameters");
        return;
    }

    int a = std::stoi(a_it->second);
    int b = std::stoi(b_it->second);
    send_result(id, "{\"sum\":" + std::to_string(a + b) + "}");
}

int main()
{
    std::fprintf(stderr, "JSON-RPC server started...\n");

    std::string line;
    while (std::getline(std::cin, line))
    {
        if (line.empty())
            continue;

        auto parsed = json::parse_object(line);
        if (!parsed)
        {
            send_error(0, -32700, "Parse error");
            continue;
        }

        auto &obj = *parsed;
        int id = 0;
        if (auto it = obj.find("id"); it != obj.end())
            id = std::stoi(it->second);

        auto method_it = obj.find("method");
        if (method_it == obj.end())
        {
            send_error(id, -32600, "Invalid request");
            continue;
        }

        const std::string &method = method_it->second;

        // Parse the nested params object if present
        json::Object params_obj;
        if (auto it = obj.find("params"); it != obj.end())
        {
            auto p = json::parse_object(it->second);
            if (p)
                params_obj = *p;
        }

        if (method == "Arith.Add")
        {
            handle_add(id, params_obj);
        }
        else
        {
            send_error(id, -32601, "Method not found");
        }
    }

    std::fprintf(stderr, "JSON-RPC server stopped.\n");
    return 0;
}

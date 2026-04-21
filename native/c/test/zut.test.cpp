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

#include <cstdio>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "ztest.hpp"
#include "zut.hpp"
#include "../zlogger.hpp"
#include "zstorage.metal.test.h"

using namespace ztst;

void zut_tests()
{

  describe("zut tests",
           []() -> void
           {
             it("should run shell program", []() -> void
                {
                  std::string stdout_response;
                  std::string stderr_response;
                  std::string args = "line1\nline2\nline3";
                  std::string shell_command = "printf \"" + args + "\"";

                  int rc = zut_spawn_shell_command(shell_command, stdout_response, stderr_response);
                  ExpectWithContext(rc, stderr_response).ToBe(0);
                  ExpectWithContext(stdout_response, "Expected all lines").ToBe(args);
                  ExpectWithContext(stderr_response, "std err should be empty").ToBe(""); 

                  shell_command = "fakeutility";
                  rc = zut_spawn_shell_command(shell_command, stdout_response, stderr_response);
                  ExpectWithContext(rc, stdout_response).ToBe(127);
                  ExpectWithContext(stderr_response, "Expecting an error").ToContain("fakeutility: FSUM7351 not found");

                  shell_command = "";
                  rc = zut_spawn_shell_command(shell_command, stdout_response, stderr_response);
                  ExpectWithContext(rc, stdout_response).ToBe(-1);
                  ExpectWithContext(stderr_response, "Expecting an error").ToContain("You must specify a program to run."); });

             it("should run programs", []() -> void
                {
                  std::string response;
                  std::string program = "printf";
                  std::vector<std::string> args = {"line1\nline2\nline3"};

                  int rc = zut_run_program(program, args, response);
                  ExpectWithContext(rc, response).ToBe(0);
                  ExpectWithContext(response, "Expected all lines").ToBe(args[0]);

                  program = "/bin/printf";
                  rc = zut_run_program(program, args, response);
                  ExpectWithContext(rc, response).ToBe(0);
                  ExpectWithContext(response, "Expected all lines").ToBe(args[0]);

                  program = "fakeutility";
                  rc = zut_run_program(program, {}, response);
                  ExpectWithContext(rc, response).ToBe(-1);
                  ExpectWithContext(response, "Expecting an error").ToContain("zut_private_run_program: fakeutility: command not found");

                  program = "";
                  rc = zut_run_program(program, {}, response);
                  ExpectWithContext(rc, response).ToBe(-1);
                  ExpectWithContext(response, "Expecting an error").ToContain("You must specify a program to run.");

                  // now with split stdout/stderr
                  std::string stdout_data, stderr_data;
                  program = "printf";

                  rc = zut_run_program(program, args, stdout_data, stderr_data);
                  ExpectWithContext(rc, stderr_data).ToBe(0);
                  ExpectWithContext(stdout_data, "Expected all lines").ToBe(args[0]);

                  program = "/bin/printf";
                  rc = zut_run_program(program, args, stdout_data, stderr_data);
                  ExpectWithContext(rc, stderr_data).ToBe(0);
                  ExpectWithContext(stdout_data, "Expected all lines").ToBe(args[0]);

                  program = "fakeutility";
                  rc = zut_run_program(program, {}, stdout_data, stderr_data);
                  ExpectWithContext(rc, stderr_data).ToBe(-1);
                  ExpectWithContext(stderr_data, "Expecting an error").ToContain("zut_private_run_program: fakeutility: command not found");
                  Expect(stdout_data).ToBe("");

                  program = "";
                  rc = zut_run_program(program, {}, stdout_data, stderr_data);
                  ExpectWithContext(rc, stderr_data).ToBe(-1);
                  ExpectWithContext(stderr_data, "Expecting an error").ToContain("You must specify a program to run.");
                  Expect(stdout_data).ToBe(""); });

             // Tests if a semicolon can break out of the command
             it("test semicolon injection", []() -> void
                {
                  std::string response;
                  std::vector<std::string> args = {"line1\nline2; echo INJECTED_PAYLOAD"};

                  int rc = zut_run_program("echo", args, response);
                  ExpectWithContext(rc, response).ToBe(0);

                  // If vulnerable, the shell would execute `echo INJECTED_PAYLOAD` and the output would just be "INJECTED_PAYLOAD\n"
                  ExpectWithContext(response.find("line2; echo INJECTED_PAYLOAD"), "Expected the semicolon and second command to be treated as literal text").Not().ToBe(std::string::npos);

                  // same case as above, with split args
                  args.clear();
                  args = {"line1\nline2;", "echo", "INJECTED_PAYLOAD"};

                  rc = zut_run_program("echo", args, response);

                  ExpectWithContext(rc, response).ToBe(0);
                  ExpectWithContext(response.find("line2; echo INJECTED_PAYLOAD"), "Expected the semicolon and second command to be treated as literal text").Not().ToBe(std::string::npos); });

             // Tests if pipes can output to another command or modify the filesystem
             it("test pipe and redirect injection", []() -> void
                {
                std::string response;
                std::vector<std::string> args = {"line1\nline2 | grep line > /tmp/hacked.txt"};

                int rc = zut_run_program("echo", args, response);

                // If vulnerable, this would create a file and output nothing. Instead we print everything
                ExpectWithContext(rc, response).ToBe(0);
                ExpectWithContext(response.find("| grep"), "Expected pipe and redirect to be treated as literal text").Not().ToBe(std::string::npos);

                // Same case, split up args
                response = "";
                args.clear();
                args = {"line1\nline2", "|", "grep", "line", ">", "/tmp/hacked.txt"};

                rc = zut_run_program("echo", args, response);

                ExpectWithContext(rc, response).ToBe(0);
                ExpectWithContext(response.find("| grep"), "Expected pipe and redirect to be treated as literal text").Not().ToBe(std::string::npos); });

             it("should upper case and truncate a long string",
                []() -> void
                {
                  char buffer[9] = {0};
                  std::string data = "lowercaselongstring";
                  zut_uppercase_pad_truncate(buffer, data, sizeof(buffer) - 1);
                  expect(std::string(buffer)).ToBe("LOWERCAS");
                });

             it("should upper case and pad a short string",
                []() -> void
                {
                  char buffer[9] = {0};
                  std::string data = "abc";
                  zut_uppercase_pad_truncate(buffer, data, sizeof(buffer) - 1);
                  expect(std::string(buffer)).ToBe("ABC     ");
                });

             describe("zut_run_program pipe draining",
                      []() -> void
                      {
                        it("should capture stdout and stderr in separate streams", []() -> void
                           {
                             std::string stdout_data, stderr_data;
                             std::string program = "sh";
                             std::vector<std::string> args = {"-c", "echo STDOUT_LINE; echo STDERR_LINE >&2"};

                             int rc = zut_run_program(program, args, stdout_data, stderr_data);
                             ExpectWithContext(rc, stderr_data).ToBe(0);
                             ExpectWithContext(stdout_data, "stdout should contain stdout output").ToContain("STDOUT_LINE");
                             ExpectWithContext(stderr_data, "stderr should contain stderr output").ToContain("STDERR_LINE");
                             ExpectWithContext(stdout_data, "stdout should not contain stderr output").Not().ToContain("STDERR_LINE");
                             ExpectWithContext(stderr_data, "stderr should not contain stdout output").Not().ToContain("STDOUT_LINE"); });

                        it("should fully drain stdout output larger than the pipe buffer", []() -> void
                           {
                             std::string stdout_data, stderr_data;
                             std::string program = "sh";
                             std::vector<std::string> args = {"-c", "awk 'BEGIN { for (i = 0; i < 8000; i++) printf \"A\" }'"};

                             int rc = zut_run_program(program, args, stdout_data, stderr_data);
                             ExpectWithContext(rc, stderr_data).ToBe(0);
                             ExpectWithContext(static_cast<int>(stdout_data.size()), "stdout should be 8000 bytes").ToBe(8000);
                             Expect(stderr_data).ToBe(""); });

                        it("should fully drain output via single-stream (merged) overload", []() -> void
                           {
                             std::string response;
                             std::string program = "sh";
                             std::vector<std::string> args = {"-c", "awk 'BEGIN { for (i = 0; i < 8000; i++) printf \"B\" }'"};

                             int rc = zut_run_program(program, args, response);
                             ExpectWithContext(rc, response).ToBe(0);
                             ExpectWithContext(static_cast<int>(response.size()), "response should be 8000 bytes").ToBe(8000); });
                      });

             describe("zut_list_parmlib",
                      []() -> void
                      {
                        it("should list parmlib data sets",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::vector<std::string> parmlibs;
                             zut_list_parmlib(diag, parmlibs);
                             expect(parmlibs.size()).ToBeGreaterThan(0);
                           });
                      });
             describe("zut_bpxwdyn",
                      []() -> void
                      {
                        ZLOG_DEBUG("zut.test: describe zut_bpxwdyn");
                        it("should allocate a sysout data set and get the DS name",
                           []() -> void
                           {
                             ZLOG_DEBUG("zut.test: it allocate sysout");
                             std::string cmd = "ALLOC SYSOUT";
                             unsigned int code = 0;
                             std::string dsname = "";
                             std::string resp = "";
                             int rc = zut_bpxwdyn_rtdsn(cmd, &code, resp, dsname);
                             expect(rc).ToBe(0);
                             expect(dsname.size()).ToBeGreaterThan(0);
                             expect(code).ToBe(0);
                             ZLOG_DEBUG("zut.test: it allocate sysout done");
                           });

                        it("should allocate a data set, get the DD name, and free it",
                           []() -> void
                           {
                             ZLOG_DEBUG("zut.test: it alloc DA SYS1.MACLIB");
                             std::string cmd = "ALLOC DA('SYS1.MACLIB') SHR";
                             unsigned int code = 0;
                             std::string ddname = "";
                             std::string resp = "";
                             int rc = zut_bpxwdyn_rtdd(cmd, &code, resp, ddname);
                             expect(rc).ToBe(0);
                             expect(ddname.size()).ToBeGreaterThan(0);
                             expect(code).ToBe(0);

                             cmd = "FREE DD(" + ddname + ")";
                             rc = zut_bpxwdyn(cmd, &code, resp);
                             expect(rc).ToBe(0);
                             expect(code).ToBe(0);
                           });
                      });

             describe("zut_read_input",
                      []() -> void
                      {
                        it("should read all content from a std::stringstream (simulates non-TTY piped input)",
                           []() -> void
                           {
                             std::string input_data = "line1\nline2\nline3";
                             std::istringstream input_stream(input_data);

                             std::string result = zut_read_input(input_stream);

                             expect(result).ToBe(input_data);
                           });

                        it("should handle empty input stream",
                           []() -> void
                           {
                             std::istringstream input_stream("");

                             std::string result = zut_read_input(input_stream);

                             expect(result).ToBe("");
                             expect(result.length()).ToBe(0);
                           });

                        it("should preserve binary data with null bytes",
                           []() -> void
                           {
                             std::string input_data = "before\0after";
                             input_data[6] = '\0'; // Ensure null byte is present
                             std::istringstream input_stream(input_data);

                             std::string result = zut_read_input(input_stream);

                             expect(result.length()).ToBe(input_data.length());
                             expect(result[6]).ToBe('\0');
                           });

                        it("should handle single line without newline",
                           []() -> void
                           {
                             std::string input_data = "single line no newline";
                             std::istringstream input_stream(input_data);

                             std::string result = zut_read_input(input_stream);

                             expect(result).ToBe(input_data);
                           });

                        it("should handle content with trailing newline",
                           []() -> void
                           {
                             std::string input_data = "line with trailing newline\n";
                             std::istringstream input_stream(input_data);

                             std::string result = zut_read_input(input_stream);

                             expect(result).ToBe(input_data);
                           });

                        it("should handle multiple consecutive newlines",
                           []() -> void
                           {
                             std::string input_data = "line1\n\n\nline2";
                             std::istringstream input_stream(input_data);

                             std::string result = zut_read_input(input_stream);

                             expect(result).ToBe(input_data);
                           });

                        it("should handle Windows-style line endings",
                           []() -> void
                           {
                             std::string input_data = "line1\r\nline2\r\nline3";
                             std::istringstream input_stream(input_data);

                             std::string result = zut_read_input(input_stream);

                             expect(result).ToBe(input_data);
                           });

                        it("should handle large input",
                           []() -> void
                           {
                             std::string input_data;
                             for (int i = 0; i < 1000; i++)
                             {
                               input_data += "Line " + std::to_string(i) + " with some content\n";
                             }
                             std::istringstream input_stream(input_data);

                             std::string result = zut_read_input(input_stream);

                             expect(result).ToBe(input_data);
                             expect(result.length()).ToBe(input_data.length());
                           });
                      });

             describe("zut_encode",
                      []() -> void
                      {
                        it("should return input unchanged when source and target encodings are the same",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             // Default std::string literals are IBM-1047 on z/OS
                             std::string input = "Hello World";
                             std::string result = zut_encode(input, "IBM-1047", "IBM-1047", diag);

                             expect(result).ToBe("Hello World");
                             expect(result.length()).ToBe(input.length());
                             expect(diag.e_msg_len).ToBe(0);
                           });

                        it("should handle ASCII to EBCDIC conversion with known byte sequences",
                           []() -> void
                           {
                             ZDIAG diag = {0};

                             // Create ASCII "HELLO" using known byte values: 0x48 0x45 0x4C 0x4C 0x4F
                             std::vector<char> ascii_hello = {0x48, 0x45, 0x4C, 0x4C, 0x4F}; // "HELLO" in ASCII

                             // Convert ASCII to EBCDIC
                             std::vector<char> ebcdic_result = zut_encode(ascii_hello.data(), ascii_hello.size(), "ISO8859-1", "IBM-1047", diag);

                             expect(diag.e_msg_len).ToBe(0);
                             expect(ebcdic_result.size()).ToBe(5);

                             // Convert back to ASCII to verify round-trip
                             std::vector<char> ascii_roundtrip = zut_encode(ebcdic_result.data(), ebcdic_result.size(), "IBM-1047", "ISO8859-1", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(ascii_roundtrip.size()).ToBe(5);

                             // Verify original ASCII bytes are preserved
                             for (size_t i = 0; i < ascii_hello.size(); i++)
                             {
                               expect(ascii_roundtrip[i]).ToBe(ascii_hello[i]);
                             }
                           });

                        it("should handle conversion failure gracefully with invalid encoding",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input = "Test data";
                             std::string result = zut_encode(input, "INVALID-ENCODING", "UTF-8", diag);

                             expect(result).ToBe("");
                             expect(diag.e_msg_len).ToBeGreaterThan(0);
                             expect(std::string(diag.e_msg)).ToContain("Cannot open converter");
                           });

                        it("should handle round-trip conversion without data loss",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string original = "Round trip test data 12345";

                             // Convert IBM-1047 -> ISO8859-1 -> IBM-1047
                             std::string step1 = zut_encode(original, "IBM-1047", "ISO8859-1", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             std::string step2 = zut_encode(step1, "ISO8859-1", "IBM-1047", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             expect(step2).ToBe(original);
                             expect(step2.length()).ToBe(original.length());
                           });

                        it("should handle empty string conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input = "";
                             std::string result = zut_encode(input, "UTF-8", "ISO8859-1", diag);

                             expect(result).ToBe("");
                             expect(result.length()).ToBe(0);
                             expect(diag.e_msg_len).ToBe(0);
                           });

                        it("should handle single character conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input = "A";
                             std::string result = zut_encode(input, "IBM-1047", "ISO8859-1", diag);

                             expect(result).ToBe(std::string(1, 0x41)); // "A" in ISO8859-1
                             expect(result.length()).ToBe(1);
                             expect(diag.e_msg_len).ToBe(0);
                           });

                        it("should handle numeric content conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input = "1234567890";
                             std::string result = zut_encode(input, "IBM-1047", "ISO8859-1", diag);
                             char iso8859_1_1234567890[10] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30};

                             expect(result).ToBe(std::string(iso8859_1_1234567890, 10));
                             expect(result.length()).ToBe(10);
                             expect(diag.e_msg_len).ToBe(0);
                           });

                        it("should handle line breaks and whitespace",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input = "Line 1\nLine 2\r\nLine 3\tTabbed";
                             std::string result = zut_encode(input, "IBM-1047", "ISO8859-1", diag);
                             char iso8859_1_line_breaks[29] = {0x4C, 0x69, 0x6E, 0x65, 0x20, 0x31, 0x0A, 0x4C, 0x69, 0x6E, 0x65, 0x20, 0x32, 0x0D, 0x0A, 0x4C, 0x69, 0x6E, 0x65, 0x20, 0x33, 0x09, 0x54, 0x61, 0x62, 0x62, 0x65, 0x64};

                             expect(result).ToBe(iso8859_1_line_breaks);
                             expect(result.length()).ToBe(input.length());
                             expect(diag.e_msg_len).ToBe(0);
                           });

                        it("should preserve data integrity for longer text",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input = "This is a longer piece of text that contains multiple words, "
                                                 "punctuation marks, numbers like 123 and 456, and various symbols "
                                                 "such as @#$%^&*()_+ to test that the encoding conversion maintains "
                                                 "data integrity across larger content blocks.";
                             std::string result = zut_encode(input, "IBM-1047", "ISO8859-1", diag);
                             std::string result_original = zut_encode(result, "ISO8859-1", "IBM-1047", diag);

                             expect(result_original).ToBe(input);
                             expect(result.length()).ToBe(input.length());
                             expect(diag.e_msg_len).ToBe(0);
                           });

                        it("should handle conversion failure gracefully with invalid encoding",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input = "Test data";
                             std::string result = zut_encode(input, "INVALID-ENCODING", "UTF-8", diag);

                             expect(result).ToBe("");
                             expect(diag.e_msg_len).ToBeGreaterThan(0);
                             expect(std::string(diag.e_msg)).ToContain("Cannot open converter");
                           });

                        it("should handle target encoding failure gracefully",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input = "Test data";
                             std::string result = zut_encode(input, "UTF-8", "INVALID-TARGET", diag);

                             expect(result).ToBe("");
                             expect(diag.e_msg_len).ToBeGreaterThan(0);
                             expect(std::string(diag.e_msg)).ToContain("Cannot open converter");
                           });

                        it("should handle std::vector conversion with same encodings",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input = "Same encoding vector test";

                             std::vector<char> result = zut_encode(input.data(), input.size(), "UTF-8", "UTF-8", diag);

                             expect(diag.e_msg_len).ToBe(0);
                             expect(result.size()).ToBe(input.length());

                             std::string result_str(result.begin(), result.end());
                             expect(result_str).ToBe(input);
                           });

                        it("should handle binary data through encoding conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};

                             // Create test data with null bytes and binary content
                             std::vector<char> binary_input = {'H', 'e', 'l', 'l', 'o', '\0', 'W', 'o', 'r', 'l', 'd'};

                             std::vector<char> result = zut_encode(binary_input.data(), binary_input.size(), "UTF-8", "UTF-8", diag);

                             expect(diag.e_msg_len).ToBe(0);
                             expect(result.size()).ToBe(binary_input.size());

                             // Verify binary content is preserved
                             for (size_t i = 0; i < binary_input.size(); i++)
                             {
                               expect(result[i]).ToBe(binary_input[i]);
                             }
                           });

                        it("should handle UTF-8 to EBCDIC conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};

                             // Create UTF-8 "Hello, world!" using known byte values
                             // UTF-8 bytes for "Hello, world!": 0x48 0x65 0x6C 0x6C 0x6F 0x2C 0x20 0x77 0x6F 0x72 0x6C 0x64 0x21
                             std::vector<char> utf8_hello_world = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C, 0x20, 0x77, 0x6F, 0x72, 0x6C, 0x64, 0x21};

                             // Convert UTF-8 to EBCDIC (IBM-1047)
                             std::vector<char> ebcdic_result = zut_encode(utf8_hello_world.data(), utf8_hello_world.size(), "UTF-8", "IBM-1047", diag);

                             expect(diag.e_msg_len).ToBe(0);
                             expect(ebcdic_result.size()).ToBe(13); // "Hello, world!" is 13 characters

                             // Convert back to UTF-8 to verify round-trip
                             std::vector<char> utf8_roundtrip = zut_encode(ebcdic_result.data(), ebcdic_result.size(), "IBM-1047", "UTF-8", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(utf8_roundtrip.size()).ToBe(13);

                             // Verify original UTF-8 bytes are preserved
                             for (size_t i = 0; i < utf8_hello_world.size(); i++)
                             {
                               expect(utf8_roundtrip[i]).ToBe(utf8_hello_world[i]);
                             }
                           });

                        it("should handle UTF-8 to UCS-2 conversion with emoji",
                           []() -> void
                           {
                             ZDIAG diag = {0};

                             // UTF-8 bytes for emoji characters:
                             std::vector<char> utf8_emoji = {
                                 0xF0, 0x9F, 0xA6, 0x80, // 🦀
                                 0xF0, 0x9F, 0x8F, 0x86, // 🏆
                                 0xF0, 0x9F, 0x98, 0x8F, // 😏
                                 0xE2, 0x98, 0x95,       // ☕
                                 0xF0, 0x9F, 0x92, 0xA4  // 💤
                             };

                             // Convert UTF-8 to UCS-2
                             std::vector<char> ucs2_result = zut_encode(utf8_emoji.data(), utf8_emoji.size(), "UTF-8", "UCS-2", diag);

                             expect(diag.e_msg_len).ToBe(0);
                             expect(ucs2_result.size()).ToBeGreaterThan(0); // UCS-2 will have different size

                             // Convert back to UTF-8 to verify round-trip
                             std::vector<char> utf8_roundtrip = zut_encode(ucs2_result.data(), ucs2_result.size(), "UCS-2", "UTF-8", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(utf8_roundtrip.size()).ToBe(utf8_emoji.size());

                             // Verify original UTF-8 bytes are preserved
                             for (size_t i = 0; i < utf8_emoji.size(); i++)
                             {
                               expect(utf8_roundtrip[i]).ToBe(utf8_emoji[i]);
                             }
                           });

                        it("should handle IBM-1047 to IBM-037 conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};

                             // Note: string literals on z/OS are typically IBM-1047 by default
                             std::string input_str = "It's a clean machine";

                             // Convert IBM-1047 to IBM-037
                             std::string result = zut_encode(input_str, "IBM-1047", "IBM-037", diag);

                             expect(diag.e_msg_len).ToBe(0);
                             expect(result.length()).ToBe(input_str.length());

                             // Convert back to IBM-1047 to verify round-trip
                             std::string roundtrip = zut_encode(result, "IBM-037", "IBM-1047", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip.length()).ToBe(input_str.length());

                             // Verify original string is preserved
                             expect(roundtrip).ToBe(input_str);
                           });

                        it("should handle CCSID 273 (German EBCDIC) conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input_str = "Muller & Sohne GmbH - Grosse: 15cm"; // Using basic ASCII chars for compatibility

                             // Convert IBM-1047 to CCSID 273 (German EBCDIC)
                             std::string result = zut_encode(input_str, "IBM-1047", "IBM-273", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             // Convert back to verify round-trip
                             std::string roundtrip = zut_encode(result, "IBM-273", "IBM-1047", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip).ToBe(input_str);
                           });

                        it("should handle CCSID 500 (International EBCDIC) conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input_str = "International characters & symbols";

                             // Convert IBM-1047 to CCSID 500
                             std::string result = zut_encode(input_str, "IBM-1047", "IBM-500", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             // Convert back to verify round-trip
                             std::string roundtrip = zut_encode(result, "IBM-500", "IBM-1047", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip).ToBe(input_str);
                           });

                        it("should handle CCSID 285 (UK English EBCDIC) conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input_str = "50.00 - proper behaviour"; // Using basic chars for compatibility

                             std::string result = zut_encode(input_str, "IBM-1047", "IBM-285", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             std::string roundtrip = zut_encode(result, "IBM-285", "IBM-1047", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip).ToBe(input_str);
                           });

                        it("should handle CCSID 297 (French EBCDIC) conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input_str = "Cafe francais - resume naif"; // Using basic chars for compatibility

                             std::string result = zut_encode(input_str, "IBM-1047", "IBM-297", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             std::string roundtrip = zut_encode(result, "IBM-297", "IBM-1047", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip).ToBe(input_str);
                           });

                        it("should handle CCSID 284 (Spanish EBCDIC) conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input_str = "Nino espanol - ano manana"; // Using basic chars for compatibility

                             std::string result = zut_encode(input_str, "IBM-1047", "IBM-284", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             std::string roundtrip = zut_encode(result, "IBM-284", "IBM-1047", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip).ToBe(input_str);
                           });

                        it("should handle CCSID 280 (Italian EBCDIC) conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input_str = "Citta italiana - piu cosi"; // Using basic chars for compatibility

                             std::string result = zut_encode(input_str, "IBM-1047", "IBM-280", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             std::string roundtrip = zut_encode(result, "IBM-280", "IBM-1047", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip).ToBe(input_str);
                           });

                        it("should handle CCSID 1140 (US EBCDIC with Euro) conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input_str = "Price: 125.50 USD 150.00"; // Using basic chars, Euro symbol handling may vary

                             std::string result = zut_encode(input_str, "IBM-037", "IBM-1140", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             std::string roundtrip = zut_encode(result, "IBM-1140", "IBM-037", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip).ToBe(input_str);
                           });

                        it("should handle CCSID 1141 (German EBCDIC with Euro) conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input_str = "Preis: 99,99 - Grosse: Muller"; // Using basic chars for compatibility

                             std::string result = zut_encode(input_str, "IBM-273", "IBM-1141", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             std::string roundtrip = zut_encode(result, "IBM-1141", "IBM-273", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip).ToBe(input_str);
                           });

                        it("should handle CCSID 819 (ISO 8859-1) conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input_str = "ASCII + Latin extras: cafe resume";

                             std::string result = zut_encode(input_str, "IBM-1047", "ISO8859-1", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             std::string roundtrip = zut_encode(result, "ISO8859-1", "IBM-1047", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip).ToBe(input_str);
                           });

                        it("should handle CCSID 1252 (Windows Latin-1) conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input_str = "Windows smart quotes - em-dash"; // Using basic chars for compatibility

                             std::string result = zut_encode(input_str, "IBM-1047", "IBM-1252", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             std::string roundtrip = zut_encode(result, "IBM-1252", "IBM-1047", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip).ToBe(input_str);
                           });

                        it("should handle CCSID 290 (Japanese EBCDIC) conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input_str = "Katakana Test"; // Using basic chars for compatibility

                             std::string result = zut_encode(input_str, "IBM-1047", "IBM-290", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             std::string roundtrip = zut_encode(result, "IBM-290", "IBM-1047", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip).ToBe(input_str);
                           });

                        it("should handle CCSID 935 (Simplified Chinese EBCDIC) conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input_str = "Simplified Chinese Test"; // Using basic chars for compatibility

                             std::string result = zut_encode(input_str, "IBM-1047", "IBM-935", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             std::string roundtrip = zut_encode(result, "IBM-935", "IBM-1047", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip).ToBe(input_str);
                           });

                        it("should handle CCSID 937 (Traditional Chinese EBCDIC) conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input_str = "Traditional Chinese Test"; // Using basic chars for compatibility

                             std::string result = zut_encode(input_str, "IBM-1047", "IBM-937", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             std::string roundtrip = zut_encode(result, "IBM-937", "IBM-1047", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip).ToBe(input_str);
                           });

                        it("should handle CCSID 420 (Arabic EBCDIC) conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input_str = "Arabic Test"; // Using basic chars for compatibility

                             std::string result = zut_encode(input_str, "IBM-1047", "IBM-420", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             std::string roundtrip = zut_encode(result, "IBM-420", "IBM-1047", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip).ToBe(input_str);
                           });

                        it("should handle CCSID 424 (Hebrew EBCDIC) conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input_str = "Hebrew Test"; // Using basic chars for compatibility

                             std::string result = zut_encode(input_str, "IBM-1047", "IBM-424", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             std::string roundtrip = zut_encode(result, "IBM-424", "IBM-1047", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip).ToBe(input_str);
                           });

                        it("should handle CCSID 870 (Latin-2 EBCDIC) conversion",
                           []() -> void
                           {
                             ZDIAG diag = {0};
                             std::string input_str = "Pristup cestina - Laszlo"; // Using basic chars for compatibility

                             std::string result = zut_encode(input_str, "IBM-1047", "IBM-870", diag);
                             expect(diag.e_msg_len).ToBe(0);

                             std::string roundtrip = zut_encode(result, "IBM-870", "IBM-1047", diag);
                             expect(diag.e_msg_len).ToBe(0);
                             expect(roundtrip).ToBe(input_str);
                           });
                      });
           });
}

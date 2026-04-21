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
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>

// Base64 upload/download over stdin/stdout for binary transfer benchmarks.
// Usage: testb64 <upload|download> <filepath> <chunksize>

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int b64_decode_char(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static void upload_b64(const char *filepath, int chunksize)
{
    FILE *out = std::fopen(filepath, "wb");
    if (!out) { std::perror("fopen"); std::exit(1); }

    std::vector<char> inbuf(chunksize * 2);
    std::vector<unsigned char> outbuf(chunksize);

    while (true)
    {
        ssize_t n = read(STDIN_FILENO, inbuf.data(), inbuf.size());
        if (n <= 0)
            break;

        size_t out_idx = 0;
        size_t i = 0;
        while (i < static_cast<size_t>(n))
        {
            // Skip whitespace and padding
            while (i < static_cast<size_t>(n) && (inbuf[i] == '\n' || inbuf[i] == '\r' || inbuf[i] == '='))
                i++;
            if (i >= static_cast<size_t>(n))
                break;

            unsigned int accum = 0;
            int bits = 0;
            while (i < static_cast<size_t>(n) && bits < 24)
            {
                if (inbuf[i] == '=') { i++; break; }
                int val = b64_decode_char(inbuf[i]);
                if (val < 0) { i++; continue; }
                accum = (accum << 6) | val;
                bits += 6;
                i++;
            }

            while (bits >= 8)
            {
                bits -= 8;
                outbuf[out_idx++] = (accum >> bits) & 0xFF;
            }
        }

        if (out_idx > 0)
            std::fwrite(outbuf.data(), 1, out_idx, out);
    }

    std::fclose(out);
}

static void download_b64(const char *filepath, int chunksize)
{
    FILE *in = std::fopen(filepath, "rb");
    if (!in) { std::perror("fopen"); std::exit(1); }

    std::vector<unsigned char> buf(chunksize);

    while (true)
    {
        size_t n = std::fread(buf.data(), 1, buf.size(), in);
        if (n == 0)
            break;

        // Encode to base64 and write to stdout
        size_t i = 0;
        while (i < n)
        {
            unsigned int b0 = buf[i++];
            unsigned int b1 = (i < n) ? buf[i++] : 0;
            unsigned int b2 = (i < n) ? buf[i++] : 0;

            size_t remaining = n - (i - 1);
            char enc[4];
            enc[0] = b64_table[(b0 >> 2) & 0x3F];
            enc[1] = b64_table[((b0 & 0x03) << 4) | ((b1 >> 4) & 0x0F)];
            enc[2] = (remaining >= 2) ? b64_table[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=';
            enc[3] = (remaining >= 3) ? b64_table[b2 & 0x3F] : '=';
            write(STDOUT_FILENO, enc, 4);
        }
    }

    std::fclose(in);
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        std::fprintf(stderr, "Usage: %s <upload|download> <filepath> <chunksize>\n", argv[0]);
        return 1;
    }

    const char *command = argv[1];
    const char *filepath = argv[2];
    int chunksize = std::atoi(argv[3]);

    if (std::strcmp(command, "upload") == 0)
        upload_b64(filepath, chunksize);
    else if (std::strcmp(command, "download") == 0)
        download_b64(filepath, chunksize);
    else
        std::fprintf(stderr, "Unknown command: %s\n", command);

    return 0;
}

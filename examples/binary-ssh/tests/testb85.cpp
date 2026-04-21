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
#include <cstdint>
#include <vector>
#include <unistd.h>

// Ascii85 upload/download over stdin/stdout for binary transfer benchmarks.
// Usage: testb85 <upload|download> <filepath> <chunksize>

static void encode_block(const unsigned char *src, size_t len, char *out, size_t &out_len)
{
    uint32_t val = 0;
    for (size_t i = 0; i < 4; i++)
        val = (val << 8) | ((i < len) ? src[i] : 0);

    char enc[5];
    for (int i = 4; i >= 0; i--)
    {
        enc[i] = static_cast<char>((val % 85) + 33);
        val /= 85;
    }

    size_t count = len + 1;
    std::memcpy(out + out_len, enc, count);
    out_len += count;
}

static void upload_b85(const char *filepath, int chunksize)
{
    FILE *out = std::fopen(filepath, "wb");
    if (!out) { std::perror("fopen"); std::exit(1); }

    std::vector<char> inbuf(chunksize * 2);
    std::vector<unsigned char> decode_buf(chunksize);

    while (true)
    {
        ssize_t n = read(STDIN_FILENO, inbuf.data(), inbuf.size());
        if (n <= 0)
            break;

        size_t out_idx = 0;
        size_t i = 0;
        while (i < static_cast<size_t>(n))
        {
            uint32_t val = 0;
            int count = 0;
            while (i < static_cast<size_t>(n) && count < 5)
            {
                char c = inbuf[i];
                if (c >= '!' && c <= 'u')
                {
                    val = val * 85 + (c - 33);
                    count++;
                }
                i++;
            }

            if (count == 0)
                continue;

            // Pad missing chars with 'u' (84)
            for (int j = count; j < 5; j++)
                val = val * 85 + 84;

            int nbytes = count - 1;
            for (int j = 3; j >= 4 - nbytes; j--)
                decode_buf[out_idx++] = (val >> (j * 8)) & 0xFF;
        }

        if (out_idx > 0)
            std::fwrite(decode_buf.data(), 1, out_idx, out);
    }

    std::fclose(out);
}

static void download_b85(const char *filepath, int chunksize)
{
    FILE *in = std::fopen(filepath, "rb");
    if (!in) { std::perror("fopen"); std::exit(1); }

    std::vector<unsigned char> buf(chunksize);
    std::vector<char> enc_buf(chunksize * 5 / 4 + 16);

    while (true)
    {
        size_t n = std::fread(buf.data(), 1, buf.size(), in);
        if (n == 0)
            break;

        size_t enc_len = 0;
        size_t i = 0;
        while (i < n)
        {
            size_t block_len = (n - i >= 4) ? 4 : (n - i);
            encode_block(buf.data() + i, block_len, enc_buf.data(), enc_len);
            i += block_len;
        }

        if (enc_len > 0)
            write(STDOUT_FILENO, enc_buf.data(), enc_len);
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
        upload_b85(filepath, chunksize);
    else if (std::strcmp(command, "download") == 0)
        download_b85(filepath, chunksize);
    else
        std::fprintf(stderr, "Unknown command: %s\n", command);

    return 0;
}

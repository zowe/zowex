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
#include <vector>
#include <unistd.h>
#include <termios.h>

// Raw binary upload/download over stdin/stdout for binary transfer benchmarks.
// Usage: testraw <upload|download> <filepath> <chunksize> [totalsize]

struct RawMode
{
    int fd;
    struct termios orig;
    bool active = false;

    bool enable(int file_fd)
    {
        fd = file_fd;
        struct termios raw;
        if (tcgetattr(fd, &orig) < 0)
            return false;
        raw = orig;
        raw.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
        raw.c_oflag &= ~OPOST;
        raw.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
        raw.c_cflag &= ~(CSIZE | PARENB);
        raw.c_cflag |= CS8;
        if (tcsetattr(fd, TCSANOW, &raw) < 0)
            return false;
        active = true;
        return true;
    }

    ~RawMode()
    {
        if (active)
            tcsetattr(fd, TCSANOW, &orig);
    }
};

static void upload_raw(const char *filepath, int chunksize, int totalsize)
{
    FILE *out = std::fopen(filepath, "wb");
    if (!out) { std::perror("fopen"); std::exit(1); }

    std::vector<char> buf(chunksize);
    int received = 0;

    while (received < totalsize)
    {
        ssize_t n = read(STDIN_FILENO, buf.data(), buf.size());
        if (n <= 0)
            break;
        std::fwrite(buf.data(), 1, n, out);
        received += n;
    }

    std::fclose(out);
}

static void download_raw(const char *filepath, int chunksize)
{
    FILE *in = std::fopen(filepath, "rb");
    if (!in) { std::perror("fopen"); std::exit(1); }

    std::vector<char> buf(chunksize);

    while (true)
    {
        size_t n = std::fread(buf.data(), 1, buf.size(), in);
        if (n == 0)
            break;
        write(STDOUT_FILENO, buf.data(), n);
    }

    std::fclose(in);
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        std::fprintf(stderr, "Usage: %s <upload|download> <filepath> <chunksize> [totalsize]\n", argv[0]);
        return 1;
    }

    RawMode stdin_raw, stdout_raw;
    if (!stdin_raw.enable(STDIN_FILENO) || !stdout_raw.enable(STDOUT_FILENO))
    {
        std::fprintf(stderr, "Error setting raw mode\n");
        return 1;
    }

    const char *command = argv[1];
    const char *filepath = argv[2];
    int chunksize = std::atoi(argv[3]);

    if (std::strcmp(command, "upload") == 0)
    {
        if (argc < 5)
        {
            std::fprintf(stderr, "Upload requires totalsize argument\n");
            return 1;
        }
        int totalsize = std::atoi(argv[4]);
        upload_raw(filepath, chunksize, totalsize);
    }
    else if (std::strcmp(command, "download") == 0)
    {
        download_raw(filepath, chunksize);
    }
    else
    {
        std::fprintf(stderr, "Unknown command: %s\n", command);
    }

    return 0;
}

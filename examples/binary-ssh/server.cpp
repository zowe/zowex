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
#include <unistd.h>
#include <termios.h>

// Reads 256 raw bytes from stdin, writes them back to stdout, and saves to a file.
// Demonstrates raw binary I/O over an SSH connection.

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

int main()
{
    RawMode stdin_raw, stdout_raw;
    if (!stdin_raw.enable(STDIN_FILENO))
    {
        std::fprintf(stderr, "Error setting stdin to raw mode\n");
        return 1;
    }
    if (!stdout_raw.enable(STDOUT_FILENO))
    {
        std::fprintf(stderr, "Error setting stdout to raw mode\n");
        return 1;
    }

    unsigned char buffer[256];
    size_t total = 0;
    while (total < sizeof(buffer))
    {
        ssize_t n = read(STDIN_FILENO, buffer + total, sizeof(buffer) - total);
        if (n <= 0)
        {
            std::fprintf(stderr, "Error reading from stdin\n");
            return 1;
        }
        total += n;
    }

    write(STDOUT_FILENO, buffer, sizeof(buffer));
    fsync(STDOUT_FILENO);

    FILE *f = std::fopen("output.txt", "w");
    if (f)
    {
        for (size_t i = 0; i < sizeof(buffer); i++)
            std::fprintf(f, "%d ", buffer[i]);
        std::fprintf(f, "\n");
        std::fclose(f);
    }

    return 0;
}

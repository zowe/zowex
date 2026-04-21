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
#include <cstring>
#include <unistd.h>

int main()
{
    char buf[1024];
    while (true)
    {
        ssize_t n = fread(buf, 1, sizeof(buf), stdin);
        if (n <= 0)
            break;

        __e2a_l(buf, n);
        if (n >= 4 && strncmp(buf, "ping", 4) == 0)
        {
            char pong[] = "pong\n";
            __a2e_s(pong);
            fwrite(pong, 1, sizeof(pong) - 1, stdout);
            fflush(stdout);
        }
    }

    return 0;
}

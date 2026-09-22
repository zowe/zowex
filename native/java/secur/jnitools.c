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

#define _OPEN_SYS 1
#include <stdlib.h>
#include <jni.h>
#include "jnitools.h"

char *jstring_to_ebcdic(JNIEnv *env, jstring jstr)
{
    if (jstr == NULL)
    {
        return NULL;
    }

    int len;
    jint rc = GetStringPlatformLength(env, jstr, &len, "IBM-1047");
    if (rc != 0)
    {
        return NULL;
    }

    char *str = __malloc31(len);
    rc = GetStringPlatform(env, jstr, str, len, "IBM-1047");
    if (rc != 0)
    {
        free(str);
        return NULL;
    }

    return str;
}

void free_if_not_null(void *ptr)
{
    free(ptr);
}

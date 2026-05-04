
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

#ifndef ZDBG_H
#define ZDBG_H

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include "zutm.h"
#include "zwto.h"

typedef void (*zut_print_func)(const char *fmt);

/**
 * @brief Dump a memory region to output for debugging
 * @param label Label for the dump
 * @param addr Pointer to the memory region
 * @param size Size of the memory region in bytes
 * @param bytes_per_line Number of bytes per line
 * @param new_line Whether to print a new line
 * @param cb_print Call back function to print the output
 */
static void zut_dump_storage_common(const char *title, const void *data, int size, int bytes_per_line, int new_line, zut_print_func cb_print)
{
  int len = 0;
  char buf[1024] = {0};
  /* Safe title length limit (100) to avoid buf overflow */
  len += sprintf(buf + len, "--- Dumping storage for '%.100s' at x'%016llx' ---", title, (unsigned long long)data);
  if (new_line)
  {
    len += sprintf(buf + len, "\n");
  }
  cb_print(buf);
  memset(buf, 0, sizeof(buf));
  len = 0;

  unsigned char *ptr = (unsigned char *)data;

#define MAX_BYTES_PER_LINE 32

  if (bytes_per_line > MAX_BYTES_PER_LINE)
  {
    bytes_per_line = MAX_BYTES_PER_LINE;
  }

  int index = 0;
  char spaces[] = "                                ";
  char fmt_buf[MAX_BYTES_PER_LINE + 1] = {0};

  int lines = size / bytes_per_line;
  int remainder = size % bytes_per_line;
  char unknown = '.';

  for (int x = 0; x < lines; x++)
  {
    len += sprintf(buf + len, "%016llx", (unsigned long long)ptr);
    len += sprintf(buf + len, " | ");
    for (int y = 0; y < bytes_per_line; y++)
    {
      unsigned char p = isprint(ptr[y]) ? ptr[y] : unknown;
      len += sprintf(buf + len, "%c", p);
    }
    len += sprintf(buf + len, " | ");

    for (int y = 0; y < bytes_per_line; y++)
    {
      len += sprintf(buf + len, "%02x", (unsigned char)ptr[y]);

      if ((y + 1) % 4 == 0)
      {
        len += sprintf(buf + len, " ");
      }
      if ((y + 1) % 16 == 0)
      {
        len += sprintf(buf + len, "    ");
      }
    }
    if (new_line)
    {
      len += sprintf(buf + len, "\n");
    }
    cb_print(buf);
    memset(buf, 0, sizeof(buf));
    len = 0;
    ptr = ptr + bytes_per_line;
  }

  len += sprintf(buf + len, "%016llx", (unsigned long long)ptr);
  len += sprintf(buf + len, " | ");
  for (int y = 0; y < remainder; y++)
  {
    unsigned char p = isprint(ptr[y]) ? ptr[y] : unknown;
    len += sprintf(buf + len, "%c", p);
  }
  memset(fmt_buf, 0x00, bytes_per_line);
  sprintf(fmt_buf, "%.*s | ", bytes_per_line - remainder, spaces);
  len += sprintf(buf + len, "%s", fmt_buf);
  for (int y = 0; y < remainder; y++)
  {
    len += sprintf(buf + len, "%02x", (unsigned char)ptr[y]);

    if ((y + 1) % 4 == 0)
    {
      len += sprintf(buf + len, " ");
    }
    if ((y + 1) % 16 == 0)
    {
      len += sprintf(buf + len, "    ");
    }
  }
  if (new_line)
  {
    len += sprintf(buf + len, "\n");
  }
  cb_print(buf);
  memset(buf, 0, sizeof(buf));
  len = 0;

  len += sprintf(buf + len, "--- END ---");
  if (new_line)
  {
    len += sprintf(buf + len, "\n");
  }
  cb_print(buf);
  memset(buf, 0, sizeof(buf));
  len = 0;
}

// example usage: zut_dump_storage_common("jprhdr", jprhdr, sizeof(JPRHDR), 16, 0, zut_print_debug);
static void zut_print_debug(const char *fmt)
{
#if defined(__IBM_METAL__)
  zwto_debug(fmt);
#else
  fprintf(stderr, "%s", fmt);
#endif
}

static void zut_dump_storage(const char *title, const void *data, int size)
{
#if defined(__IBM_METAL__)
  zut_dump_storage_common(title, data, size, 16, 0, zut_print_debug);
#else
  zut_dump_storage_common(title, data, size, 32, 1, zut_print_debug);
#endif
}

#endif

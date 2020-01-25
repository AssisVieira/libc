/*******************************************************************************
 *   Copyright 2019 Assis Vieira
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 ******************************************************************************/

#include "str.h"
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////

typedef struct str_t {
  size_t size;
  size_t length;
  char buff[];
} str_t;

////////////////////////////////////////////////////////////////////////////////

static const str_t STR_NULL = {0, 0};

////////////////////////////////////////////////////////////////////////////////

static const unsigned char ASCII_LOWER_CASE[] = {
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
    15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
    30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
    45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
    60,  61,  62,  63,  64,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106,
    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
    122, 91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
    135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
    165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
    255};

////////////////////////////////////////////////////////////////////////////////

str_t *str_new(size_t size) {
  if (size <= 0) {
    return NULL;
  }

  str_t *str = malloc(sizeof(str_t) + size);

  if (str == NULL) {
    return NULL;
  }

  str->size = size;
  str->length = 0;
  str->buff[0] = '\0';

  return str;
}

////////////////////////////////////////////////////////////////////////////////

str_t *str_clonecstr(const char *cstr) {
  return str_clonecstrlen(cstr, strlen(cstr));
}

////////////////////////////////////////////////////////////////////////////////

str_t *str_clonecstrlen(const char *cstr, size_t len) {
  str_t *str = str_new(len + 1);
  if (str == NULL) {
    return NULL;
  }
  if (str_addcstrlen(&str, cstr, len) != 0) {
    return NULL;
  }
  return str;
}

////////////////////////////////////////////////////////////////////////////////

const char *str_cstr(const str_t *str) { return str->buff; }

////////////////////////////////////////////////////////////////////////////////

void str_free(str_t **str) {
  free(*str);
  *str = NULL;
}

////////////////////////////////////////////////////////////////////////////////

size_t str_utf8len(const str_t *str) {
  size_t len = 0;
  const char *p = str->buff;
  while (*p) {
    len += (*(p++) & 0xC0) != 0x80;
  }
  return len;
}

////////////////////////////////////////////////////////////////////////////////

size_t str_size(const str_t *str) { return str->size; }

////////////////////////////////////////////////////////////////////////////////

size_t str_len(const str_t *str) { return str->length; }

////////////////////////////////////////////////////////////////////////////////

str_t *str_expand(str_t *str, size_t need) {
  if (str->length + need < str->size) {
    return str;
  }

  size_t newSize = str->length + need + 1;

  str_t *newStr = realloc(str, sizeof(str_t) + newSize);

  if (newStr == NULL) {
    return NULL;
  }

  newStr->size = newSize;

  return newStr;
}

////////////////////////////////////////////////////////////////////////////////

int str_addc(str_t **dest, char c) {
  assert(dest != NULL);
  assert(*dest != NULL);

  str_t *newStr = str_expand(*dest, 1);

  if (newStr == NULL) {
    return -1;
  }

  newStr->buff[newStr->length++] = c;

  newStr->buff[newStr->length] = '\0';

  *dest = newStr;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int str_addcstr(str_t **dest, const char *orig) {
  assert(dest != NULL);
  assert(*dest != NULL);
  assert(orig != NULL);
  return str_addcstrlen(dest, orig, strlen(orig));
}

////////////////////////////////////////////////////////////////////////////////

int str_addcstrlen(str_t **dest, const char *orig, size_t len) {
  assert(dest != NULL);
  assert(*dest != NULL);
  assert(orig != NULL);

  str_t *newStr = str_expand(*dest, len);

  if (newStr == NULL) {
    return -1;
  }

  for (int i = 0; i < len; i++) {
    newStr->buff[newStr->length++] = orig[i];
  }

  newStr->buff[newStr->length] = '\0';

  *dest = newStr;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int str_add(str_t **dest, const str_t *orig) {
  assert(dest != NULL);
  assert(*dest != NULL);
  assert(orig != NULL);
  return str_addcstrlen(dest, orig->buff, orig->length);
}

////////////////////////////////////////////////////////////////////////////////

str_t *str_clear(str_t *str) {
  assert(str != NULL);
  str->length = 0;
  str->buff[0] = '\0';
  return str;
}

////////////////////////////////////////////////////////////////////////////////
// TODO: verificar a ordem de caçapa e cacapa, pois cacapa deve estar na frente
// de caçapa.

int str_cmp(const str_t *s1, const str_t *s2) {
  assert(s1 != NULL);
  assert(s2 != NULL);

  const char *p1 = s1->buff;
  const char *p2 = s2->buff;

  if (s1 == s2) {
    return 0;
  }

  for (; *p1 == *p2; p1++, p2++) {
    if (*p1 == '\0') {
      return 0;
    }
  }

  return (*(unsigned char *)p1 < *(unsigned char *)p2) ? -1 : +1;
}

////////////////////////////////////////////////////////////////////////////////

int str_cmpcstr(const str_t *s1, const char *s2) {
  assert(s1 != NULL);
  assert(s2 != NULL);

  const char *p1 = s1->buff;
  const char *p2 = s2;

  for (; *p1 == *p2; p1++, p2++) {
    if (*p1 == '\0') {
      return 0;
    }
  }

  return (*(unsigned char *)p1 < *(unsigned char *)p2) ? -1 : +1;
}

////////////////////////////////////////////////////////////////////////////////

int str_casecmp(const str_t *s1, const str_t *s2) {
  assert(s1 != NULL);
  assert(s2 != NULL);

  if (s1 == s2) {
    return 0;
  }

  return str_casecmpcstrlen(s1, s2->buff, s2->length);
}

////////////////////////////////////////////////////////////////////////////////

int str_casecmpcstr(const str_t *s1, const char *s2) {
  assert(s1 != NULL);
  assert(s2 != NULL);
  return str_casecmpcstrlen(s1, s2, strlen(s2));
}

////////////////////////////////////////////////////////////////////////////////

int str_casecmpcstrlen(const str_t *s1, const char *s2, size_t len) {
  assert(s1 != NULL);
  assert(s2 != NULL);

  const unsigned char *u1 = (const unsigned char *)s1->buff;
  const unsigned char *u2 = (const unsigned char *)s2;

  for (;;) {
    if (ASCII_LOWER_CASE[*u1] != ASCII_LOWER_CASE[*u2]) {
      return (u1 < u2) ? -1 : +1;
    }
    if (*u1 == '\0') {
      return 0;
    }
    u1++;
    u2++;
  }
}

////////////////////////////////////////////////////////////////////////////////

str_t *str_clone(const str_t *str) {
  assert(str != NULL);
  str_t *newStr = str_new(str->length);
  if (newStr == NULL) {
    return NULL;
  }
  if (str_add(&newStr, str) != 0) {
    str_free(&newStr);
    return NULL;
  }
  return newStr;
}

////////////////////////////////////////////////////////////////////////////////

static size_t itoa(long long value, int radix, bool uppercase, bool unsig,
                   char *buff, size_t zero_pad) {
  assert(buff != NULL);
  const char digit1[] = "0123456789ABCDEF";
  const char digit2[] = "0123456789abcdf";
  const char *digit = (uppercase) ? digit1 : digit2;
  char *buffp = buff;
  int negative = 0;
  size_t i;
  size_t len;

  if (radix > 16)
    return 0;

  if (value < 0 && !unsig) {
    negative = 1;
    value = -value;
  }

  while (value > 0) {
    *buffp++ = digit[value % radix];
    value /= radix;
  };

  for (i = 0; i < zero_pad; i++)
    *buffp++ = '0';

  if (negative)
    *buffp++ = '-';

  len = (size_t)(buffp - buff);
  for (i = 0; i < len / 2; i++) {
    char j = buff[i];
    buff[i] = buff[len - i - 1];
    buff[len - i - 1] = j;
  }

  return len;
}

////////////////////////////////////////////////////////////////////////////////

size_t str_fmtv(str_t *str, const char *fmt, va_list va) {
  const size_t initLen = str->length;
  const size_t maxLen = str->size - 1;

  for (; *fmt; fmt++) {
    if (*fmt != '%') {
      if (str->length == maxLen) {
        break;
      }
      str->buff[str->length++] = *fmt;
    } else if (*fmt == '%') {
      fmt++;
      if (*fmt == 's') {
        const char *buff = va_arg(va, const char *);
        while (*buff && str->length < maxLen) {
          str->buff[str->length++] = *buff++;
        }
      } else if (*fmt == 'd' || *fmt == 'l') {
        long long num = (*fmt == 'd') ? va_arg(va, int) : va_arg(va, long long);
        char buff[32];
        size_t buffLen = itoa(num, 10, true, false, buff, 0);
        if (str->length + buffLen - 1 == maxLen) {
          break;
        }
        memcpy(str->buff + str->length, buff, buffLen);
        str->length += buffLen;
      } else {
        if (str->length == maxLen) {
          break;
        }
        str->buff[str->length++] = *fmt;
      }
    }
  }

  str->buff[str->length] = '\0';

  if (*fmt != '\0') {
    return -1;
  }

  return str->length - initLen;
}

////////////////////////////////////////////////////////////////////////////////

size_t str_fmt(str_t *str, const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  size_t r = str_fmtv(str, fmt, va);
  va_end(va);
  return r;
}

////////////////////////////////////////////////////////////////////////////////

const str_t *str_null() { return &STR_NULL; }

////////////////////////////////////////////////////////////////////////////////

str_t *str_move(str_t **str) {
  str_t *tmp = *str;
  *str = str_new(tmp->size);
  return tmp;
}

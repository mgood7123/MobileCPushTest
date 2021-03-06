﻿// source : https://gist.github.com/ingramj/1105106


/* Implementations of the getdelim() and getline() functions from POSIX 2008,
   just in case your libc doesn't have them.

   getdelim() reads from a stream until a specified delimiter is encountered.
   getline() reads from a stream until a newline is encountered.

   See: http://pubs.opengroup.org/onlinepubs/9699919799/functions/getdelim.html

   NOTE: It is always the caller's responsibility to free the line buffer, even
   when an error occurs.

   Copyright (c) 2011 James E. Ingram

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/


#include <limits.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#if __STDC_VERSION__ >= 199901L
/* restrict is a keyword */
#else
# define restrict
#endif


#ifndef _POSIX_SOURCE
typedef long ssize_t;
#define SSIZE_MAX LONG_MAX
#endif

bool getdelim_include_delimiter = false;
bool getline_include_delimiter = false;

size_t __getdelim_default_grow_by_allocation = 1;
size_t __getline_default_grow_by_allocation = 1;
size_t __getdelim_default_minimum_allocation = 1;
size_t __getline_default_minimum_allocation = 1;

size_t getdelim_grow_by_allocation = __getdelim_default_grow_by_allocation;
size_t getline_grow_by_allocation = __getline_default_grow_by_allocation;
size_t getdelim_minimum_allocation = __getdelim_default_minimum_allocation;
size_t getline_minimum_allocation = __getline_default_minimum_allocation;

ssize_t getdelim(char **restrict lineptr, size_t *restrict n, int delimiter,
                 FILE *restrict stream);
ssize_t getline(char **restrict lineptr, size_t *restrict n,
                FILE *restrict stream);



#define _GETDELIM_GROWBY getdelim_grow_by_allocation    /* amount to grow line buffer by */
#define _GETDELIM_MINLEN getdelim_minimum_allocation      /* minimum line buffer size */


ssize_t getdelim(char **restrict lineptr, size_t *restrict n, int delimiter,
                 FILE *restrict stream)
{
	char *buf, *pos;
	int c;
	ssize_t bytes;

	if (lineptr == NULL || n == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (stream == NULL) {
		errno = EBADF;
		return -1;
	}

	/* resize (or allocate) the line buffer if necessary */
	buf = *lineptr;
	if (buf == NULL || *n < _GETDELIM_MINLEN) {
		buf = realloc(*lineptr, _GETDELIM_GROWBY);
		if (buf == NULL) {
			errno = ENOMEM;
			return -1;
		}
		*n = _GETDELIM_GROWBY;
		*lineptr = buf;
	}

	/* read characters until delimiter is found, end of file is reached, or an
	   error occurs. */
	bytes = 0;
	pos = buf;
	while ((c = getc(stream)) != EOF) {
		if (bytes + 1 >= SSIZE_MAX) {
			errno = EOVERFLOW;
			return -1;
		}
		bytes++;
		if (bytes >= *n - 1) {
			buf = realloc(*lineptr, *n + _GETDELIM_GROWBY);
			if (buf == NULL) {
				errno = ENOMEM;
				return -1;
			}
			*n += _GETDELIM_GROWBY;
			pos = buf + bytes - 1;
			*lineptr = buf;
		}

		if (c == delimiter) {
			if (getdelim_include_delimiter == true) *pos++ = (char) c;
			break;
		}
		else *pos++ = (char) c;
	}
	if (ferror(stream) || (feof(stream) && (bytes == 0))) {
		/* EOF, or an error from getc(). */
		return -1;
	}
	*pos = '\0';
	return bytes;
}


ssize_t getline(char **restrict lineptr, size_t *restrict n,
                FILE *restrict stream)
{
	if (getline_include_delimiter == true) getdelim_include_delimiter = true;
	// prioritize getdelim, use getline as a fallback
	if (getline_grow_by_allocation != 1 && getdelim_grow_by_allocation == 1) getdelim_grow_by_allocation = getline_grow_by_allocation;
	if (getline_minimum_allocation != 1 && getdelim_minimum_allocation == 1) getdelim_minimum_allocation = getline_minimum_allocation;
		
	return getdelim(lineptr, n, '\n', stream);
}

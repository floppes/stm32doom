/*
 * syscalls.c
 *
 *  Created on: 16.05.2014
 *      Author: Florian
 */


/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "debug.h"

/*---------------------------------------------------------------------*
 *  local definitions                                                  *
 *---------------------------------------------------------------------*/

#define HEAP_SIZE		0x00700000 // 7 MB

/*---------------------------------------------------------------------*
 *  external declarations                                              *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public data                                                        *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  private data                                                       *
 *---------------------------------------------------------------------*/

/* heap */
__attribute__ ((section(".sdram")))
static char heap[HEAP_SIZE];

/* pointer to current position in heap */
static char* _cur_brk = heap;

/*---------------------------------------------------------------------*
 *  private functions                                                  *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

int _read_r (struct _reent* r, int file, char* ptr, int len)
{
	errno = EINVAL;
	return -1;
}

int _lseek_r (struct _reent* r, int file, int ptr, int dir)
{
	return 0;
}

int _write_r (struct _reent* r, int file, char* ptr, int len)
{
	int i;

	/* output string via UART */
	for (i = 0; i < len; i++)
	{
		if (ptr[i] == '\n')
		{
			debug_chr ('\r');
		}

		debug_chr (ptr[i]);
	}

	return len;
}

int _close_r (struct _reent* r, int file)
{
	return 0;
}

caddr_t _sbrk_r (struct _reent* r, int incr)
{
	char* _old_brk = _cur_brk;

	if ((_cur_brk + incr) > (heap + HEAP_SIZE))
	{
		uint8_t i;

		char* msg = "HEAP FULL!\r\n";

		for (i = 0; i < strlen(msg); i++)
		{
			debug_chr (msg[i]);
		}

		errno = ENOMEM;
		return (void *)-1;
	}

	_cur_brk += incr;

	return (caddr_t)_old_brk;
}

int _fstat_r (struct _reent* r, int file, struct stat* st)
{
	st->st_mode = S_IFCHR;

	return 0;
}

int _isatty_r (struct _reent* r, int fd)
{
	return 1;
}

void _exit (int rc)
{
	while (1)
	{
	}
}

int _kill (int pid, int sig)
{
	errno = EINVAL;

	return -1;
}

int _getpid ()
{
	return 1;
}

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/

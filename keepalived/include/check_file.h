/*
 * Soft:        Keepalived is a failover program for the LVS project
 *              <www.linuxvirtualserver.org>. It monitor & manipulate
 *              a loadbalanced server pool using multi-layer checks.
 *
 * Part:        check_file.c include file.
 *
 * Author:      Udi Trugman, <udi@trugman.com>
 *
 *              This program is distributed in the hope that it will be useful,
 *              but WITHOUT ANY WARRANTY; without even the implied warranty of
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *              See the GNU General Public License for more details.
 *
 *              This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU General Public License
 *              as published by the Free Software Foundation; either version
 *              2 of the License, or (at your option) any later version.
 *
 * Copyright (C) 2001-2011 Alexandre Cassen, <acassen@linux-vs.org>
 */

#ifndef _CHECK_FILE_H
#define _CHECK_FILE_H

/* system includes */
#include <stdlib.h>

/* local includes */
#include "scheduler.h"

/* Checker argument structure  */
typedef struct _file_checker {
	char *path;
	int dynamic;  /* 0: old-style, 1: contents of the file affects weight */
} file_checker_t;

/* Prototypes defs */
extern void install_file_check_keyword(void);

#endif

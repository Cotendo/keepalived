/*
 * Soft:        Keepalived is a failover program for the LVS project
 *              <www.linuxvirtualserver.org>. It monitor & manipulate
 *              a loadbalanced server pool using multi-layer checks.
 *
 * Part:        FILE CHECK. Reads the contents of a file and treats it as result and/or weight
 *
 * Authors:     Udi Trugman, <udi@cotendo.com>
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

#include "check_file.h"
#include "check_api.h"
#include "memory.h"
#include "ipwrapper.h"
#include "logger.h"
#include "smtp.h"
#include "utils.h"
#include "parser.h"
#include "notify.h"
#include "daemon.h"
#include "signals.h"

int file_check_thread(thread_t *);

/* Configuration stream handling */
void
free_file_check(void *data)
{
	file_checker_t *file_checker = CHECKER_DATA(data);

	FREE(file_checker->path);
	FREE(file_checker);
	FREE(data);
}

void
dump_file_check(void *data)
{
	file_checker_t *file_checker = CHECKER_DATA(data);
	log_message(LOG_INFO, "   Keepalive method = FILE_CHECK");
	log_message(LOG_INFO, "   file = %s", file_checker->path);
	log_message(LOG_INFO, "   dynamic = %s", file_checker->dynamic ? "YES" : "NO");
}

void
file_check_handler(vector strvec)
{
	file_checker_t *file_checker = (file_checker_t *) MALLOC(sizeof (file_checker_t));

	/* queue new checker */
	queue_checker(free_file_check, dump_file_check, file_check_thread,
	              file_checker);
}

void
file_path_handler(vector strvec)
{
	file_checker_t *file_checker = CHECKER_GET();
	file_checker->path = CHECKER_VALUE_STRING(strvec);
}

void
file_dynamic_handler(vector strvec)
{
	file_checker_t *file_checker = CHECKER_GET();
	file_checker->dynamic = 1;
}

void
install_file_check_keyword(void)
{
	install_keyword("FILE_CHECK", &file_check_handler);
	install_sublevel();
	install_keyword("file_path", &file_path_handler);
	install_keyword("file_dynamic", &file_dynamic_handler);
	install_sublevel_end();
}

int
file_check_thread(thread_t * thread)
{
	checker_t *checker;
	file_checker_t *file_checker;

	checker = THREAD_ARG(thread);
	file_checker = CHECKER_ARG(checker);

	if (CHECKER_ENABLED(checker))
	{
		FILE* f = fopen (file_checker->path, "r");
		if (f == NULL)
		{
			if (svr_checker_up(checker->id, checker->rs))
			{
				// failed to open
				log_message(LOG_INFO, "file check \"%s\" open failed. error:%d-%s!!!"
							, file_checker->path
							, errno, strerror (errno));
				smtp_alert(checker->rs, NULL, NULL,
					   "DOWN",
					   "=> FILE CHECK failed on service <=");
				update_svr_checker_state(DOWN, checker->id
								 , checker->vs
								 , checker->rs);
			}
		}
		else
		{
			char buf [100];
			if (fgets (buf, sizeof (buf)-1, f) == NULL)
			{
				if (svr_checker_up(checker->id, checker->rs))
				{
					// failed to open
					log_message(LOG_INFO, "file check \"%s\" read line failed.!!!"
								, file_checker->path);
					smtp_alert(checker->rs, NULL, NULL,
						   "DOWN",
						   "=> FILE CHECK failed on service <=");
					update_svr_checker_state(DOWN, checker->id
									 , checker->vs
									 , checker->rs);
				}
			}
			else
			{
				// first let's ensure that strlen will never fail
				buf [sizeof(buf)-1] = '\0';

				// now, let's trip the buffer from spaces and new lines
				int len = strlen (buf);
				while (len > 0 && isspace(buf[len-1]))
					--len;

				// ensure an end of string again.
				buf [len] = '\0';

				const char* end;
				long int value = strtol (buf, &end, 10);

				// if we found any non digit
				if (*end != '\0')
				{
					if (svr_checker_up(checker->id, checker->rs))
					{
						// failed to open
						log_message(LOG_INFO, "file check \"%s\" bad content. expecting a number.!!!"
									, file_checker->path);
						smtp_alert(checker->rs, NULL, NULL,
							   "DOWN",
							   "=> FILE CHECK failed on service <=");
						update_svr_checker_state(DOWN, checker->id
										 , checker->vs
										 , checker->rs);
					}
				}
				else
				{

					if (value == 0 || (file_checker->dynamic == 1 && value >= 2 && value <= 255))
					{
						/*
						 * The actual weight set when using file_dynamic is two less than
						 * the value.  Effective range is 0..253.
						 * Catch legacy case of value being 0 but file_dynamic being set.
						 */
						if (file_checker->dynamic == 1 && value != 0)
							update_svr_wgt(value - 2, checker->vs, checker->rs);

						/* everything is good */
						if (!svr_checker_up(checker->id, checker->rs)) {
							log_message(LOG_INFO, "File check of [%s] success."
									    , file_checker->path);
							smtp_alert(checker->rs, NULL, NULL,
								   "UP",
								   "=> FILE CHECK succeed on service <=");
							update_svr_checker_state(UP, checker->id
										   , checker->vs
										   , checker->rs);
						}
					}
					else
					{
						if (svr_checker_up(checker->id, checker->rs)) {
							log_message(LOG_INFO, "File check of [%s] failed."
									    , file_checker->path);
							smtp_alert(checker->rs, NULL, NULL,
								   "DOWN",
								   "=> FILE CHECK failed on service <=");
							update_svr_checker_state(DOWN, checker->id
										     , checker->vs
										     , checker->rs);
						}
					}
				}
			}
		}

		if (f)
			fclose (f);
	}

	/* Register next timer checker */
	thread_add_timer(thread->master, file_check_thread, checker,
			 checker->vs->delay_loop);
	return 0;

}

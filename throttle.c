/*
 * throttle: bandwidth limiting pipe
 * Copyright (C) 2003, 2004  James Klicman <james at klicman dot org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif


#include <stdio.h>
#include <errno.h>
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif


#define THROTTLE PACKAGE

#define TSECS(tvp) (((double)(tvp)->tv_sec)+(((double)(tvp)->tv_usec)*0.000001))
#define BSECS(bytes,Bps) (((double)(bytes))/((double)(Bps)))

typedef enum { false = 0, true = 1 } boolean;

static int throttle(double Bps, time_t window, void *block, size_t blocksize)
{
    struct timeval starttime, currenttime, elapsedtime;
    double sync;
    int32_t bytesread;
    size_t nleft;
    ssize_t nread, nwritten;
    char *ptr;
    boolean done;
#if HAVE_NANOSLEEP
    struct timespec ts_sync;
#endif
#if HAVE_SUBSEC_USLEEP
    unsigned int seconds;
#endif

    elapsedtime.tv_sec = window; /* start main loop off on the right foot */
    bytesread = 0;

    done = false;
    do { /* while (!done) */

	ptr = block;
	nleft = blocksize;
	do {
	    nread = read(STDIN_FILENO, ptr, nleft);
	    if (nread < 0) {
		perror(THROTTLE ": read");
		return -1;
	    }

	    if (nread == 0) {
		done = true;
		break; /* EOF */
	    }

	    nleft -= nread;
	    ptr += nread;
	} while (nleft > 0);

	nread = (blocksize - nleft);
	bytesread += nread;


	if (elapsedtime.tv_sec >= window ||
	    bytesread < 0 /* check rollover */)
	{
	    elapsedtime.tv_sec = 0;
	    bytesread = nread;

	    if (gettimeofday(&starttime, NULL) == -1) {
		perror(THROTTLE ": gettimeofday");
		return -1;
	    }

	    sync = BSECS(bytesread, Bps);
	}
	else
	{
	    if (gettimeofday(&currenttime, NULL) == -1) {
		perror(THROTTLE ": gettimeofday");
		return -1;
	    }

	    timersub(&currenttime, &starttime, &elapsedtime);

	    sync = BSECS(bytesread, Bps) - TSECS(&elapsedtime);
	}

#if HAVE_NANOSLEEP
	if (sync >= 0.000000001) {
	    ts_sync.tv_sec = (time_t)sync;
	    ts_sync.tv_nsec = (long)((sync - ts_sync.tv_sec) * 1e+9);
	    while (nanosleep(&ts_sync, &ts_sync) != 0 && errno == EINTR)
		/* nanosleep remainder */;
	}
#else
# if HAVE_SUBSEC_USLEEP
	if (sync >= 1.0) {
	    /* on some systems, usleep with a value >= 1000000
	     * is considered an error
	     */
	    seconds = (unsigned int)sync;
	    while ((seconds = sleep(seconds)) != 0)
		/* sleep remainder */;

	    sync -= (double)((unsigned int)sync);

	    /* protect against rounding to 1000000
	     * when calculating (unsigned int)(sync * 1000000)
	     */
	    if (sync > 999999*0.000001)
		sync = 999999*0.000001;
	}
# endif
	if (sync >= 0.000001)
	    usleep((unsigned int)(sync * 1000000));
#endif

	ptr = block;
	nleft = nread;
	while (nleft > 0) {
	    nwritten = write(STDOUT_FILENO, ptr, nleft);
	    if (nwritten <= 0) {
		perror(THROTTLE ": write");
		return -1;
	    }

	    nleft -= nwritten;
	    ptr += nwritten;
	}

    } while (!done);

    return 0;
}


static void usage() {
    fprintf(stderr,
	"Usage: " THROTTLE " [-V] [-s blocksize] [-w window] [-bkmBKM] limit\n"
	"     limit      - Bandwidth limit.\n"
	"  -b, -k, -m    - bits, kilobits or megabits per second.\n"
	"  -B, -K, -M    - Bytes, Kilobytes or Megabytes per second.\n"
	"  -s blocksize  - Block size for input and output.\n"
	"  -w window     - Window of time in seconds.\n"
	"  -V            - Print the version number and copyright and exit.\n"
	"  -h            - Display this message and exit.\n");
}

#define DEFBLOCKSIZE 512

int main(int argc, char **argv)
{
    int c, err;
    double Bps, limit, unit;
    long larg;
    time_t window;
    size_t blocksize;
    void *block;
    char defblock[DEFBLOCKSIZE];

    /* defaults */
    unit = 1.0; /* bytes per second */
    window = 60;
    blocksize = DEFBLOCKSIZE;

    while ((c = getopt(argc, argv, "s:w:bkmBKMVh")) != -1) {
	switch (c) {

	case 's':
	    if ((larg = atol(optarg)) < 1) {
		fprintf(stderr, THROTTLE ": invalid blocksize %s\n", optarg);
		return 1;
	    }
	    blocksize = (size_t)larg;
	    break;

	case 'w':
	    if ((larg = atol(optarg)) < 1) {
		fprintf(stderr, THROTTLE ": invalid window size %s\n", optarg);
		return 1;
	    }
	    window = (time_t)larg;
	    break;

	case 'b':
	    unit = 1.0/8.0;
	    break;

	case 'k':
	    unit = 1024.0/8.0;
	    break;

	case 'm':
	    unit = (1024.0*1024.0)/8.0;
	    break;

	case 'B':
	    unit = 1.0;
	    break;

	case 'K':
	    unit = 1024.0;
	    break;

	case 'M':
	    unit = 1024.0*1024.0;
	    break;

	case 'V':
	    fprintf(stdout,
		THROTTLE " " VERSION "\n"
		"Copyright 2003, 2004 James Klicman <james@klicman.org>\n"
		"This is free software; see the source for copying conditions.  There is NO\n"
		"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	    return 0;

	case 'h':
	default:
	    usage();
	    return 1;
	}
    }

    if (optind == (argc - 1)) {
	limit = strtod(argv[optind], (char **)NULL);
	if (limit <= 0.0) {
	    fprintf(stderr, THROTTLE ": invalid limit %s\n", argv[optind]);
	    return 1;
	}

	Bps = limit * unit;
	
    } else {
	usage();
	return 1;
    }

    if (blocksize > DEFBLOCKSIZE) {
	block = malloc(blocksize);
	if (block == NULL) {
	    perror(THROTTLE ": malloc");
	    return 1;
	}
    } else {
	block = defblock;
    }

    err = throttle(Bps, window, block, blocksize);

    if (blocksize > DEFBLOCKSIZE)
	free(block);

    return (err == 0 ? 0 : 1);
}

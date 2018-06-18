#ifndef _DUMP_BUF_H
#define _DUMP_BUF_H

#define DEBUG
#ifdef DEBUG
	#include <stdio.h>
	#define dprintf(fmt, args...)   printf(fmt, ##args)
#else
        #define dprintf(fmt, args...)   do {} while (0)
#endif

static inline void dump_buf(const char *msg, const unsigned char *data, int len)
{
	int i;

	dprintf("%s (len = %u)\n", msg, len);

	for (i = 0; data && i < len; i++) {
		dprintf(" %02x", data[i]);
		if (!((i + 1) % 16))
			dprintf("\n");
	}
	dprintf("\n");
}

#endif


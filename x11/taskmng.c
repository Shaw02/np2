#include "compiler.h"

#include "np2.h"
#include "toolkit.h"

#include "taskmng.h"


void
taskmng_initialize(void)
{

	np2running = TRUE;
}

BOOL
taskmng_sleep(UINT32 tick)
{
	UINT32	base;

	base = GETTICK();
	while (taskmng_isavail() && ((GETTICK() - base) < tick)) {
		toolkit_event_process();
		usleep(960);
	}
	return taskmng_isavail();
}

void
taskmng_exit(void)
{

	np2running = FALSE;
}

/* Check whether <unistd.h> declares close_range, for HAVE_CLOSE_RANGE. */

/* Supplied by the build environment later. */
#define _GNU_SOURCE

#include <unistd.h>

void *volatile p;

int
main(void)
{
  p = (void *) close_range;
}

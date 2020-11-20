#ifndef __QUEUEDEF_H__
#define __QUEUEDEF_H__

#include <stdint.h>
#include <stddef.h>

typedef unsigned int qerr_t;

/* Most q APIs return "int ".  These definitions must
 * be kept in sync with the "qerrs" string array in "q.c". */
#define QERR_OK 0        /* Success */
#define QERR_BUG1 1      /* Internal software bug - should never happen */
#define QERR_BUG2 2      /* Internal software bug - should never happen */
#define QERR_BADSIZE 3   /* q_size parameter invalid */
#define QERR_MALLOCERR 4 /* No memory available */
#define QERR_FULL 5      /* No room in queue */
#define QERR_EMPTY 6     /* No messages in queue */
#define LAST_QERR 6      /* Set to value of last "QERR_*" definition */

/* Validate queue size (used on create new queue) */
qerr_t queue_size_is_valid(size_t q_size);

/* Returns a string representation of a queue API return error code.
 * qerr : value returned by most q APIs indicating success or faiure.
 * (See q.h for list of QERR_* definitions.) */
const char *queue_qerr_str(qerr_t qerr);

/* Returns a size, rounded to power of 2 (if size lower than 2, return always 2) */
size_t size_to_power_of_2(size_t q_size);

#endif // __QUEUEDEF_H__

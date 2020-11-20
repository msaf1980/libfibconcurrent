#include <stdlib.h>

#include <queuedef.h>

/* This list of strings must be kept in sync with the
 * corresponding "QERR_*" constant definitions in "spsc_queue.h".
 * It is used by the q_qerr_str() function. */
static const char *qerrs[] = {"QERR_OK",      "QERR_BUG1",      "QERR_BUG2",
                              "QERR_BADSIZE", "QERR_MALLOCERR", "QERR_FULL",
                              "QERR_EMPTY",   "BAD_QERR",       NULL};
#define BAD_QERR (sizeof(qerrs) / sizeof(qerrs[0]) - 2)

/* Internal function: return 1 if power of 2 */
static int is_power_2(size_t n) {
    /* Thanks to Alex Allain at
     * http://www.cprogramming.com/tutorial/powtwosol.html for this cute algo.
     */
    return ((n - 1) & n) == 0;
} /* is_power_2 */

const char *queue_qerr_str(qerr_t qerr) {
    if (qerr >= BAD_QERR) {
        return qerrs[BAD_QERR];
    } /* bad qerr */

    return qerrs[qerr];
} /* queue_qerr_str */

qerr_t queue_size_is_valid(size_t q_size) {
    /* Sanity check input size */
    if (q_size <= 1 || !is_power_2(q_size)) {
        return QERR_BADSIZE;
    }
    return QERR_OK;
} /* queue_size_is_valid */

size_t size_to_power_of_2(size_t q_size) {
    size_t v = 1;
    while (q_size >>= 1) {
        v <<= 1;
    }
    return v;
} /* num_to_power_of_2 */

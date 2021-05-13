#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <concurrent/queuedef.h>

#define CTEST_MAIN
#define CTEST_SEGFAULT

#include "ctest.h"

CTEST(queuedef, size_to_power_of_2) {
    ASSERT_EQUAL_U(1, size_to_power_of_2(0));
    ASSERT_EQUAL_U(1, size_to_power_of_2(1));

    ASSERT_EQUAL_U(2, size_to_power_of_2(2));

    ASSERT_EQUAL_U(4, size_to_power_of_2(6));
    ASSERT_EQUAL_U(8, size_to_power_of_2(8));

    ASSERT_EQUAL_U(8, size_to_power_of_2(9));
    ASSERT_EQUAL_U(8, size_to_power_of_2(10));

    ASSERT_EQUAL_U(16, size_to_power_of_2(17));

    ASSERT_EQUAL_U(32, size_to_power_of_2(33));

    ASSERT_EQUAL_U(1024, size_to_power_of_2(1025));

    ASSERT_EQUAL_U(2048, size_to_power_of_2(2049));

    ASSERT_EQUAL_U(1048576, size_to_power_of_2(1048577));
    ASSERT_EQUAL_U(1073741824, size_to_power_of_2(1073741825));
    
    ASSERT_EQUAL_U(SIZE_MAX / 2 + 1, size_to_power_of_2(SIZE_MAX));
}

int main(int argc, const char *argv[]) {
    return ctest_main(argc, argv);
} /* main */

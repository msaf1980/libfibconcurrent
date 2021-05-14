# Scan source dir for standart source files and exclude main.cpp

set(TEST_INCLUDES)
foreach(dir IN LISTS DIRS_INCLUDES SUB_DIRS)
    list(APPEND TEST_INCLUDES ../${dir})
endforeach()

include_directories(${TEST_INCLUDES})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(REQUIRED_SOURCES)

include(CheckFunctionExists)
set(CMAKE_REQUIRED_LIBRARIES Threads::Threads)
check_function_exists(pthread_barrier_init HAVE_PTHREAD_BARRIER)
if(HAVE_PTHREAD_BARRIER)
    ADD_DEFINITIONS(-DHAVE_PTHREAD_BARRIER=1)   
else()
    list(APPEND REQUIRED_SOURCES "pthread_barrier.c")
    ADD_DEFINITIONS(-DHAVE_PTHREAD_BARRIER=0)
endif()

set(TEST_LIBRARIES concurrent_static Threads::Threads)

# Build tests
file(GLOB testFiles *_test.c)
foreach(testFile ${testFiles})
    file(RELATIVE_PATH test_file ${CMAKE_CURRENT_SOURCE_DIR} ${testFile})
    string(REGEX REPLACE "^([0-9a-zA-Z_\\-]+)_test\.c.*$" "\\1" test
                         "${test_file}")
    add_executable(test_${test} ${test_file} ${REQUIRED_SOURCES})
    target_link_libraries(test_${test} ${TEST_LIBRARIES})
    add_test(NAME test_${test}
             COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_${test})
endforeach(testFile)

add_executable(bench_queue "queue_bench.c" ${REQUIRED_SOURCES})
target_link_libraries(bench_queue ${TEST_LIBRARIES})

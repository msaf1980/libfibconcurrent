set(SOURCES)
set(LIBRARIES)

# Scan dir for standart source files
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} FIBCONCURRENT_SOURCES)

if(BUILD_SHARED_LIBS)
    add_library(fibconcurrent SHARED ${FIBCONCURRENT_SOURCES})
    set_target_properties(
        fibconcurrent
        PROPERTIES OUTPUT_NAME fibconcurrent
                   SOVERSION "${VERSION_MAJOR}"
                   VERSION "${VERSION_STRING}")
endif()
add_library(fibconcurrent_static STATIC ${FIBCONCURRENT_SOURCES})
set_target_properties(fibconcurrent_static PROPERTIES OUTPUT_NAME fibconcurrent)

set(SOURCES)
set(LIBRARIES)

# Scan dir for standart source files
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} CONCURRENT_SOURCES)

if(BUILD_SHARED_LIBS)
    add_library(concurrent SHARED ${CONCURRENT_SOURCES})
    set_target_properties(
        concurrent
        PROPERTIES OUTPUT_NAME concurrent
                   SOVERSION "${VERSION_MAJOR}"
                   VERSION "${VERSION_STRING}")
endif()
add_library(concurrent_static STATIC ${CONCURRENT_SOURCES})
set_target_properties(concurrent_static PROPERTIES OUTPUT_NAME concurrent)

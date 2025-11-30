set(XORDLL_VERSION_MAJOR 1)
set(XORDLL_VERSION_MINOR 0)
set(XORDLL_VERSION_PATCH 0)
set(XORDLL_VERSION_BUILD 0)

set(XORDLL_VERSION "${XORDLL_VERSION_MAJOR}.${XORDLL_VERSION_MINOR}.${XORDLL_VERSION_PATCH}")
set(XORDLL_VERSION_FULL "${XORDLL_VERSION}.${XORDLL_VERSION_BUILD}")

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE XORDLL_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT XORDLL_GIT_HASH)
        set(XORDLL_GIT_HASH "unknown")
    endif()
else()
    set(XORDLL_GIT_HASH "unknown")
endif()

configure_file(
    "${CMAKE_SOURCE_DIR}/include/version.h.in"
    "${CMAKE_BINARY_DIR}/generated/version.h"
    @ONLY
)

message(STATUS "xorDLL version: ${XORDLL_VERSION_FULL}")

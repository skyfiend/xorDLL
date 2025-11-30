# Version.cmake - Version management for xorDLL project

set(XORDLL_VERSION_MAJOR 1)
set(XORDLL_VERSION_MINOR 0)
set(XORDLL_VERSION_PATCH 0)
set(XORDLL_VERSION_BUILD 0)

set(XORDLL_VERSION )
set(XORDLL_VERSION_FULL )

# Get git hash if available
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
        set(XORDLL_GIT_HASH )
    endif()
else()
    set(XORDLL_GIT_HASH )
endif()

# Configure version header
configure_file(
    
    
    @ONLY
)

message(STATUS )

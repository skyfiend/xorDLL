# xorDLL - Modern DLL Injector for Windows
# CMakeLists.txt - Main build configuration

cmake_minimum_required(VERSION 3.16)

# Project definition
project(xorDLL
    VERSION 1.0.0
    DESCRIPTION 
    HOMEPAGE_URL 
    LANGUAGES CXX
)

# Only build on Windows
if(NOT WIN32)
    message(FATAL_ERROR )
endif()

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH )
include(CompilerFlags)
include(Version)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Options
option(XORDLL_ENABLE_LOGGING  ON)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/generated
)

# Collect source files
file(GLOB_RECURSE CORE_SOURCES
    
)

file(GLOB_RECURSE UI_SOURCES
    
)

file(GLOB_RECURSE UTILS_SOURCES
    
)

file(GLOB_RECURSE CLI_SOURCES
    
)

set(MAIN_SOURCE
    
)

set(CLI_MAIN_SOURCE
    
)

# Resource file for Windows
set(RESOURCE_FILE
    
)

# Create executable
add_executable(${PROJECT_NAME} WIN32
    ${MAIN_SOURCE}
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${UTILS_SOURCES}
    ${RESOURCE_FILE}
)

# Link Windows libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    user32
    gdi32
    comctl32
    shell32
    advapi32
    dwmapi
    uxtheme
    psapi
    ntdll
    wintrust
    version
    crypt32
    gdiplus
    ole32
    msimg32
)

# Set subsystem to Windows
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    VS_DEBUGGER_WORKING_DIRECTORY 
)

# Enable visual styles (Common Controls 6.0)
if(MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE
        
    )
endif()

# Create CLI executable
add_executable(${PROJECT_NAME}_cli
    ${CLI_MAIN_SOURCE}
    ${CORE_SOURCES}
    ${CLI_SOURCES}
    ${UTILS_SOURCES}
)

target_link_libraries(${PROJECT_NAME}_cli PRIVATE
    user32
    shell32
    advapi32
    psapi
    ntdll
    version
    wintrust
)

set_target_properties(${PROJECT_NAME}_cli PROPERTIES
    OUTPUT_NAME 
    VS_DEBUGGER_WORKING_DIRECTORY 
)

# Install rules
install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_cli
    RUNTIME DESTINATION bin
)

install(DIRECTORY resources/
    DESTINATION share/xorDLL
    FILES_MATCHING PATTERN  PATTERN 
)

# CPack configuration for installer
set(CPACK_PACKAGE_NAME )
set(CPACK_PACKAGE_VENDOR )
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY )
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY )
set(CPACK_RESOURCE_FILE_LICENSE )
include(CPack)

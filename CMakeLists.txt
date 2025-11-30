cmake_minimum_required(VERSION 3.16)

project(xorDLL
    VERSION 1.0.0
    DESCRIPTION "Modern DLL Injector for Windows"
    LANGUAGES CXX
)

if(NOT WIN32)
    message(FATAL_ERROR "xorDLL can only be built on Windows")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(CompilerFlags)
include(Version)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(XORDLL_ENABLE_LOGGING "Enable logging system" ON)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/generated
)

file(GLOB_RECURSE CORE_SOURCES "src/core/*.cpp")
file(GLOB_RECURSE UI_SOURCES "src/ui/*.cpp")
file(GLOB_RECURSE UTILS_SOURCES "src/utils/*.cpp")
file(GLOB_RECURSE CLI_SOURCES "src/cli/*.cpp")

set(MAIN_SOURCE "src/main.cpp")
set(CLI_MAIN_SOURCE "src/cli_main.cpp")
set(RESOURCE_FILE "resources/xorDLL.rc")

add_executable(${PROJECT_NAME} WIN32
    ${MAIN_SOURCE}
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${UTILS_SOURCES}
    ${RESOURCE_FILE}
)

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

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

if(MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE
        "/MANIFESTDEPENDENCY:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'"
    )
endif()

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
    OUTPUT_NAME "xordll"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_cli
    RUNTIME DESTINATION bin
)

install(DIRECTORY resources/
    DESTINATION share/xorDLL
    FILES_MATCHING PATTERN "*.ico" PATTERN "*.png"
)

set(CPACK_PACKAGE_NAME "xorDLL")
set(CPACK_PACKAGE_VENDOR "skyfiend")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern DLL Injector for Windows")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "xorDLL")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
include(CPack)

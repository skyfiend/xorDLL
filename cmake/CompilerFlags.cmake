# CompilerFlags.cmake - Compiler settings for xorDLL project

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC specific flags
if(MSVC)
    # Warning level 4 and treat warnings as errors in Release
    add_compile_options(/W4)
    
    # Enable multi-processor compilation
    add_compile_options(/MP)
    
    # Use Unicode
    add_definitions(-DUNICODE -D_UNICODE)
    
    # Windows version targeting (Windows 7+)
    add_definitions(-D_WIN32_WINNT=0x0601 -DWINVER=0x0601)
    
    # Security flags
    add_compile_options(/GS)  # Buffer security check
    add_compile_options(/sdl) # Additional security checks
    
    # Debug configuration
    if(CMAKE_BUILD_TYPE STREQUAL )
        add_compile_options(/Od /Zi /RTC1)
        add_definitions(-D_DEBUG)
    endif()
    
    # Release configuration
    if(CMAKE_BUILD_TYPE STREQUAL )
        add_compile_options(/O2 /GL /Gy)
        add_compile_options(/WX)  # Treat warnings as errors
        add_definitions(-DNDEBUG)
        
        # Link-time optimization
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE )
    endif()
    
    # Disable specific warnings
    add_compile_options(/wd4100)  # Unreferenced formal parameter
endif()

# MinGW specific flags
if(MINGW)
    add_compile_options(-Wall -Wextra)
    add_compile_options(-Wno-unknown-pragmas -Wno-missing-field-initializers -Wno-unused-parameter -Wno-cast-function-type)
    add_definitions(-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
    add_definitions(-D_WIN32_WINNT=0x0601 -DWINVER=0x0601)
    
    if(CMAKE_BUILD_TYPE STREQUAL )
        add_compile_options(-g -O0)
    endif()
    
    if(CMAKE_BUILD_TYPE STREQUAL )
        # Optimization for size while keeping exceptions
        add_compile_options(-Os -s -fno-rtti)
        add_compile_options(-ffunction-sections -fdata-sections -fmerge-all-constants)
        add_compile_options(-fno-ident -fomit-frame-pointer -fvisibility=hidden)
        add_link_options(-s -Wl,--gc-sections -Wl,--strip-all)
        add_link_options(-Wl,--build-id=none -Wl,--no-insert-timestamp)
        # Static linking of libgcc and libstdc++ for smaller deployment
        add_link_options(-static-libgcc -static-libstdc++)
        add_definitions(-DNDEBUG)
    endif()
endif()

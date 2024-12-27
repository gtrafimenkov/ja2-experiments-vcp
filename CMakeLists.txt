cmake_minimum_required(VERSION 3.10)

add_subdirectory(libs/corrosion)
corrosion_import_crate(MANIFEST_PATH rustlib/Cargo.toml)

project(ja2vcp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
#     # add_compile_options(-Wno-unused-parameter)  # Example for GCC
# elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#     # add_compile_options(-Wno-unused-parameter)  # Example for Clang
# endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message(STATUS "Compiler: MSVC")

    add_compile_options(/utf-8)

    # # treating warnings as errors
    # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")

    add_compile_options(/wd4018) # signed/unsigned mismatch
    add_compile_options(/wd4200) # nonstandard extension used: zero-sized array in struct/union
    add_compile_options(/wd4244) # conversion from 'type1' to 'type2', possible loss of data
    add_compile_options(/wd4267) # conversion from 'size_t' to 'type', possible loss of data
    add_compile_options(/wd4996) # deprecated stuff in utf8cpp library

    # using release C runtime library in order to link with Rust static library
    add_compile_options(/MT) # release static
endif()

add_subdirectory(libs/sdl2 EXCLUDE_FROM_ALL)
# add_subdirectory(libs/googletest)

file(GLOB_RECURSE SRC_FILES ja2/*.cc ja2/*.c libs/slog/slog/*.c libs/smacker/libsmacker/*.c)

add_executable(ja2vcp ${SRC_FILES})
target_include_directories(ja2vcp PRIVATE ja2)
# target_include_directories(ja2vcp PRIVATE libs/googletest/googletest/include)
target_include_directories(ja2vcp PRIVATE libs/slog)
target_include_directories(ja2vcp PRIVATE libs/smacker)
target_include_directories(ja2vcp PRIVATE libs/utf8cpp/source)
target_include_directories(ja2vcp PRIVATE rustlib/include)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # fixing warning LNK4098: defaultlib 'LIBCMT' conflicts with use of other libs; use /NODEFAULTLIB:library
    target_link_options(ja2vcp PRIVATE /NODEFAULTLIB:libcmt)
endif()

if(WIN32)
    if(TARGET SDL2::SDL2main)
        target_link_libraries(ja2vcp PRIVATE SDL2::SDL2main)
    endif()
endif()

target_link_libraries(ja2vcp PRIVATE SDL2::SDL2-static)
target_link_libraries(ja2vcp PRIVATE rustlib)
# target_link_libraries(ja2vcp PRIVATE gtest)
if(WIN32)
    target_link_libraries(ja2vcp PRIVATE "netapi32.lib") # ggstd needs it
    target_link_libraries(ja2vcp PRIVATE "secur32.lib")  # ggstd needs it
endif()

cmake_minimum_required(VERSION 3.15)


project(Server LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc -Wall -Wextra")

add_definitions(-DMY_LITTLE_ENDIAN=0)
add_definitions(-DMY_BIG_ENDIAN=1)
add_definitions(-DMY_BYTE_ORDER=MY_LITTLE_ENDIAN)


if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "This project requires GCC. Detected compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0")
    message(FATAL_ERROR "GCC version must be at least 13.0 to support full C++23 features. Detected version: ${CMAKE_CXX_COMPILER_VERSION}")
endif()




set(SOURCES
    "main.cpp"
    "../lib/timer.cpp"
    "../lib/net_core.cpp"
    "../core/stun.cpp"
    ${PLATFORM_SOURCES}
)


add_executable(stun-server ${SOURCES})


target_include_directories(stun-server PRIVATE ../lib)
target_include_directories(stun-server PRIVATE ../core)

# link libraries
# target_link_libraries(stun-server PRIVATE some_library)
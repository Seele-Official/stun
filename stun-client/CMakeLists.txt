cmake_minimum_required(VERSION 3.15)


project(Client LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc -Wall -Wextra -fno-exceptions")

add_definitions(-DMY_LITTLE_ENDIAN=0)
add_definitions(-DMY_BIG_ENDIAN=1)
add_definitions(-DMY_BYTE_ORDER=MY_LITTLE_ENDIAN)


if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "This project requires GCC. Detected compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14.0")
    message(FATAL_ERROR "GCC version must be at least 14.0 to support full C++23 features. Detected version: ${CMAKE_CXX_COMPILER_VERSION}")
endif()

set(SOURCES
    "main.cpp"
    "nat_test.cpp"
    "../lib/timer.cpp"
    "../lib/net_core.cpp"
    "../core/stun.cpp"
    ${PLATFORM_SOURCES}
)


add_executable(stun-client ${SOURCES})

# link libraries
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(stun-client ws2_32)
    target_link_libraries(stun-client Iphlpapi)
endif()


target_include_directories(stun-client PRIVATE ../lib)
target_include_directories(stun-client PRIVATE ../core)

install(TARGETS stun-client
        RUNTIME DESTINATION bin)

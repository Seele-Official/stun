cmake_minimum_required(VERSION 3.15)


project(Client LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc -Wall -Wextra -g")

add_definitions(-DMY_LITTLE_ENDIAN=0)
add_definitions(-DMY_BIG_ENDIAN=1)
add_definitions(-DMY_BYTE_ORDER=MY_LITTLE_ENDIAN)


set(CMAKE_BUILD_TYPE "Debug")



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
    target_link_libraries(stun-client Bcrypt)
endif()


target_include_directories(stun-client PRIVATE ../lib)
target_include_directories(stun-client PRIVATE ../core)

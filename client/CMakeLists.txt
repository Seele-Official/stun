# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.15)

# 项目名称和语言
project(Client LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc -g")

add_definitions(-DMY_LITTLE_ENDIAN=0)
add_definitions(-DMY_BIG_ENDIAN=1)
add_definitions(-DMY_BYTE_ORDER=MY_LITTLE_ENDIAN)


set(CMAKE_BUILD_TYPE "Debug")


# 设置源文件
set(SOURCES
    "main.cpp"
    "nat_test.cpp"
    "./lib/timer.cpp"
    "./lib/net_core.cpp"
    "./core/stun.cpp"
    ${PLATFORM_SOURCES}
)

# 创建可执行文件
add_executable(stun-client ${SOURCES})

# Windows 需要链接 Winsock 库
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(stun-client ws2_32)
    target_link_libraries(stun-client Iphlpapi)
    target_link_libraries(stun-client Bcrypt)
endif()


target_include_directories(stun-client PRIVATE lib)
target_include_directories(stun-client PRIVATE core)

# 如果有额外的库依赖，可以在这里添加链接指令
# target_link_libraries(MyCpp20Project PRIVATE some_library)
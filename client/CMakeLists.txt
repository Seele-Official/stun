# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.15)

# 项目名称和语言
project(Client LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc -g")

add_definitions(-DMY_BYTE_ORDER=MY_LITTLE_ENDIAN)


set(CMAKE_BUILD_TYPE "Debug")

# 检测操作系统并选择对应的源文件
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Building for Linux")
    set(PLATFORM_SOURCES "linux_client.cpp")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "Building for Windows")
    set(PLATFORM_SOURCES "win_client.cpp")
    add_definitions(-D_WIN32_WINNT=0x0601) # Windows 7 及以上
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

# 设置源文件
set(SOURCES
    "main.cpp"
    "nat_test.cpp"
    "./include/timer.cpp"
    "./include/stun.cpp"
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


target_include_directories(stun-client PRIVATE include)


# 如果有额外的库依赖，可以在这里添加链接指令
# target_link_libraries(MyCpp20Project PRIVATE some_library)
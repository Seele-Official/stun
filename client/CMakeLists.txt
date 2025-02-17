# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.15)

# 项目名称和语言
project(Client LANGUAGES CXX)

# 设置 C++20 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc -g")

add_definitions(-DMY_BYTE_ORDER=MY_LITTLE_ENDIAN)


# 设置 Debug 模式（可选）
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
    "nat_test.cpp"
    "./include/timer.cpp"
    ${PLATFORM_SOURCES}
)

# 创建可执行文件
add_executable(client ${SOURCES})

# Windows 需要链接 Winsock 库
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(client ws2_32)
    target_link_libraries(client Iphlpapi)
    target_link_libraries(client Bcrypt)
endif()


target_include_directories(client PRIVATE include)

# 可选：启用 C++20 特性标志
# target_compile_features(MyCpp20Project PRIVATE cxx_std_20)

# 如果有额外的库依赖，可以在这里添加链接指令
# target_link_libraries(MyCpp20Project PRIVATE some_library)
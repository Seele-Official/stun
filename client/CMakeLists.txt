# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.15)

# 项目名称和语言
project(Client LANGUAGES CXX)

# 设置 C++20 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
add_definitions(-BYTE_ORDER=LITTLE_ENDIAN)
# DEBUG 模式
set(CMAKE_BUILD_TYPE "Debug")

# 设置支持 C++20 的编译器（可选，通常不需要）
# 如果需要，你可以指定编译器路径或要求特定版本的编译器
# set(CMAKE_CXX_COMPILER "/path/to/your/compiler")

file(GLOB SOURCES
    "client.cpp"
)
add_executable(Client ${SOURCES} )

# target_include_directories(Client PRIVATE include)
# target_include_directories(Client PRIVATE src)
# 可选：启用 C++20 特性标志
# target_compile_features(MyCpp20Project PRIVATE cxx_std_20)

# 如果有额外的库依赖，可以在这里添加链接指令
# target_link_libraries(MyCpp20Project PRIVATE some_library)

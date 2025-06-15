cmake_minimum_required(VERSION 3.15)


project(seele-lib LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SEELE_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_library(seele-lib STATIC ${SEELE_SRC})

target_compile_options(seele-lib PRIVATE -Wall -Wextra -fno-exceptions)



if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "This project requires GCC. Detected compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14.0")
    message(FATAL_ERROR "GCC version must be at least 14.0 to support full C++23 features. Detected version: ${CMAKE_CXX_COMPILER_VERSION}")
endif()


target_link_options(seele-lib INTERFACE -static-libstdc++ -static-libgcc) 

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(seele-lib ws2_32)
    target_link_libraries(seele-lib Iphlpapi)
endif()


target_include_directories(seele-lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>  
)



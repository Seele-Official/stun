cmake_minimum_required(VERSION 3.15)


project(Client LANGUAGES CXX)

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
  #include <cstdlib>
  int main() {
    void *p = std::aligned_alloc(16, 32);
    (void)p;
    return 0;
  }
" HAVE_ALIGNED_ALLOC)

if(HAVE_ALIGNED_ALLOC)
  add_compile_definitions(HAVE_ALIGNED_ALLOC=1)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc -Wall -Wextra -fno-exceptions")


if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "This project requires GCC. Detected compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14.0")
    message(FATAL_ERROR "GCC version must be at least 14.0 to support full C++23 features. Detected version: ${CMAKE_CXX_COMPILER_VERSION}")
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_subdirectory(lib)

add_executable(stun-client ${SOURCES})

# link libraries
target_link_libraries(stun-client PRIVATE seele-lib)


target_include_directories(stun-client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/stun)

install(TARGETS stun-client
        RUNTIME DESTINATION bin)

cmake_minimum_required(VERSION 3.15)
project(OSProject_CT353 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization and warning flags
if(MSVC)
    add_compile_options(/W4 /O2 /permissive-)
else()
    add_compile_options(-Wall -Wextra -O3 -pthread)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(COMMON_SOURCES
    src/main.cpp
    src/benchmark.cpp
    src/kalman_filter.cpp
)

if(WIN32)
    message(STATUS "Configuring build for Windows NT platform...")
    set(PLATFORM_SOURCES
        src/windows/syscalls_windows.cpp
        src/windows/kalman_windows.cpp
    )
    add_executable(os_project ${COMMON_SOURCES} ${PLATFORM_SOURCES})
    target_link_libraries(os_project PRIVATE psapi advapi32)
else()
    message(STATUS "Configuring build for Linux/POSIX platform...")
    set(PLATFORM_SOURCES
        src/linux/syscalls_linux.cpp
        src/linux/kalman_linux.cpp
    )
    add_executable(os_project ${COMMON_SOURCES} ${PLATFORM_SOURCES})
    target_link_libraries(os_project PRIVATE pthread rt)
endif()


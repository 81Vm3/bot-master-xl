cmake_minimum_required(VERSION 3.25)
project(BotMasterXL)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES
        "src/*.cpp"
        "src/*.c"
)
file(GLOB_RECURSE HEADERS
        "src/*.h"
        "src/*.hpp"
)

# 添加库
add_subdirectory(lib/RakNet)
add_subdirectory(lib/glm)
add_subdirectory(lib/spdlog)

# Enable SSL for libhv
#add_definitions(-DWITH_OPENSSL)
add_subdirectory(lib/libhv)
add_subdirectory(lib/sqlite)

# Bullet
set (BUILD_CPU_DEMOS OFF CACHE BOOL "Build original Bullet CPU examples")
set (BUILD_BULLET2_DEMOS OFF CACHE BOOL "Set when you want to build the Bullet 2 demos")
set (BUILD_EXTRAS OFF CACHE BOOL "Set when you want to build the extras")
set (BUILD_OPENGL3_DEMOS OFF CACHE BOOL "Set when you want to build the OpenGL3+ demos")
set (BUILD_UNIT_TESTS OFF CACHE BOOL "Build Unit Tests")
add_subdirectory (
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/bullet
)
set (BULLET_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/lib/bullet")
find_path(BULLET_INCLUDE_DIRS NAMES btBulletCollisionCommon.h
        HINTS
        ${BULLET_ROOT}/include
        ${BULLET_ROOT}/src
        PATH_SUFFIXES bullet
)

# include (FindBullet)

# ColAndreas
set (CA_VERSION "v1.4.0")
set (CA_DATABASE_VERSION 2)
add_definitions (-DCA_VERSION="${CA_VERSION}" -DCA_DATABASE_VERSION=${CA_DATABASE_VERSION})

# Find required packages
#find_package(OpenSSL REQUIRED)

add_executable(BotMasterXL ${SOURCES})

# Set output directory for the executable
set_target_properties(BotMasterXL PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

target_include_directories(BotMasterXL PRIVATE
        src
        lib/RakNet/src
        lib/libhv/include
        lib/glm
        lib/spdlog/include
        lib/sqlite
        lib/bullet/src
)
target_link_libraries(BotMasterXL
        RakNet
        glm::glm
        spdlog
        hv
        sqlite
        BulletInverseDynamics
        BulletSoftBody
        BulletDynamics
        BulletCollision
        LinearMath
        Bullet3Common
        ${CMAKE_THREAD_LIBS_INIT}
)
target_compile_options(BotMasterXL PRIVATE -static-libgcc -static-libstdc++)

if(UNIX AND NOT APPLE)
    message("---------Compiling on Linux---------")
    target_link_libraries(BotMasterXL ${CMAKE_THREAD_LIBS_INIT})
    target_compile_options(BotMasterXL PRIVATE
            -fno-strict-aliasing
            -fwrapv
            -fstack-protector-strong
            -D_GNU_SOURCE
            -pthread
    )
    target_compile_definitions(BotMasterXL PRIVATE _GNU_SOURCE)
endif()

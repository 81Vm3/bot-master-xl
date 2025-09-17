cmake_minimum_required(VERSION 3.25)
project(BotMasterXL)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Fix endian macro conflicts for cross-compilation
    add_definitions(-DLITTLE_ENDIAN=1234)
    add_definitions(-DBIG_ENDIAN=4321)
    add_definitions(-DBYTE_ORDER=1234)
    # Fix std::byte conflict with Windows headers
    add_definitions(-D_HAS_STD_BYTE=0)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # iconv port
    add_subdirectory(lib/win-iconv)
endif ()

file(GLOB_RECURSE SOURCES
        "src/*.cpp"
        "src/*.c"
)
file(GLOB_RECURSE HEADERS
        "src/*.h"
        "src/*.hpp"
)

# 添加库
set(OPENSSL_USE_STATIC_LIBS TRUE)
if(CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # For MinGW cross-compilation, set OpenSSL paths manually
    set(OPENSSL_ROOT_DIR /usr/x86_64-w64-mingw32)
    set(OPENSSL_INCLUDE_DIR /usr/x86_64-w64-mingw32/include)
    set(OPENSSL_CRYPTO_LIBRARY /usr/x86_64-w64-mingw32/lib/libcrypto.a)
    set(OPENSSL_SSL_LIBRARY /usr/x86_64-w64-mingw32/lib/libssl.a)
elseif(UNIX AND NOT APPLE)
    # For Linux static linking, set OpenSSL paths manually
    set(OPENSSL_CRYPTO_LIBRARY /usr/lib/x86_64-linux-gnu/libcrypto.a)
    set(OPENSSL_SSL_LIBRARY /usr/lib/x86_64-linux-gnu/libssl.a)
endif ()
find_package(OpenSSL REQUIRED)
add_subdirectory(lib/RakNet)
add_subdirectory(lib/glm)
add_subdirectory(lib/spdlog)
add_subdirectory(lib/miniz)

# Enable SSL for libhv
add_definitions(-DWITH_OPENSSL)
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

# ColAndreas
set (CA_VERSION "v1.4.0")
set (CA_DATABASE_VERSION 2)
add_definitions (-DCA_VERSION="${CA_VERSION}" -DCA_DATABASE_VERSION=${CA_DATABASE_VERSION})
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
        lib/miniz
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_include_directories(BotMasterXL PRIVATE
            lib/win-iconv
    )
endif ()

target_link_libraries(BotMasterXL
        RakNet
        glm::glm
        spdlog
        hv_static
        sqlite
        BulletInverseDynamics
        BulletSoftBody
        BulletDynamics
        BulletCollision
        LinearMath
        Bullet3Common
        miniz
        OpenSSL::SSL
        OpenSSL::Crypto
)
target_compile_options(BotMasterXL PRIVATE -static -static-libgcc -static-libstdc++ -O2)
target_link_options(BotMasterXL PRIVATE -static -static-libgcc -static-libstdc++)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message("---------Compiling on Windows---------")
    target_link_libraries(BotMasterXL
            ws2_32
            iconv_static
    )
elseif(UNIX AND NOT APPLE)
    message("---------Compiling on Linux---------")
    target_compile_options(BotMasterXL PRIVATE
            -fno-strict-aliasing
            -fwrapv
            -fstack-protector-strong
            -D_GNU_SOURCE
            -pthread
    )
    target_compile_definitions(BotMasterXL PRIVATE _GNU_SOURCE)
endif()

cmake_minimum_required(VERSION 3.20)

# Standalone configuration when built directly
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(EterBase LANGUAGES C CXX)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    if(WIN32)
        add_compile_definitions(WIN32 _WINDOWS _CRT_SECURE_NO_WARNINGS)
    else()
        add_compile_definitions(__LINUX__ _GNU_SOURCE)
        add_compile_options(
            -Wall
            -Wextra
            -Wno-unused-parameter
            -Wno-invalid-offsetof
            -Wno-multichar
            -Wno-sign-compare
            -fPIC
        )
    endif()

    option(BUILD_TESTING "Build EterBase unit tests" ON)
    if(BUILD_TESTING)
        enable_testing()
    endif()
endif()

# Dependencies
find_package(PkgConfig QUIET)
find_library(LZO_LIBRARY NAMES lzo2 lzo REQUIRED)

# Source files
set(ETERBASE_SOURCES
    CPostIt.cpp
    CPostIt.h
    CRC32.cpp
    CRC32.h
    Debug.cpp
    Debug.h
    error.cpp
    error.h
    FileBase.cpp
    FileBase.h
    FileDir.cpp
    FileDir.h
    FileLoader.cpp
    FileLoader.h
    Filename.h
    lzo.cpp
    lzo.h
    MappedFile.cpp
    MappedFile.h
    Poly/Base.cpp
    Poly/Base.h
    Poly/Poly.cpp
    Poly/Poly.h
    Poly/Symbol.cpp
    Poly/Symbol.h
    Poly/SymTable.cpp
    Poly/SymTable.h
    Random.cpp
    Random.h
    ServiceDefs.h
    Singleton.h
    StdAfx.cpp
    StdAfx.h
    Stl.cpp
    Stl.h
    tea.cpp
    tea.h
    TempFile.cpp
    TempFile.h
    Timer.cpp
    Timer.h
    Utils.cpp
    Utils.h
    vk.h
)

add_library(EterBase STATIC ${ETERBASE_SOURCES})
add_library(EterBase::EterBase ALIAS EterBase)

target_include_directories(EterBase
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Poly>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(EterBase
    PUBLIC
        ${LZO_LIBRARY}
)

# Unit Tests
if((CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND BUILD_TESTING) OR BUILD_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()

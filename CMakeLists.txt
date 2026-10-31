cmake_minimum_required(VERSION 3.20)

# Standalone configuration when built directly
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(EterBase LANGUAGES C CXX VERSION 1.0.0)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    if(NOT WIN32)
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
find_package(OpenGL REQUIRED)

# Source files
set(ETERBASE_SOURCES
    src/CPostIt.cpp
    src/CRC32.cpp
    src/Debug.cpp
    src/Direct3DDevice9Compat.cpp
    src/error.cpp
    src/FileBase.cpp
    src/FileDir.cpp
    src/FileLoader.cpp
    src/ImageLoaderCompat.cpp
    src/ImageWriterCompat.cpp
    src/backends/gl/GLCompat.cpp
    src/lzo.cpp
    src/MappedFile.cpp
    src/Random.cpp
    src/StdAfx.cpp
    src/Stl.cpp
    src/tea.cpp
    src/TempFile.cpp
    src/Timer.cpp
    src/Utils.cpp
    src/Poly/Base.cpp
    src/Poly/Poly.cpp
    src/Poly/Symbol.cpp
    src/Poly/SymTable.cpp
)

set(ETERBASE_HEADERS
    include/EterBase/CPostIt.h
    include/EterBase/CRC32.h
    include/EterBase/Debug.h
    include/EterBase/error.h
    include/EterBase/FileBase.h
    include/EterBase/FileDir.h
    include/EterBase/FileLoader.h
    include/EterBase/Filename.h
    include/EterBase/ImageLoaderCompat.h
    include/EterBase/lzo.h
    include/EterBase/MappedFile.h
    include/EterBase/Random.h
    include/EterBase/ServiceDefs.h
    include/EterBase/Singleton.h
    include/EterBase/StdAfx.h
    include/EterBase/Stl.h
    include/EterBase/tea.h
    include/EterBase/TempFile.h
    include/EterBase/Timer.h
    include/EterBase/Utils.h
    include/EterBase/vk.h
    include/EterBase/Poly/Base.h
    include/EterBase/Poly/Poly.h
    include/EterBase/Poly/Symbol.h
    include/EterBase/Poly/SymTable.h
    include/Platform/Platform.h
    include/Platform/GLCompat.h
    include/Platform/stb_image.h
    include/Platform/stb_image_write.h
)

add_library(EterBase STATIC
    ${ETERBASE_SOURCES}
    ${ETERBASE_HEADERS}
)
add_library(EterBase::EterBase ALIAS EterBase)

# Platform macros must be PUBLIC: consumers (standalone EterLib, ScriptLib,
# EterGrnLib, SpeedTreeLib, PRTerrainLib, EterPythonLib) include the Platform
# headers and only see definitions carried by the target they link (issue #79).
if(WIN32)
    target_compile_definitions(EterBase PUBLIC WIN32 _WINDOWS _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_definitions(EterBase PUBLIC
        OPENMT2_PLATFORM_LINUX=1
        OPENMT2_RENDERER_GL=1
        OPENMT2_SDL_WINDOW=1
        _GNU_SOURCE
    )
endif()

target_include_directories(EterBase
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/EterBase>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/EterBase/Poly>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Poly
)

target_link_libraries(EterBase
    PUBLIC
        ${LZO_LIBRARY}
        $<$<PLATFORM_ID:Linux>:OpenGL::GL>
)

# Unit Tests
if((CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND BUILD_TESTING) OR BUILD_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()

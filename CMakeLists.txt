# src/EterBase/CMakeLists.txt

find_package(PkgConfig QUIET)
find_library(LZO_LIBRARY NAMES lzo2 lzo REQUIRED)

add_library(EterBase STATIC
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

target_include_directories(EterBase
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Poly
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/extern/include
)

target_link_libraries(EterBase
    PUBLIC
        ${LZO_LIBRARY}
)

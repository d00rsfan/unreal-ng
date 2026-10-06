include_guard(GLOBAL)

option(UNREAL_USE_SYSTEM_LIBS "Use distribution shared libraries for native Linux packages" OFF)
if(UNREAL_USE_SYSTEM_LIBS)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(FATAL_ERROR "UNREAL_USE_SYSTEM_LIBS is supported only on desktop Linux")
    endif()
    if(UNREAL_BUNDLE_QT)
        message(FATAL_ERROR "UNREAL_USE_SYSTEM_LIBS and UNREAL_BUNDLE_QT are mutually exclusive")
    endif()

    # Missing shared development libraries must fail configuration, not silently
    # select a static archive. Project-specific vendored targets remain static.
    set(CMAKE_FIND_LIBRARY_SUFFIXES .so)
    set(OPENSSL_USE_STATIC_LIBS OFF)
    set(USE_STATIC_LIBS_ONLY OFF CACHE BOOL "Use only static libraries as dependencies" FORCE)
    set(BUILD_ZLIB OFF CACHE BOOL "Build zlib from source" FORCE)
    find_package(PkgConfig REQUIRED)
endif()

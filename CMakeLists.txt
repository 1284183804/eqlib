cmake_minimum_required(VERSION 3.16)
project(eqlib VERSION 0.9.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(EQLIB_BUILD_STATIC "Build static library" ON)
option(EQLIB_BUILD_SHARED "Build shared library" ON)
option(EQLIB_BUILD_TESTS  "Build tests" ON)
option(EQLIB_INSTALL      "Generate install target" ON)

set(EQLIB_SOURCES
    nbym/rz.cpp
    nbym/wav_common.cpp
    nbym/jhqhx.cpp
    nbym/dtjhq.cpp
    nbym/neizhiqx.cpp
    nbym/caiyang.cpp
    nbym/cl.cpp
    nbym/zc.cpp
    nbym/ppfx.cpp
    nbym/zdjhq.cpp
    nbym/zdzy.cpp
    nbym/bh.cpp
    nbym/dtd.cpp
    nbym/wjdx.cpp
    nbym/nccwj.cpp
    nbym/ypdr.cpp
    nbym/xxwjhq.cpp
    nbym/hhxw.cpp
    nbym/hxhc.cpp
    nbym/eq.cpp
    nbym/dr_impl.cpp
    nbym/dwjk.cpp
)

if(NOT EQLIB_BUILD_STATIC AND NOT EQLIB_BUILD_SHARED)
    message(FATAL_ERROR "At least one of EQLIB_BUILD_STATIC or EQLIB_BUILD_SHARED must be ON")
endif()

find_package(Threads REQUIRED)

if(EQLIB_BUILD_STATIC)
    add_library(eqlib_static STATIC ${EQLIB_SOURCES})

    target_include_directories(eqlib_static PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/dyjk
        ${CMAKE_CURRENT_SOURCE_DIR}/nbym
    )

    target_include_directories(eqlib_static PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/dsf
    )

    target_compile_features(eqlib_static PUBLIC cxx_std_20)

    set_target_properties(eqlib_static PROPERTIES
        OUTPUT_NAME eqlib
    )

    target_link_libraries(eqlib_static PRIVATE Threads::Threads)

    if(MSVC)
        target_compile_options(eqlib_static PRIVATE /W4 /permissive-)
    else()
        target_compile_options(eqlib_static PRIVATE -Wall -Wextra)
    endif()
endif()

if(EQLIB_BUILD_SHARED)
    add_library(eqlib_shared SHARED ${EQLIB_SOURCES})

    target_include_directories(eqlib_shared PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/dyjk
        ${CMAKE_CURRENT_SOURCE_DIR}/nbym
    )

    target_include_directories(eqlib_shared PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/dsf
    )

    target_compile_features(eqlib_shared PUBLIC cxx_std_20)

    target_compile_definitions(eqlib_shared PRIVATE EQLIB_BUILD_SHARED)

    set_target_properties(eqlib_shared PROPERTIES
        OUTPUT_NAME eqlib
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        CXX_VISIBILITY_PRESET hidden
        C_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    target_link_libraries(eqlib_shared PRIVATE Threads::Threads)

    if(MSVC)
        target_compile_options(eqlib_shared PRIVATE /W4 /permissive-)
        set_target_properties(eqlib_shared PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/shared
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
    endif()
endif()

if(EQLIB_BUILD_TESTS)
    enable_testing()

    if(EQLIB_BUILD_STATIC)
        set(EQLIB_TEST_LIB eqlib_static)
    else()
        set(EQLIB_TEST_LIB eqlib_shared)
    endif()

    set(EQLIB_TEST_NAMES
        cs_sejlbq
        cs_jhqhx
        cs_dtjhq
        cs_dsjhq
        cs_zdjhq
        cs_ppfx
        cs_zdzy
        cs_bh
        cs_dtd
        cs_wjdx
        cs_nccwj
        cs_mpmc
        cs_ypdr
        cs_ypdc
        cs_xxwjhq
        cs_hhxw
        cs_hxhc
        cs_eq
        cs_dwjk
        cs_stereo
    )

    foreach(test_name ${EQLIB_TEST_NAMES})
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cs/${test_name}.cpp)
            add_executable(${test_name} cs/${test_name}.cpp)
            target_link_libraries(${test_name} PRIVATE ${EQLIB_TEST_LIB})
            add_test(NAME ${test_name} COMMAND ${test_name})
        endif()
    endforeach()
endif()

if(EQLIB_INSTALL)
    include(GNUInstallDirs)

    if(EQLIB_BUILD_STATIC)
        install(TARGETS eqlib_static
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif()

    if(EQLIB_BUILD_SHARED)
        install(TARGETS eqlib_shared
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif()

    install(FILES
        dyjk/eqlib/eqlib.h
        dyjk/eqlib/eq.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/eqlib
    )
endif()

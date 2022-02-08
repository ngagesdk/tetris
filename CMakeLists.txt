cmake_minimum_required(VERSION 3.00)

if(DEFINED ENV{NGAGESDK})
    SET(NGAGESDK $ENV{NGAGESDK})
    set(CMAKE_TOOLCHAIN_FILE ${NGAGESDK}/cmake/ngage-toolchain.cmake)
else()
    message(FATAL_ERROR "The environment variable NGAGESDK needs to be defined.")
endif()

project(tetris CXX)

# Use CMake or Visual Studio to enable these settings.
option(INSTALL_EKA2L1 "Install app for EKA2L1" OFF)

set(UID1 0x1000007a) # KExecutableImageUidValue, e32uid.h
set(UID2 0x100039ce) # KAppUidValue16, apadef.h
set(UID3 0x101f7059) # tetris UID

set(GCC_COMN_DEFS -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__)
set(GCC_MODE_DEFS -DNDEBUG -D_UNICODE)
set(GCC_DEFS      ${GCC_COMN_DEFS} ${GCC_MODE_DEFS})

set(tetris_libs
    ${EPOC_LIB}/euser.lib
    ${EPOC_LIB}/apparc.lib
    ${EPOC_LIB}/avkon.lib
    ${EPOC_LIB}/bafl.lib
    ${EPOC_LIB}/bitgdi.lib
    ${EPOC_LIB}/cone.lib
    ${EPOC_LIB}/efsrv.lib
    ${EPOC_LIB}/eikcoctl.lib
    ${EPOC_LIB}/eikcore.lib
    ${EPOC_LIB}/eikdlg.lib
    ${EPOC_LIB}/estlib.lib
    ${EPOC_LIB}/fbscli.lib
    ${EPOC_LIB}/gdi.lib
    ${EPOC_LIB}/mediaclientimage.lib
    ${EPOC_LIB}/ws32.lib)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mut-1.00a")

set(tetris_sources
    ${SRC_DIR}/comp.cpp
    ${SRC_DIR}/mut.cpp
    ${SRC_DIR}/play.cpp
    ${SRC_DIR}/util.cpp)

add_library(tetris STATIC ${tetris_sources})
build_resource(${SRC_DIR} Mut "-D_SERIES60")
build_aif(${SRC_DIR} tetris ${UID3})
build_dll(tetris app ${UID1} ${UID2} ${UID3} "${tetris_libs}")

if(INSTALL_EKA2L1)
    copy_file(tetris.app ${CMAKE_CURRENT_BINARY_DIR} ${EKA2L1_E_DRIVE}/system/apps/tetris tetris.app)
    copy_file(tetris.app ${CMAKE_CURRENT_BINARY_DIR} ${EKA2L1_E_DRIVE}/system/apps/tetris tetris.aif)
    copy_file(tetris.app ${SRC_DIR}                  ${EKA2L1_E_DRIVE}/system/apps/tetris bg1.jpg)
    copy_file(tetris.app ${SRC_DIR}                  ${EKA2L1_E_DRIVE}/system/apps/tetris bg2.jpg)
    copy_file(tetris.app ${SRC_DIR}                  ${EKA2L1_E_DRIVE}/system/apps/tetris config.txt)
    copy_file_ex(Mut.rsc ${CMAKE_CURRENT_BINARY_DIR} ${EKA2L1_E_DRIVE}/system/apps/tetris Mut.rsc tetris.rsc)
endif()

add_dependencies(
    tetris.app
    tetris
    tetris.aif
    Mut.rsc)

target_compile_definitions(
    tetris
    PUBLIC
    __DLL__
    _SERIES60
    ${GCC_DEFS}
    UID1=${UID1}
    UID2=${UID2}
    UID3=${UID3})

target_include_directories(
    tetris
    PUBLIC
    ${SRC_DIR})

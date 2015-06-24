#
# Created by Matthieu Heitz on 6/9/15.
# Copyright (c) 2015 Kitware Inc. All rights reserved.
#

cmake_minimum_required( VERSION 2.8 )

# Set up project
set( PROJECT_NAME "IntersonManager" )
project( ${PROJECT_NAME} )

# Update CMake module path
set( ${PROJECT_NAME}_CMAKE_EXTENSIONS_DIR ${${PROJECT_NAME}_SOURCE_DIR}/CMake/Modules )
set( CMAKE_MODULE_PATH ${${PROJECT_NAME}_CMAKE_EXTENSIONS_DIR} ${CMAKE_MODULE_PATH} )

# TODO: modify FindLibUSB.cmake to handle version checking correctly
# Skip libusb version check, because FindLibUSB.cmake doesn't work very well yet.
set( LIBUSB_SKIP_VERSION_CHECK YES )

# Find libusb
find_package( LibUSB REQUIRED )

## If compiling for Android Lollipop (5.0), make Position Independent binaries
#if( ${CMAKE_SYSTEM_NAME} STREQUAL Android )
#  message(STATUS "Android: Build binaries as position independent" )
#  set( CMAKE_POSITION_INDEPENDENT_CODE YES )
#  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE -pie" )
#endif( ${CMAKE_SYSTEM_NAME} STREQUAL Android )

if( LIBUSB_FOUND )
  message( STATUS "LibUSB found.\n"
                  "LIBUSB_HEADER_FILE = ${LIBUSB_HEADER_FILE}\n"
                  "LIBUSB_INCLUDE_DIRS = ${LIBUSB_INCLUDE_DIRS}\n"
                  "LIBUSB_LIBRARIES = ${LIBUSB_LIBRARIES}\n"
                  "LIBUSB_VERSION = ${LIBUSB_VERSION}\n"
                  "LIBUSB_HAVE_GET_VERSION = ${LIBUSB_HAVE_GET_VERSION}" )
  # Specify libusb include directories
  include_directories( ${LIBUSB_INCLUDE_DIRS} )


  # Project CypressManager
  set( ${PROJECT_NAME}_SOURCES ${CMAKE_SOURCE_DIR}/main.cpp
                               ${CMAKE_SOURCE_DIR}/IntersonManager.cpp
                               ${CMAKE_SOURCE_DIR}/IntersonManager.h
                               ${CMAKE_SOURCE_DIR}/IntersonManagerTest.cpp
                               ${CMAKE_SOURCE_DIR}/IntersonManagerTest.h
                               )

  # Create target
  set( TARGET_NAME ${PROJECT_NAME} )
  add_executable( ${TARGET_NAME} ${${PROJECT_NAME}_SOURCES} )

  # Link against libusb libraries
  if( ${CMAKE_SYSTEM_NAME} STREQUAL Android )
    target_link_libraries( ${TARGET_NAME} ${LIBUSB_LIBRARIES} )
  else( ${CMAKE_SYSTEM_NAME} STREQUAL Android )
    target_link_libraries( ${TARGET_NAME} ${LIBUSB_LIBRARIES} pthread )
  endif( ${CMAKE_SYSTEM_NAME} STREQUAL Android )

  # Build libusb samples

  # FXLoad
  set( FXLOAD_SOURCES ${CMAKE_SOURCE_DIR}/fxload.c
                      ${CMAKE_SOURCE_DIR}/ezusb.c
                      ${CMAKE_SOURCE_DIR}/ezusb.h )
  add_executable( FxLoad ${FXLOAD_SOURCES} )
  target_link_libraries( FxLoad ${LIBUSB_LIBRARIES} )

  # ListDev
  set( LISTDEV_SOURCES ${CMAKE_SOURCE_DIR}/listdevs.c)
  add_executable( ListDev ${LISTDEV_SOURCES} )
  target_link_libraries( ListDev ${LIBUSB_LIBRARIES} )


else( LIBUSB_FOUND )
  message( ERROR "LibUSB not found.\n" )

endif( LIBUSB_FOUND )

cmake_minimum_required (VERSION 3.9)

set (PROJ_NAME debuglogger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project (${PROJ_NAME} VERSION 1.0 DESCRIPTION "Best C++ debug logger ever")

# for finding libraries
INCLUDE(FindPkgConfig)

find_package(Boost COMPONENTS system filesystem REQUIRED)

set(CMAKE_BUILD_TYPE Debug)

# Add include folders
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# Source Files
set(NEON_MIR_SRC
  ${PROJECT_SOURCE_DIR}/src/debuglogger.cpp
)

# Define the output executable
add_library(${PROJ_NAME} SHARED
  ${NEON_MIR_SRC})

set_target_properties(${PROJ_NAME} PROPERTIES SOVERSION 1)
set_target_properties(${PROJ_NAME} PROPERTIES PUBLIC_HEADER
                      "include/debuglogger.hpp")

set(INCLUDE_INSTALL_DIR include/ )
set(LIB_INSTALL_DIR lib/ )

include(CMakePackageConfigHelpers)

configure_package_config_file(
  ./metadata/${PROJ_NAME}Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJ_NAME}Config.cmake
  INSTALL_DESTINATION ${LIB_INSTALL_DIR}/${PROJ_NAME}/cmake
  PATH_VARS INCLUDE_INSTALL_DIR
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJ_NAME}ConfigVersion.cmake
  VERSION 1.0.0
  COMPATIBILITY SameMajorVersion
)

install(
  TARGETS ${PROJ_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJ_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJ_NAME}ConfigVersion.cmake
  DESTINATION ${LIB_INSTALL_DIR}/${PROJ_NAME}/cmake
)

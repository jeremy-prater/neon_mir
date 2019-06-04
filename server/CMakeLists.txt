cmake_minimum_required (VERSION 3.1)

set (PROJ_NAME neon-mir)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

project (${PROJ_NAME})

# for finding libraries
# CMAKE_INSTALL_PREFIX=/usr
INCLUDE(FindPkgConfig)

find_package(Boost COMPONENTS system filesystem REQUIRED)
message("${CMAKE_MODULE_PATH}")
find_package(debuglogger REQUIRED)

set(CMAKE_BUILD_TYPE Debug)

# Add include folders
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}")
message("${debuglogger_INCLUDE_DIRS}")
include_directories("${debuglogger_INCLUDE_DIR}")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# Source Files
set(NEON_MIR_SRC
  ${PROJECT_SOURCE_DIR}/src/session-server.cpp
  ${PROJECT_SOURCE_DIR}/src/audio-session.cpp
  ${PROJECT_SOURCE_DIR}/src/neon-mir.cpp
  ${PROJECT_SOURCE_DIR}/src/essentia.cpp
  ${PROJECT_SOURCE_DIR}/src/essentia-session.cpp
  neon.session.capnp.c++
)

# Generate capnp headers
execute_process(COMMAND capnp compile --src-prefix=../../metadata -oc++ ${CMAKE_SOURCE_DIR}/../metadata/neon.session.capnp)

# Define the output executable
add_executable(${PROJ_NAME} ${NEON_MIR_SRC})

target_link_libraries(${PROJ_NAME}
  essentia
  avformat
  avcodec
  avutil
  capnp
  capnp-rpc
  zmq
  kj
  kj-async
  yaml
  pthread
  rt
  ${Boost_LIBRARIES}
  ${debuglogger_LIBRARIES}
)

install(TARGETS ${PROJ_NAME} RUNTIME DESTINATION bin)
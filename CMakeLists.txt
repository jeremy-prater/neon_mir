cmake_minimum_required (VERSION 3.1)

set (PROJ_NAME neon-mir)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project (${PROJ_NAME})

# for finding libraries
INCLUDE(FindPkgConfig)

find_package(Boost COMPONENTS system filesystem REQUIRED)

set(CMAKE_BUILD_TYPE Debug)

# Add include folders
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}")

# Source Files
set(NEON_MIR_SRC
  ${PROJECT_SOURCE_DIR}/src/core-handler.cpp
  ${PROJECT_SOURCE_DIR}/src/debuglogger.cpp
  ${PROJECT_SOURCE_DIR}/src/audio-session.cpp
  ${PROJECT_SOURCE_DIR}/src/neon-mir.cpp
  ${PROJECT_SOURCE_DIR}/src/essentia.cpp
  ${PROJECT_SOURCE_DIR}/src/essentia-session.cpp
)

# Define the output executable
add_executable(${PROJ_NAME} ${NEON_MIR_SRC})

target_link_libraries(${PROJ_NAME} essentia avformat avcodec avutil yaml iio pthread rt ${Boost_LIBRARIES} libcppnetlib-client-connections.so libcppnetlib-server-parsers.so libcppnetlib-uri.so)

install(TARGETS ${PROJ_NAME} RUNTIME DESTINATION bin)
cmake_minimum_required (VERSION 3.1)

set (PROJ_NAME neon-mir)

project (${PROJ_NAME})

# for finding libraries
INCLUDE(FindPkgConfig)

set(CMAKE_BUILD_TYPE Debug)

# Add include folders
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}")

# Source Files
set(NEON_MIR_SRC
  ${PROJECT_SOURCE_DIR}/src/debuglogger.cpp
  ${PROJECT_SOURCE_DIR}/src/test1.cpp
)

# Define the output executable
add_executable(${PROJ_NAME} ${NEON_MIR_SRC})

target_link_libraries(${PROJ_NAME} pistache)

install(TARGETS ${PROJ_NAME} RUNTIME DESTINATION bin)
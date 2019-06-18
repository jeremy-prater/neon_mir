cmake_minimum_required (VERSION 3.1)

set (PROJ_NAME neon-spectrum-gfx)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

project (${PROJ_NAME})

# for finding libraries
INCLUDE(FindPkgConfig)

find_package(Boost COMPONENTS system filesystem REQUIRED)
find_package(debuglogger)
find_package(pulse-audio-input)

find_package(Magnum REQUIRED GL Shaders Sdl2Application)
find_package(Boost COMPONENTS system filesystem REQUIRED)
find_package(Corrade REQUIRED Utility)

set(CMAKE_BUILD_TYPE Debug)

# Add include folders
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/../core/include")
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

corrade_add_resource(Resources_data resources/test-gfx-data.conf)
corrade_add_resource(Resources_shaders resources/test-gfx-shaders.conf)

# Source Files GFX
set(NEON_GFX_SRC
  ${PROJECT_SOURCE_DIR}/src/test-gfx-1.cpp
  ${PROJECT_SOURCE_DIR}/src/test-shader-1.cpp
  neon.session.capnp.c++
)

# Source Files Audio
set(NEON_MIR_SRC
  ${PROJECT_SOURCE_DIR}/src/audio-processor.cpp
  neon.session.capnp.c++
)

# Generate capnp headers
execute_process(COMMAND capnp compile --src-prefix=../../../metadata -oc++ ${CMAKE_SOURCE_DIR}/../../metadata/neon.session.capnp)

# Define the output executable
# Define the output executable
add_executable(${PROJ_NAME} ${NEON_MIR_SRC} ${NEON_GFX_SRC} ${Resources_data} ${Resources_shaders})

target_link_libraries(${PROJ_NAME}
  zmq
  capnp
  capnp-rpc
  pthread 
  rt
  kj
  kj-async
  pulse
  ${Boost_LIBRARIES}
  ${debuglogger_LIBRARIES}
  ${pulse-audio-input_LIBRARIES}
  Magnum::Application
  Magnum::GL
  Magnum::Magnum
  Magnum::Shaders
  Corrade::Utility)


install(TARGETS ${PROJ_NAME} RUNTIME DESTINATION bin)
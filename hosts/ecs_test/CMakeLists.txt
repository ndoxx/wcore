# Defines an application for Entity Component System testing.
# Parent directory contains prototypes for game specific classes.
# The goal of the application is to:
#   - prototype some game logic and a set of data-oriented components usable by epsilon-core project
#   - develop an architecture for game object management and inter-component interactions
#   - integrate scripting abilities

project(epsilon)

include(toolchain_clang7)


add_definitions(-std=c++17)
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 ")

# include_directories(BEFORE "/usr/include/c++/8/")
# include_directories(AFTER "/usr/include/x86_64-linux-gnu/c++/8/")

set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE MinSizeRel)
# set(CMAKE_BUILD_TYPE RelWithDebInfo)

subdirs(source)

set(SRC_EPSILON
    source/src/weapon.cpp
    source/src/main.cpp
   )

add_executable(ecs ${SRC_EPSILON})

target_include_directories(ecs PRIVATE "${CMAKE_SOURCE_DIR}/hosts/ecs_test/source/include")
# TMP --------
target_include_directories(ecs PRIVATE "${CMAKE_SOURCE_DIR}/source/include")
# TMP --------

set_target_properties(ecs
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

add_library(iwcore SHARED IMPORTED)
set_target_properties(iwcore PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libwcore.so"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/source"
)

target_link_libraries(ecs
                      iwcore)
cotire(ecs)

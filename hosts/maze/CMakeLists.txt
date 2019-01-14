# Defines a test application which generates a maze and renders it
# using WCore engine.
# -> Will evolve to a mini-game
# -> To make it clearer how the API should look like

project(maze)

if(DEFINED CLANG6)
  include(toolchain_clang6)
else()
  include(toolchain_clang7)
endif()


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

set(SRC_MAZE
    source/src/main.cpp
    source/src/arguments.cpp
   )

add_executable(maze ${SRC_MAZE})

target_include_directories(maze PRIVATE "${CMAKE_SOURCE_DIR}/hosts/maze/source/include")

set_target_properties(maze
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

target_link_libraries(maze
                      iwcore)
cotire(maze)

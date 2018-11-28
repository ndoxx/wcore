# Defines an application for Entity Component System testing.
# Parent directory contains prototypes for game specific classes.
# The goal of the application is to:
#   - prototype some game logic and a set of data-oriented components usable by epsilon-core project
#   - develop an architecture for game object management and inter-component interactions
#   - integrate scripting abilities

# temporary directory for game classes
subdirs(epsilon)
include_directories(epsilon/include)
set(SRC_EPSILON
    epsilon/src/weapon.cpp
   )

SET (CMAKE_C_COMPILER             "/usr/bin/clang-6.0")
SET (CMAKE_C_FLAGS                "-Wall -std=c99")
SET (CMAKE_C_FLAGS_DEBUG          "-g")
SET (CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_C_FLAGS_RELEASE        "-O3 -DNDEBUG")
SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")

SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++-6.0")
SET (CMAKE_CXX_FLAGS                "-Wall -pedantic -Wno-unsupported-friend")
SET (CMAKE_CXX_FLAGS_DEBUG          "-g")
SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE        "-O3 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -fno-omit-frame-pointer -fno-inline-functions -fno-optimize-sibling-calls")

SET (CMAKE_AR      "/usr/bin/llvm-ar-6.0")
SET (CMAKE_LINKER  "/usr/bin/llvm-link-6.0")
SET (CMAKE_NM      "/usr/bin/llvm-nm-6.0")
SET (CMAKE_OBJDUMP "/usr/bin/llvm-objdump-6.0")
SET (CMAKE_RANLIB  "/usr/bin/llvm-ranlib-6.0")


add_definitions(-std=c++17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 ")

set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE MinSizeRel)
# set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_executable(ecs
               ${SRC_MATHS}
               ${SRC_CORE}
               ${SRC_CONTEXT}
               ${SRC_3D}
               ${SRC_RES}
               ${SRC_IMGUI}
			         ${SRC_EPSILON}
               main.cpp)

set_target_properties(ecs
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

link_directories(${CMAKE_SOURCE_DIR}/lib)

add_definitions(-D__DEBUG__)


add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLEW)

target_link_libraries(ecs
                      m
                      stdc++fs
                      GL
                      glfw
                      GLEW
                      png
                      z
                      freetype
                      pthread)

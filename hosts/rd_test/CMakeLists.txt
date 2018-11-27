# Defines a test application with basic scene setup
# to assess some aspects of:
#   - the content pipeline
#   - the rendering pipeline
#   - the chunk management system

SET (CMAKE_C_COMPILER             "/usr/bin/clang-7")
SET (CMAKE_C_FLAGS                "-Wall -std=c99")
SET (CMAKE_C_FLAGS_DEBUG          "-g")
SET (CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_C_FLAGS_RELEASE        "-O3 -DNDEBUG")
SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")

SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++-7")
SET (CMAKE_CXX_FLAGS                "-stdlib=libstdc++ -Wall -pedantic -Wno-unsupported-friend")
SET (CMAKE_CXX_FLAGS_DEBUG          "-g")
SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE        "-O3 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -fno-omit-frame-pointer -fno-inline-functions -fno-optimize-sibling-calls")

SET (CMAKE_AR      "/usr/bin/llvm-ar-7")
SET (CMAKE_LINKER  "/usr/bin/llvm-link-7")
SET (CMAKE_NM      "/usr/bin/llvm-nm-7")
SET (CMAKE_OBJDUMP "/usr/bin/llvm-objdump-7")
SET (CMAKE_RANLIB  "/usr/bin/llvm-ranlib-7")


add_definitions(-std=c++17)
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 ")

include_directories(BEFORE "/usr/include/c++/8/")
include_directories(AFTER "/usr/include/x86_64-linux-gnu/c++/8/")

set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE MinSizeRel)
# set(CMAKE_BUILD_TYPE RelWithDebInfo)

subdirs(source)
include_directories(source/include)
set(SRC_RD_TEST
    source/src/main.cpp
    source/src/arguments.cpp
   )

add_executable(wcore
               ${SRC_MATHS}
               ${SRC_CORE}
               ${SRC_3D}
               ${SRC_RES}
               ${SRC_IMGUI}
               ${SRC_RD_TEST})

set_target_properties(wcore
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${wcore_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${wcore_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${wcore_SOURCE_DIR}/bin"
)

link_directories(${CMAKE_SOURCE_DIR}/lib)

# add_definitions(-D__DISABLE_EDITOR__)
add_definitions(-D__DEBUG__)
# add_definitions(-D__PRESERVE_STRS__)
# add_definitions(-D__DEBUG_EXPORT_SPLINES__)
# add_definitions(-D__PROFILING_GAMELOOP__)
# add_definitions(-D__PROFILING_RENDERERS__)
# add_definitions(-D__PROFILING_CHUNKS__)
# add_definitions(-D__PROFILING_STOP_AFTER_X_SAMPLES__)
# add_definitions(-D__PROFILING_SET_1x1_VIEWPORT__)
# add_definitions(-D__PROFILING_SET_2x2_TEXTURE__)
# add_definitions(-D__OPTIM_BLOOM_USE_PP2__)
# add_definitions(-D__EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__)
# add_definitions(-D__EXPERIMENTAL_VSM_BLUR__)
# add_definitions(-D__OPTIM_LIGHT_VOLUMES_STENCIL_INVERT__)

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLEW)

target_link_libraries(wcore
                      m
                      stdc++fs
                      GL
                      glfw
                      GLEW
                      png
                      z
                      freetype
                      pthread)

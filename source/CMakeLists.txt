# defines wcore library

subdirs(src tests sandbox)

cmake_minimum_required(VERSION 3.10)
project(wcore VERSION 1.0.1 DESCRIPTION "WCore engine shared library")

if(DEFINED CLANG6)
  include(toolchain_clang6)
else()
  include(toolchain_clang7)
endif()


# Commented bc libzipios depends directly on C++11 ABI
#add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
set(CMAKE_CXX_STANDARD 17)

# include_directories(BEFORE "/usr/include/c++/8/")
# include_directories(AFTER "/usr/include/x86_64-linux-gnu/c++/8/")

set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE MinSizeRel)
# set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_library(wcore SHARED
            ${SRC_MATHS}
            ${SRC_CORE}
            ${SRC_3D}
            ${SRC_RES}
            ${SRC_IMGUI})

target_include_directories(wcore PUBLIC "${CMAKE_SOURCE_DIR}/source")
target_include_directories(wcore PRIVATE "${CMAKE_SOURCE_DIR}/source/include")
target_include_directories(wcore PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor")
set_target_properties(wcore PROPERTIES PUBLIC_HEADER "${CMAKE_SOURCE_DIR}/source/wcore.h")
set_target_properties(wcore PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(wcore PROPERTIES SOVERSION 1)
set_target_properties(wcore
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
target_compile_definitions(wcore PRIVATE WCORE_BUILD_LIB=1)


add_library(freetype STATIC IMPORTED)
set_target_properties(freetype PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libfreetype.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/source/vendor/freetype"
)

add_library(zipios STATIC IMPORTED)
set_target_properties(zipios PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libzipios.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/source/vendor/zipios"
)

add_library(fmod SHARED IMPORTED)
set_target_properties(fmod PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libfmod.so"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/source/vendor/fmod"
)

# add_definitions(-D__DISABLE_EDITOR__)
add_definitions(-D__DEBUG__)
add_definitions(-D__PROFILE__)
# add_definitions(-D__PRESERVE_STRS__)
# add_definitions(-D__DEBUG_EXPORT_SPLINES__)
# add_definitions(-D__PROFILING_GAMELOOP__)
# add_definitions(-D__PROFILING_CHUNKS__)
# add_definitions(-D__PROFILING_STOP_AFTER_X_SAMPLES__)
# add_definitions(-D__PROFILING_SET_1x1_VIEWPORT__)
# add_definitions(-D__PROFILING_SET_2x2_TEXTURE__)
# add_definitions(-D__OPTIM_BLOOM_USE_PP2__)
# add_definitions(-D__EXPERIMENTAL_VARIANCE_SHADOW_MAPPING__)
# add_definitions(-D__EXPERIMENTAL_VSM_BLUR__)
# add_definitions(-D__OPTIM_LIGHT_VOLUMES_STENCIL_INVERT__)

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLEW)

if(DEFINED __DEBUG__)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--export-dynamic")
endif()

target_link_libraries(wcore
                      m
                      stdc++fs
                      GL
                      glfw
                      GLEW
                      png
                      fmod
                      z
                      zipios
                      freetype
                      pthread)
cotire(wcore)

# Defines a test application with basic scene setup
# to assess some aspects of:
#   - the content pipeline
#   - the rendering pipeline
#   - the chunk management system

project(sandbox)

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

# include_directories(BEFORE "/usr/include/c++/8/")
# include_directories(AFTER "/usr/include/x86_64-linux-gnu/c++/8/")

set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE MinSizeRel)
# set(CMAKE_BUILD_TYPE RelWithDebInfo)

subdirs(source)

set(SRC_SANDBOX
    source/src/main.cpp
    source/src/arguments.cpp
   )

add_executable(sandbox ${SRC_SANDBOX})

target_include_directories(sandbox PRIVATE "${CMAKE_SOURCE_DIR}/hosts/sandbox/source/include")

set_target_properties(sandbox
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

target_link_libraries(sandbox
                      iwcore)
cotire(sandbox)

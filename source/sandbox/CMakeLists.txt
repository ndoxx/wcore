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

include_directories(${wcore_SOURCE_DIR}/source/include)

add_executable(detector_idiom
               detector_idiom.cpp)

set_target_properties(detector_idiom
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${wcore_SOURCE_DIR}/bin/sandbox"
)

link_directories(${CMAKE_SOURCE_DIR}/lib)

target_link_libraries(detector_idiom
                      m)


add_executable(shader_resource
               shader_resource.cpp)

set_target_properties(shader_resource
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${wcore_SOURCE_DIR}/bin/sandbox"
)

target_link_libraries(shader_resource
                      m)

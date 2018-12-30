# Tool for intern string lookup

project(internstr)

include(toolchain_clang7)

add_definitions(-std=c++17)
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
add_definitions(-D__DEBUG__)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# include_directories(BEFORE "/usr/include/c++/8/")
# include_directories(AFTER "/usr/include/x86_64-linux-gnu/c++/8/")

set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE MinSizeRel)
# set(CMAKE_BUILD_TYPE RelWithDebInfo)

subdirs(source)

set(SRC_INTERNSTR
    source/src/main.cpp
   )

add_executable(internstr ${SRC_INTERNSTR})

target_include_directories(internstr PRIVATE "${CMAKE_SOURCE_DIR}/hosts/internstr/source/include")
target_include_directories(internstr PRIVATE "${CMAKE_SOURCE_DIR}/source/include")

set_target_properties(internstr
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

target_link_libraries(internstr
                      iwcore)
cotire(internstr)

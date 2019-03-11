# Defines an application for "nuclear" tests

project(nuclear)

if(DEFINED CLANG6)
  include(toolchain_clang6)
else()
  include(toolchain_clang7)
endif()

#add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
add_definitions(-D__DEBUG__)
add_definitions(-D__IS_TOOL__)
set(CMAKE_CXX_STANDARD 17)

# include_directories(BEFORE "/usr/include/c++/8/")
# include_directories(AFTER "/usr/include/x86_64-linux-gnu/c++/8/")

set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE MinSizeRel)
# set(CMAKE_BUILD_TYPE RelWithDebInfo)

subdirs(source)

set(SRC_NUCLEAR
    source/src/main.cpp
   )

add_executable(nuclear ${SRC_NUCLEAR})

target_include_directories(nuclear PRIVATE "${CMAKE_SOURCE_DIR}/hosts/nuclear/source/include")
target_include_directories(nuclear PRIVATE "${CMAKE_SOURCE_DIR}/source/vendor/gnuplot")
target_include_directories(nuclear PRIVATE "${CMAKE_SOURCE_DIR}/source/include")

set_target_properties(nuclear
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

add_library(zipios STATIC IMPORTED)
set_target_properties(zipios PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libzipios.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/source/vendor/zipios"
)

target_link_libraries(nuclear
                      #zipios
                      #z
                      iwcore
                      boost_iostreams
                      boost_system
                      boost_filesystem
                      )
cotire(nuclear)

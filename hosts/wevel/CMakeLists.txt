cmake_minimum_required(VERSION 3.1.0)

# Tool for generating materials that can be loaded by WCore

project(wevel)

if(DEFINED CLANG6)
  include(toolchain_clang6)
else()
  include(toolchain_clang7)
endif()

add_definitions(-D__DEBUG__)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE MinSizeRel)
# set(CMAKE_BUILD_TYPE RelWithDebInfo)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)

# Find the QtWidgets library
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

set(CMAKE_AUTORCC ON)
qt5_add_resources(RCC_SOURCES resources.qrc)

# Populate a CMake variable with the sources
set(wevel_SRCS
    main.cpp
)

# Tell CMake to create the wevel executable
add_executable(wevel ${wevel_SRCS} ${RCC_SOURCES})

target_include_directories(wevel PRIVATE "${CMAKE_SOURCE_DIR}/source/include/")
target_include_directories(wevel PRIVATE "${CMAKE_SOURCE_DIR}/source/")

set_target_properties(wevel
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

add_library(iwcore SHARED IMPORTED)
set_target_properties(iwcore PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libwcore.so"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/source/include"
)

# Use the Widgets module from Qt 5
target_link_libraries(wevel GL GLEW iwcore Qt5::Widgets)
cotire(wevel)



# UNIT TESTS
set(test_wevel_SRCS
    tests/catch_app.cpp
    tests/transaction_test.cpp
)

add_executable(test_wevel ${test_wevel_SRCS})
target_include_directories(test_wevel PRIVATE "${CMAKE_SOURCE_DIR}/hosts/wevel/")
set_target_properties(test_wevel
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

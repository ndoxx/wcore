cmake_minimum_required(VERSION 3.1.0)

# Tool for generating materials that can be loaded by WCore

project(materialeditor)

if(DEFINED CLANG6)
  include(toolchain_clang6)
else()
  include(toolchain_clang7)
endif()

add_definitions(-std=c++17)
add_definitions(-D__DEBUG__)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 ")

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
find_package(Qt5Widgets CONFIG REQUIRED)

set(CMAKE_AUTORCC ON)
qt5_add_resources(RCC_SOURCES resources.qrc)

# Populate a CMake variable with the sources
set(materialeditor_SRCS
    editor_model.cpp
    normal_generator.cpp
    texlist_model.cpp
    texlist_delegate.cpp
    texmap_controls.cpp
    mainwindow.cpp
    new_project_dialog.cpp
    droplabel.cpp
    color_picker_label.cpp
    main.cpp
)
# Tell CMake to create the materialeditor executable
add_executable(materialeditor ${materialeditor_SRCS} ${RCC_SOURCES})

target_include_directories(materialeditor PRIVATE "${CMAKE_SOURCE_DIR}/hosts/materialeditor/source/include/")
target_include_directories(materialeditor PRIVATE "${CMAKE_SOURCE_DIR}/source/include/")
target_include_directories(materialeditor PRIVATE "${CMAKE_SOURCE_DIR}/source/")

set_target_properties(materialeditor
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

# Use the Widgets module from Qt 5
target_link_libraries(materialeditor iwcore Qt5::Widgets)
cotire(materialeditor)

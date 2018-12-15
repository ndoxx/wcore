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

include_directories(${CMAKE_SOURCE_DIR}/source)
include_directories(${CMAKE_SOURCE_DIR}/source/include)
include_directories(${CMAKE_SOURCE_DIR}/source/vendor)


set(SRC_MATHS_TEST
    ${CMAKE_SOURCE_DIR}/source/src/quaternion.cpp
    ${CMAKE_SOURCE_DIR}/source/src/math3d.cpp
    ${CMAKE_SOURCE_DIR}/source/src/bezier.cpp
    ${CMAKE_SOURCE_DIR}/source/src/numeric.cpp
    ${CMAKE_SOURCE_DIR}/source/src/gaussian.cpp
    ${CMAKE_SOURCE_DIR}/source/src/algorithms.cpp)

set(SRC_CORE_TEST
    ${CMAKE_SOURCE_DIR}/source/src/informer.cpp
    ${CMAKE_SOURCE_DIR}/source/src/listener.cpp
    ${CMAKE_SOURCE_DIR}/source/src/logger.cpp
    ${CMAKE_SOURCE_DIR}/source/src/colors.cpp
    ${CMAKE_SOURCE_DIR}/source/src/wentity.cpp
    ${CMAKE_SOURCE_DIR}/source/src/wcomponent.cpp)

set(SRC_3D_TEST
    ${CMAKE_SOURCE_DIR}/source/src/model.cpp
    ${CMAKE_SOURCE_DIR}/source/src/transformation.cpp
    ${CMAKE_SOURCE_DIR}/source/src/camera.cpp)

set(SRC_CONTEXT_TEST
    ${CMAKE_SOURCE_DIR}/source/src/gl_context.cpp)

set(SRC_RES_TEST
    ${CMAKE_SOURCE_DIR}/source/src/png_loader.cpp
    ${CMAKE_SOURCE_DIR}/source/src/pixel_buffer.cpp)

set(SRC_IMGUI
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/source/vendor/imgui/imgui_impl_opengl3.cpp)

add_executable(test_math
               catch_app.cpp
               catch_vec.cpp
               catch_mat.cpp
               catch_quat.cpp
               catch_bezier.cpp
               catch_cspline.cpp
               catch_algo.cpp
               catch_numeric.cpp
               ${SRC_MATHS_TEST}
               ${SRC_CORE_TEST})

set_target_properties(test_math
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

#add_definitions(-D__PRESERVE_STRS__)
add_definitions(-D__DISABLE_EDITOR__)
add_executable(test_engine_core
               catch_app.cpp
               catch_messaging.cpp
               catch_components.cpp
               ${SRC_CORE_TEST})

set_target_properties(test_engine_core
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

add_executable(test_engine_3d
               catch_app.cpp
               catch_vertex.cpp
               catch_mesh.cpp
               catch_transformation.cpp
               catch_cam.cpp
               ${SRC_CORE_TEST}
               ${SRC_3D_TEST}
               ${SRC_MATHS_TEST})

set_target_properties(test_engine_3d
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

target_link_libraries(test_engine_3d
                      m
                      GL
                      GLEW)

add_executable(test_resources
               catch_app.cpp
               catch_png.cpp
               ${SRC_CORE_TEST}
               ${SRC_RES_TEST})

set_target_properties(test_resources
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

#add_definitions(-D__DEBUG__)
target_link_libraries(test_resources
                      m
                      GL
                      GLEW
                      png)

add_executable(test_xml
               test_xml.cpp
               ${SRC_CORE_TEST})

set_target_properties(test_xml
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

target_link_libraries(test_xml
                      m)

add_definitions(-D__DEBUG__)
add_executable(test_logger
               test_logger.cpp
               ${SRC_CORE_TEST})

set_target_properties(test_logger
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

# add_definitions(-D__PRESERVE_STRS__)
add_executable(test_config
               catch_app.cpp
               catch_config.cpp
               ${CMAKE_SOURCE_DIR}/source/src/xml_utils.cpp
               ${CMAKE_SOURCE_DIR}/source/src/config.cpp
               ${SRC_CORE_TEST})

set_target_properties(test_config
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

add_definitions(-D__DEBUG_MODEL__)
add_executable(test_objload
               catch_app.cpp
               catch_objload.cpp
               ${CMAKE_SOURCE_DIR}/source/src/obj_loader.cpp
               ${CMAKE_SOURCE_DIR}/source/src/surface_mesh.cpp
               ${SRC_CORE_TEST})

set_target_properties(test_objload
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

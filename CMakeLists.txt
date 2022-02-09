# project
cmake_minimum_required(VERSION 3.20.0)
project(nanovg VERSION 0.1.0)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release/bin)

set(TARGET_NAME glad)
set(GLAD_DIR ${CMAKE_CURRENT_LIST_DIR} _external/glad)
add_library(${TARGET_NAME} ${GLAD_DIR}/src/glad.c)
target_include_directories(${TARGET_NAME} PUBLIC ${GLAD_DIR}/include)

set(TARGET_NAME glfw)
set(GLFW_DIR ${CMAKE_CURRENT_LIST_DIR} _external/glfw)
add_library(
  ${TARGET_NAME}
  ${GLFW_DIR}/src/context.c
  ${GLFW_DIR}/src/init.c
  ${GLFW_DIR}/src/input.c
  ${GLFW_DIR}/src/monitor.c
  ${GLFW_DIR}/src/platform.c
  ${GLFW_DIR}/src/vulkan.c
  ${GLFW_DIR}/src/window.c
  ${GLFW_DIR}/src/egl_context.c
  ${GLFW_DIR}/src/osmesa_context.c
  ${GLFW_DIR}/src/null_init.c
  ${GLFW_DIR}/src/null_monitor.c
  ${GLFW_DIR}/src/null_window.c
  ${GLFW_DIR}/src/null_joystick.c
  # win32
  ${GLFW_DIR}/src/win32_module.c
  ${GLFW_DIR}/src/win32_time.c
  ${GLFW_DIR}/src/win32_thread.c
  ${GLFW_DIR}/src/win32_init.c
  ${GLFW_DIR}/src/win32_joystick.c
  ${GLFW_DIR}/src/win32_monitor.c
  ${GLFW_DIR}/src/win32_window.c
  ${GLFW_DIR}/src/wgl_context.c)
target_compile_definitions(
  ${TARGET_NAME}
  PUBLIC _WIN32
  PRIVATE _GLFW_WIN32 GLFW_BUILD_WIN32 UNICODE _UNICODE)
target_compile_options(${TARGET_NAME} PRIVATE /wd4005)
target_include_directories(${TARGET_NAME} PUBLIC ${GLFW_DIR}/include)

set(TARGET_NAME nanovg)
add_library(${TARGET_NAME} src/nanovg.c)
target_include_directories(${TARGET_NAME} PUBLIC src)

set(TARGET_NAME example_gl3)
add_executable(
  ${TARGET_NAME}
  example/example_gl3.cpp
  example/GlfwApp.cpp
  example/demo.cpp
  example/perf.cpp
  src/nanovg_gl.cpp
  src/nanovg_gl_shader.cpp
  src/nanovg_gl_texture.cpp
  src/nanovg_gl_context.cpp
  src/renderer.cpp
  )
# target_compile_definitions(${TARGET_NAME} PRIVATE NANOVG_GL3_IMPLEMENTATION=1)
target_link_libraries(${TARGET_NAME} PRIVATE nanovg glfw glad OPENGL32.lib)

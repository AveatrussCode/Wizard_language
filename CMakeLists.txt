cmake_minimum_required(VERSION 3.20)
project(wizard_language LANGUAGES C CXX)

include(FetchContent)

# The UI stays in C. cimgui builds the Dear ImGui implementation internally as C++.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
  cimgui
  GIT_REPOSITORY https://github.com/cimgui/cimgui.git
  # Pinned commit: update deliberately, together with its imgui submodule.
  GIT_TAG 053280dfff63a74cc56a3e493671bee4bb6c60e4
  GIT_SHALLOW TRUE
  GIT_SUBMODULES_RECURSE TRUE
)
set(IMGUI_STATIC yes CACHE STRING "Build cimgui statically" FORCE)
FetchContent_MakeAvailable(cimgui)

find_package(OpenGL REQUIRED)

option(WIZARD_DEBUG_GC "Emit collector diagnostics to stderr" OFF)

# cimgui supplies the C API for the backends, while its base CMake target only
# builds core Dear ImGui. Build the GLFW/OpenGL3 implementations once here.
add_library(wizard_imgui_backend STATIC
  ${cimgui_SOURCE_DIR}/cimgui_impl.cpp
  ${cimgui_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp
  ${cimgui_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)
target_compile_features(wizard_imgui_backend PRIVATE cxx_std_11)
target_compile_definitions(wizard_imgui_backend PUBLIC
  CIMGUI_USE_GLFW
  CIMGUI_USE_OPENGL3
  IMGUI_IMPL_API=extern\ "C"
)
target_include_directories(wizard_imgui_backend PUBLIC ${cimgui_SOURCE_DIR})
target_include_directories(wizard_imgui_backend PRIVATE
  ${cimgui_SOURCE_DIR}/imgui
  ${cimgui_SOURCE_DIR}/imgui/backends
)
target_link_libraries(wizard_imgui_backend PUBLIC cimgui glfw OpenGL::GL)

add_executable(wizard
  main.c
  chunk.c
  compiler.c
  debug.c
  memory.c
  scanner.c
  value.c
  vm.c
  object.c
  table.c
  wizard_ui.c
)
target_compile_features(wizard PRIVATE c_std_11)
target_compile_definitions(wizard PRIVATE CIMGUI_USE_GLFW CIMGUI_USE_OPENGL3)
if(WIZARD_DEBUG_GC)
  target_compile_definitions(wizard PRIVATE DEBUG_LOG_GC)
endif()
target_link_libraries(wizard PRIVATE wizard_imgui_backend cimgui glfw OpenGL::GL m)

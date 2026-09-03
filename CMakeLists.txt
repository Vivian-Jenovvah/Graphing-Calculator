cmake_minimum_required(VERSION 3.16)
project(GraphingCalculator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -O2)
endif()

include(FetchContent)

# ---------------------------------------------------------------------------
# GLFW (windowing / input)
# ---------------------------------------------------------------------------
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
  GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ---------------------------------------------------------------------------
# Dear ImGui (immediate-mode GUI)
# ---------------------------------------------------------------------------
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.0
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)

# ---------------------------------------------------------------------------
# ImPlot (plotting widgets built on top of Dear ImGui)
# ---------------------------------------------------------------------------
FetchContent_Declare(
  implot
  GIT_REPOSITORY https://github.com/epezent/implot.git
  GIT_TAG        v0.16
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(implot)

add_library(implot STATIC
  ${implot_SOURCE_DIR}/implot.cpp
  ${implot_SOURCE_DIR}/implot_items.cpp
)
target_include_directories(implot PUBLIC ${implot_SOURCE_DIR})
target_link_libraries(implot PUBLIC imgui)

# ---------------------------------------------------------------------------
# OpenGL
# ---------------------------------------------------------------------------
find_package(OpenGL REQUIRED)

# ---------------------------------------------------------------------------
# Application
# ---------------------------------------------------------------------------
add_executable(GraphingCalculator
  src/main.cpp
  src/ExpressionEvaluator.cpp
  src/NumericalMethods.cpp
  src/GraphData.cpp
  src/CalculatorApp.cpp
)

target_include_directories(GraphingCalculator PRIVATE src)
target_link_libraries(GraphingCalculator PRIVATE imgui implot OpenGL::GL)

if(WIN32)
  target_link_libraries(GraphingCalculator PRIVATE opengl32)
endif()

if(APPLE)
  target_link_libraries(GraphingCalculator PRIVATE "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
endif()

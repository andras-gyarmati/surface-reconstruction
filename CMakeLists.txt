cmake_minimum_required(VERSION 3.24)
project(SurfaceReconstruction LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.32.8
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(SDL2)

set(SDL2IMAGE_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_VENDORED OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
set(SDL2IMAGE_BACKEND_IMAGEIO OFF CACHE BOOL "" FORCE)
foreach(format AVIF JXL TIF WEBP)
    set(SDL2IMAGE_${format} OFF CACHE BOOL "" FORCE)
endforeach()
FetchContent_Declare(SDL2_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-2.8.8
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(SDL2_image)

set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ONLY_LIBS ON CACHE BOOL "" FORCE)
FetchContent_Declare(glew
    GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
    GIT_TAG glew-cmake-2.3.1
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(glew)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9b
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL2::SDL2-static)

find_package(OpenGL REQUIRED)

set(APP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SurfaceReconstruction)
file(GLOB APP_SOURCES CONFIGURE_DEPENDS ${APP_DIR}/*.cpp ${APP_DIR}/Includes/*.cpp)
add_executable(SurfaceReconstruction ${APP_SOURCES})
target_include_directories(SurfaceReconstruction PRIVATE ${APP_DIR} ${APP_DIR}/Includes)
target_compile_definitions(SurfaceReconstruction PRIVATE GL_SILENCE_DEPRECATION GLM_ENABLE_EXPERIMENTAL $<$<CXX_COMPILER_ID:MSVC>:_USE_MATH_DEFINES>)
target_link_libraries(SurfaceReconstruction PRIVATE
    $<$<TARGET_EXISTS:SDL2::SDL2main>:SDL2::SDL2main>
    SDL2::SDL2-static SDL2_image::SDL2_image-static libglew_static glm::glm imgui OpenGL::GL)

foreach(asset_dir shaders inputs)
    add_custom_command(TARGET SurfaceReconstruction POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${APP_DIR}/${asset_dir} $<TARGET_FILE_DIR:SurfaceReconstruction>/${asset_dir})
endforeach()
add_custom_command(TARGET SurfaceReconstruction POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${APP_DIR}/imgui.ini $<TARGET_FILE_DIR:SurfaceReconstruction>/imgui.ini)

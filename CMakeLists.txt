# cmake -S . -B ./build

cmake_minimum_required(VERSION 3.16)
project(Swiftcanon VERSION 1.0.0)

file(GLOB_RECURSE source src/*.cpp src/*.hpp src/*.c src/*.h)
add_executable(${PROJECT_NAME} ${source})
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

add_subdirectory(vendor/glfw)
target_link_libraries(${PROJECT_NAME} glfw)

add_subdirectory(vendor/glm)
target_link_libraries(${PROJECT_NAME} glm)

add_subdirectory(vendor/spdlog)
target_link_libraries(${PROJECT_NAME} spdlog)

add_subdirectory(vendor/tinyobjloader)
target_link_libraries(${PROJECT_NAME} tinyobjloader)

find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

if (MSVC)
  # Set Swiftcanon as the Default Project
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif(MSVC)

if(WIN32)
  # for Windows OS in general
  add_definitions(-DWIN32)
elseif(APPLE)
  # for MacOS X or iOS, watchOS, tvOS (since 3.10.3)
  add_definitions(-DAPPLE)
elseif(UNIX AND NOT APPLE)
  # for Linux, BSD, Solaris, Minix
  add_definitions(-DUNIX)
endif()
# cmake -S . -B ./build

cmake_minimum_required(VERSION 3.14.0)
project(Swiftcanon VERSION 1.0.0)

file(GLOB_RECURSE source src/*.cpp src/*.hpp src/*.c src/*.h)
add_executable(${PROJECT_NAME} ${source})

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/vendor/glm)

add_subdirectory(vendor/glfw)
target_link_libraries(${PROJECT_NAME} glfw)

find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

if (MSVC)
    # Set Swiftcanon as the Default Project
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif(MSVC)
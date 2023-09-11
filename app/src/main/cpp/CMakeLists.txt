cmake_minimum_required(VERSION 3.18.1)
project(my_engine)

# Include the GameActivity static lib to the project.
find_package(game-activity REQUIRED CONFIG)
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u \
    Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR=1)

add_library(${PROJECT_NAME} SHARED
    vk_main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC
    vulkan
    game-activity::game-activity_static
    android
    log)

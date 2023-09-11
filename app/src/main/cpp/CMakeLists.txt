cmake_minimum_required(VERSION 3.18.1)
project(my_engine)

# Include the GameActivity static lib to the project.
find_package(game-activity REQUIRED CONFIG)
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u \
    Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

# my device doesn't support vk validation
# add_compile_definitions(WITH_VK_VALIDATION)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

if (CMAKE_BUILD_TYPE MATCHES Release)
    message("${CMAKE_CXX_FLAGS_RELEASE}")
else ()
    message("${CMAKE_CXX_FLAGS_DEBUG}")
endif ()

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/*.cpp")

add_library(${PROJECT_NAME} SHARED
        ${SOURCES})

target_link_libraries(${PROJECT_NAME} PUBLIC
        vulkan
        game-activity::game-activity_static
        android
        log)

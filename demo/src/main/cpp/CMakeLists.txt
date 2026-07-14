# 设置CMake最低版本
cmake_minimum_required(VERSION 3.22.1)
# 设置项目名称
project("lvgl_demo")
# 根据编译类型设置参数
if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Building Release...")
    add_compile_definitions(APP_DEBUG=0)
else ()
    message("Building Debug...")
    add_compile_definitions(APP_DEBUG=1)
endif ()
# 查找LVGL库
find_package(lvgl REQUIRED CONFIG)
# 添加库
add_library(
        ${CMAKE_PROJECT_NAME} SHARED
        DemoApp.cpp
)
target_include_directories(
        ${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
# 链接库
target_link_libraries(
        ${CMAKE_PROJECT_NAME}
        android
        log
        lvgl::lvgl
        lvgl::lvgl_demos
        lvgl::lvgl_examples
)
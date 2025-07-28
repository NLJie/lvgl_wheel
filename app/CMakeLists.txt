cmake_minimum_required(VERSION 3.15)
project(demo)

# 设置 UI 路径
set(UI_PATH ${CMAKE_SOURCE_DIR}/app/${UI_VARIANT})

# *********** 递归收集所有 .c/.cpp 文件作为源码 ***********
file(GLOB_RECURSE SOURCES
    ${UI_PATH}/*.c
    ${UI_PATH}/*.cpp
)

add_executable(demo ${SOURCES})

# *********** 收集所有的目录中的头文件 *******************
file(GLOB_RECURSE HEADER_FILES ${UI_PATH}/*.h)

set(INCLUDE_DIRS "")
foreach(hfile ${HEADER_FILES})
    get_filename_component(dir ${hfile} DIRECTORY)
    list(APPEND INCLUDE_DIRS ${dir})
endforeach()

list(REMOVE_DUPLICATES INCLUDE_DIRS)

# 调试
foreach(inc ${INCLUDE_DIRS})
    message(STATUS "Include dir: ${inc}")
endforeach()

# 添加头文件路径
target_include_directories(demo PRIVATE
    ${INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/lvgl
    ${CMAKE_SOURCE_DIR}/lvgl/src
    ${CMAKE_SOURCE_DIR}/component/hal
    ${CMAKE_SOURCE_DIR}/component/wifi
    ${CMAKE_SOURCE_DIR}/component/net
    ${CMAKE_SOURCE_DIR}/component/osal
    ${CMAKE_SOURCE_DIR}/component/font
    ${CMAKE_SOURCE_DIR}/component/player
    ${CMAKE_SOURCE_DIR}/component/usb_hid
    ${CMAKE_SOURCE_DIR}/component/net/inc
)

# 链接依赖库
target_link_libraries(demo PRIVATE
    lvgl
    lvgl_porting
    wifi
    http
    player
    font
    usb_hid
)

# 设置资源路径宏
if(SIMULATOR_LINUX)
    set(PROJECT_RES_URL ${PROJECT_BINARY_DIR}/${UI_VARIANT}/res/)
else()
    set(PROJECT_RES_URL /usr/res/)
endif()

target_compile_definitions(demo PUBLIC PROJECT_RES_URL="${PROJECT_RES_URL}")
message("PROJECT_RES_URL is ${PROJECT_RES_URL}")

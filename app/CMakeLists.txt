cmake_minimum_required(VERSION 3.15)

project(demo)

add_subdirectory(${UI_VARIANT}/res)

aux_source_directory(${UI_VARIANT}/ SOURCES)
aux_source_directory(${UI_VARIANT}/ui SOURCES)
aux_source_directory(${UI_VARIANT}/ui/data SOURCES)
aux_source_directory(${UI_VARIANT}/ui/utils SOURCES)
aux_source_directory(${UI_VARIANT}/ui/utils/ble SOURCES)
aux_source_directory(${UI_VARIANT}/ui/utils/clock SOURCES)
aux_source_directory(${UI_VARIANT}/ui/utils/file SOURCES)

add_executable(demo ${SOURCES})

target_include_directories(demo PRIVATE
                        ${UI_VARIANT}/
                        ${UI_VARIANT}/ui/
                        ${UI_VARIANT}/ui/data/
                        ${UI_VARIANT}/ui/utils/
                        ${UI_VARIANT}/ui/utils/ble/
                        ${UI_VARIANT}/ui/utils/clock/
                        ${UI_VARIANT}/ui/utils/file/
                        ${UI_VARIANT}/res/
                        ${UI_VARIANT}/res/color/
                        ${UI_VARIANT}/res/font/
                        ${UI_VARIANT}/res/music/
                        ${UI_VARIANT}/res/image_800x480/
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

target_link_libraries(demo PRIVATE
                        lvgl
                        lvgl_porting
                        wifi
                        http
                        player
                        font
                        usb_hid
                        )

if(SIMULATOR_LINUX)
    set(PROJECT_RES_URL ${PROJECT_BINARY_DIR}/${UI_VARIANT}/res/)
else()
    set(PROJECT_RES_URL /usr/res/)
endif()
target_compile_definitions(demo PUBLIC PROJECT_RES_URL="${PROJECT_RES_URL}")
message("PROJECT_RES_URL is ${PROJECT_RES_URL}")



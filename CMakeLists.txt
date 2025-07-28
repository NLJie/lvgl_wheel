cmake_minimum_required(VERSION 3.15)

project(app VERSION 1.0)

# set(UI_VARIANT KelpBar)
# set(UI_VARIANT WheelPanel)
set(UI_VARIANT P001_LV_Wheel)

# 添加自定义 LV_CONF 路径
set(LV_CONF_PATH_FULL "${CMAKE_SOURCE_DIR}/app/${UI_VARIANT}/lv_conf.h")
add_compile_definitions(LV_CONF_PATH=${LV_CONF_PATH_FULL})

if(SIMULATOR_LINUX)
    message("building linux x86")
    add_subdirectory(platform/x86linux) 
    add_compile_definitions(SIMULATOR_LINUX)
else()
    message("building t113")
    add_subdirectory(platform/t113)
endif()

add_subdirectory(component/osal)
add_subdirectory(component/wifi)
add_subdirectory(component/net)
add_subdirectory(component/player)
add_subdirectory(component/font)
add_subdirectory(component/usb_hid)
add_subdirectory(lvgl)
add_subdirectory(app)
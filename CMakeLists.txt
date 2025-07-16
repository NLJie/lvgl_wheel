cmake_minimum_required(VERSION 3.15)

project(app VERSION 1.0)

# set(UI_VARIANT KelpBar)
set(UI_VARIANT WheelPanel)

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
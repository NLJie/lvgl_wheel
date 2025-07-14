
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(COMPILER_PATH "/home/xiaozhi/t113-v1.1/prebuilt/rootfsbuilt/arm/toolchain-sunxi-glibc-gcc-830")

set(CMAKE_C_COMPILER ${COMPILER_PATH}/toolchain/bin/arm-openwrt-linux-gcc)
set(CMAKE_CXX_COMPILER ${COMPILER_PATH}/toolchain/bin/arm-openwrt-linux-g++)
set(CMAKE_ASM_COMPILER ${COMPILER_PATH}/toolchain/bin/arm-openwrt-linux-gcc)


add_link_options(-L${CMAKE_CURRENT_LIST_DIR}/lib)
add_link_options(-L${CMAKE_SOURCE_DIR}/wifi/libs/)
add_link_options(-L${CMAKE_SOURCE_DIR}/net/libs/)

add_link_options(-lpthread -lfreetype -lrt -ldl -znow -zrelro -luapi -lm -lz -lbz2 -O0 -rdynamic -g -funwind-tables -ffunction-sections)
add_link_options(-fPIC -Wl,-gc-sections)
add_compile_options(-I${CMAKE_CURRENT_LIST_DIR}/src/porting)
add_compile_options(-I${CMAKE_SOURCE_DIR})
add_compile_options(-I${CMAKE_SOURCE_DIR}/lvgl/demos)
add_compile_options(-I${CMAKE_CURRENT_LIST_DIR}/include)
add_compile_options(-I${CMAKE_CURRENT_LIST_DIR}/include/freetype)

add_compile_options(-march=armv7-a -mtune=cortex-a7 -mfpu=neon -mfloat-abi=hard -O0 -ldl -rdynamic -g -funwind-tables -ffunction-sections)
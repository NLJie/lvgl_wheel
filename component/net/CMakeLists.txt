project(http)

aux_source_directory(./src SOURCE)

include_directories(http
    PRIVATE
    inc
    ${CMAKE_SOURCE_DIR}/component/hal
    ${CMAKE_SOURCE_DIR}/component/osal
)

add_library(http STATIC ${SOURCE})

target_link_libraries(http 
                mbedcrypto
                mbedtls
                mbedx509        
                pthread
                cjson
                curl
                ssl
                crypto
                nghttp2
                dl
                lvgl_porting
                osal
                )     
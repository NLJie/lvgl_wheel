project(font)
aux_source_directory(./ SOURCE)
include_directories(font PUBLIC
                        .
                        ${CMAKE_SOURCE_DIR}/app/${UI_VARIANT}
                        )
add_library(font STATIC ${SOURCE})
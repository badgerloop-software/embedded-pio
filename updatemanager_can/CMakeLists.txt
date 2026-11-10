add_library(updatemanager_can STATIC
    updatemanager_can.cpp
    flashimage.cpp
    updcan.cpp
)

target_link_libraries(updatemanager_can PRIVATE zephyr_interface zephyrcan)

add_dependencies(updatemanager_can zephyr_generated_headers)

target_include_directories(updatemanager_can
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

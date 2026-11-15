add_library(coordinator STATIC
    coordinator.cpp
)

target_link_libraries(coordinator PRIVATE zephyr_interface)

add_dependencies(coordinator zephyr_generated_headers)

target_include_directories(coordinator
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

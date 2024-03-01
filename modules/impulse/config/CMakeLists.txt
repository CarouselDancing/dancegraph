    set(SOURCE_FILES
    impulse_signal_config.cpp
)

set(HEADER_FILES
    impulse_signal_config.h
)

add_definitions(
    ${PLATFORM_COMPILE_FLAGS}
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ${COMMON_INCLUDES} )

add_library(impulse_signal_config SHARED ${ALL_SOURCE_FILES})

target_link_libraries(impulse_signal_config PUBLIC ${CONTRIB_LIBRARIES} sig common spdlog)


set(DANCEGRAPH_MODULE_LIBNAME "config")

# The copy_to_appdata line ensures that the target dll will be copied to the target directory (typically %LOCALAPPDATA%/DanceGraph/modules) at build time
# This is where both the native and engine plugins look for dancegraph components

copy_to_appdata(impulse_signal_config)

target_include_directories(
    impulse_signal_config PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    impulse_signal_config PRIVATE -D_EXPORT
)

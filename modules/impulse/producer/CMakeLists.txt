set(SOURCE_FILES
    impulse_signal_producer.cpp
)

set(HEADER_FILES
    impulse_signal_producer.h
)

add_definitions(
    ${PLATFORM_COMPILE_FLAGS}
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ${COMMON_INCLUDES} )

add_library(impulse_signal_producer SHARED ${ALL_SOURCE_FILES})

target_link_libraries(impulse_signal_producer PUBLIC ${CONTRIB_LIBRARIES} sig spdlog)

# The copy_to_appdata line ensures that the target dll will be copied to the target directory (typically %LOCALAPPDATA%/DanceGraph/modules) at build time
# This is where both the native and engine plugins look for dancegraph components
copy_to_appdata(impulse_signal_producer)


set(DANCEGRAPH_MODULE_LIBNAME "producer")


target_include_directories(
    impulse_signal_producer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    impulse_signal_producer PRIVATE -D_EXPORT
)

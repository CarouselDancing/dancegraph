set(SOURCE_FILES
    zed_signal_config.cpp
)

set(HEADER_FILES
    zed_signal_config.h
)

add_definitions(
    ${PLATFORM_COMPILE_FLAGS}
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ${COMMON_INCLUDES} ../ )
 
add_library(zed_signal_config SHARED ${ALL_SOURCE_FILES})

target_link_libraries(zed_signal_config PUBLIC ${CONTRIB_LIBRARIES} sig common spdlog net)

set(DANCEGRAPH_MODULE_LIBNAME "config")

set(OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)
add_custom_command(
    TARGET zed_signal_config POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_CONFIG} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:zed_signal_config> > ${OUTFILE})


target_include_directories(
    zed_signal_config PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    zed_signal_config PRIVATE -D_EXPORT
)

copy_to_appdata(zed_signal_config)
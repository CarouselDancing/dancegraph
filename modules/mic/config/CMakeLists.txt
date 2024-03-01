set(SOURCE_FILES
    mic_signal_config.cpp
)

set(HEADER_FILES
    mic_signal_config.h
)

add_definitions(
    ${PLATFORM_COMPILE_FLAGS}
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)
#source_group("Sources" FILES ${SOURCE_FILES})
#source_group("Headers" FILES ${HEADER_FILES})
#assign_source_group(${ALL_SOURCE_FILES})

include_directories ( ${COMMON_INCLUDES} )

add_library(mic_signal_config SHARED ${ALL_SOURCE_FILES})


set(DANCEGRAPH_MODULE_LIBNAME "config")

set(OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)
add_custom_command(
    TARGET mic_signal_config POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_CONFIG} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:mic_signal_config> > ${OUTFILE})

target_link_libraries(mic_signal_config PUBLIC ${CONTRIB_LIBRARIES} sig common spdlog)

target_include_directories(
    mic_signal_config PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

copy_to_appdata(mic_signal_config)


# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    mic_signal_config PRIVATE -D_EXPORT
)

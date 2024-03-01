set(SOURCE_FILES
    env_config.cpp
)

set(HEADER_FILES
    env_config.h
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

add_library(env_config SHARED ${ALL_SOURCE_FILES})

target_link_libraries(env_config PUBLIC ${CONTRIB_LIBRARIES} sig spdlog)


# These lines are the boilerplate to include that will have the DLLS show up in the config file
set(DANCEGRAPH_MODULE_LIBNAME "config")

set(OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)
add_custom_command(
    TARGET env_config POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_CONFIG} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:env_config> > ${OUTFILE})
# End of config file boilerplate
copy_to_appdata(env_config)

target_include_directories(
    env_config PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    env_config PRIVATE -D_EXPORT
)

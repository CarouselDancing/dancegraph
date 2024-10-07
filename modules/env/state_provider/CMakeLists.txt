
set(SOURCE_FILES
    state_provider.cpp
)

set(HEADER_FILES
    state_provider.h
)

add_definitions(
    ${PLATFORM_COMPILE_FLAGS}
)


set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ${COMMON_INCLUDES} ../ )

add_library(state_provider SHARED ${ALL_SOURCE_FILES})


target_link_libraries (state_provider PUBLIC ${CONTRIB_LIBRARIES} sig common spdlog)

target_include_directories(
    state_provider PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set(DANCEGRAPH_MODULE_LIBNAME "null")

set(DANCEGRAPH_DLL_OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)
add_custom_command(
    TARGET state_provider POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_PRODUCER} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:state_provider> > ${DANCEGRAPH_DLL_OUTFILE})


copy_to_appdata(state_provider)

# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
   producer_null PRIVATE -D_EXPORT
)

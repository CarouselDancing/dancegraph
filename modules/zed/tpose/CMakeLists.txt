
set(SOURCE_FILES
    tpose.cpp
)

set(HEADER_FILES
    tpose.h
)

add_definitions(
    ${PLATFORM_COMPILE_FLAGS}
)


set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ${COMMON_INCLUDES} ../ )

add_library(zed_tpose SHARED ${ALL_SOURCE_FILES})

target_link_libraries(zed_tpose PUBLIC ${CONTRIB_LIBRARIES} sig common zed_common ipc spdlog)

target_include_directories(
    zed_tpose PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

copy_to_appdata(zed_tpose)    

set(DANCEGRAPH_MODULE_LIBNAME "tpose")

set(DANCEGRAPH_DLL_OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)
add_custom_command(
    TARGET zed_tpose POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_PRODUCER} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:zed_tpose> > ${DANCEGRAPH_DLL_OUTFILE})

# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    zed_tpose PRIVATE -D_EXPORT
)

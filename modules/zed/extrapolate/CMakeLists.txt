
set(SOURCE_FILES
    extrapolate.cpp    
)

set(HEADER_FILES
    extrapolate.h
)

add_definitions(
    ${PLATFORM_COMPILE_FLAGS}
)


set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ${COMMON_INCLUDES} ../ )

add_library(zed_extrapolate SHARED ${ALL_SOURCE_FILES})


set(DANCEGRAPH_MODULE_LIBNAME "extrapolate")

set(OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)
add_custom_command(
    TARGET zed_extrapolate POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_TRANSFORMER} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:zed_extrapolate> > ${OUTFILE})

target_link_libraries(zed_extrapolate PUBLIC ${CONTRIB_LIBRARIES} sig common zed_common ${ZED_LIBS} spdlog)

target_include_directories(
    zed_extrapolate PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    zed_extrapolate PRIVATE -D_EXPORT
)
copy_to_appdata(zed_extrapolate)
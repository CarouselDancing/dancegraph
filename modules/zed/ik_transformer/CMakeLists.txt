
set(SOURCE_FILES
    ik_transformer.cpp    
)

set(HEADER_FILES
    ik_transformer.h
)

add_definitions(
    ${PLATFORM_COMPILE_FLAGS}
)


set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ${COMMON_INCLUDES} ../ )

add_library(ik_transformer SHARED ${ALL_SOURCE_FILES})


set(DANCEGRAPH_MODULE_LIBNAME "ik_transformer")

set(OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)

add_custom_command(
    TARGET ik_transformer POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_TRANSFORMER} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:ik_transformer> > ${OUTFILE})

target_link_libraries(ik_transformer PUBLIC ${CONTRIB_LIBRARIES} sig common zed_common ${ZED_LIBS} spdlog)

target_include_directories(
    ik_transformer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    ik_transformer PRIVATE -D_EXPORT
)
copy_to_appdata(ik_transformer)
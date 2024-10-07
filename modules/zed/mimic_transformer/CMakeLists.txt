
set(SOURCE_FILES
    mimic_transformer.cpp    
)

set(HEADER_FILES
    mimic_transformer.h
)

add_definitions(
    ${PLATFORM_COMPILE_FLAGS}
)


set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ${COMMON_INCLUDES} ../ )

add_library(mimic_transformer SHARED ${ALL_SOURCE_FILES})


set(DANCEGRAPH_MODULE_LIBNAME "mimic_transformer")

set(OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)

add_custom_command(
    TARGET mimic_transformer POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_TRANSFORMER} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:mimic_transformer> > ${OUTFILE})

target_link_libraries(mimic_transformer PUBLIC ${CONTRIB_LIBRARIES} sig common zed_common ${ZED_LIBS} spdlog)

target_include_directories(
    mimic_transformer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    mimic_transformer PRIVATE -D_EXPORT
)
copy_to_appdata(mimic_transformer)
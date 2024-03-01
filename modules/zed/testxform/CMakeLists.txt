
set(SOURCE_FILES
    testxform.cpp    
)

set(HEADER_FILES
    testxform.h
)

add_definitions(
    ${PLATFORM_COMPILE_FLAGS}
)


set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ${COMMON_INCLUDES} ../ )

add_library(zed_testxform SHARED ${ALL_SOURCE_FILES})


set(DANCEGRAPH_MODULE_LIBNAME "test_transformer")

set(OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)
add_custom_command(
    TARGET zed_testxform POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_TRANSFORMER} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:zed_testxform> > ${OUTFILE})

target_link_libraries(zed_testxform PUBLIC ${CONTRIB_LIBRARIES} sig common zed_common ${ZED_LIBS} spdlog)

target_include_directories(
    zed_testxform PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    zed_testxform PRIVATE -D_EXPORT
)
copy_to_appdata(zed_testxform)
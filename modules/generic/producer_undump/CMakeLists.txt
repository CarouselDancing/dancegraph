
set(SOURCE_FILES
    undumper.cpp
)

set(HEADER_FILES
    undumper.h
)

add_definitions(
    ${PLATFORM_COMPILE_FLAGS}
)


set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ${COMMON_INCLUDES} ../ )

add_library(undumper SHARED ${ALL_SOURCE_FILES})



target_include_directories(
    undumper PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)


set(DANCEGRAPH_MODULE_LIBNAME "undumper")

set(OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)
add_custom_command(
    TARGET undumper POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_PRODUCER} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:undumper> > ${OUTFILE})

target_link_libraries(undumper PUBLIC ${CONTRIB_LIBRARIES} sig common ipc net spdlog)

copy_to_appdata(undumper)

# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    undumper PRIVATE -D_EXPORT
)

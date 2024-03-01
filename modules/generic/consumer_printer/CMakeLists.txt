set(SOURCE_FILES
    consumer_printer.cpp
)

set(HEADER_FILES
    consumer_printer.h
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

add_library(consumer_printer SHARED ${ALL_SOURCE_FILES})

target_link_libraries(consumer_printer PUBLIC ${CONTRIB_LIBRARIES} sig spdlog)

target_include_directories(
    consumer_printer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set(DANCEGRAPH_MODULE_LIBNAME "printer")
set(OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)
add_custom_command(
    TARGET consumer_printer POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_CONSUMER} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:consumer_printer> > ${OUTFILE})

copy_to_appdata(consumer_printer)

# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    consumer_printer PRIVATE -D_EXPORT
)

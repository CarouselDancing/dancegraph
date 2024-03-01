set(SOURCE_FILES
    consumer_dump2file.cpp
)

set(HEADER_FILES
    consumer_dump2file.h
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

add_library(consumer_dump2file SHARED ${ALL_SOURCE_FILES})

target_link_libraries(consumer_dump2file PUBLIC ${CONTRIB_LIBRARIES} sig common spdlog)

set(DANCEGRAPH_MODULE_LIBNAME "dump2file")
set(OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)
add_custom_command(
    TARGET consumer_dump2file POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_CONSUMER} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:consumer_dump2file> > ${OUTFILE})

copy_to_appdata(consumer_dump2file)

target_include_directories(
    consumer_dump2file PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    consumer_dump2file PRIVATE -D_EXPORT
)

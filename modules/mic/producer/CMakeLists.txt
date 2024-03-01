
set(SOURCE_FILES
    producer.cpp
    Capture.cpp
)



set(HEADER_FILES
    producer.h
    Capture.h
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

add_library(mic_producer SHARED ${ALL_SOURCE_FILES})

target_link_libraries(mic_producer PUBLIC ${CONTRIB_LIBRARIES} sig spdlog)

copy_to_appdata(mic_producer)
set(DANCEGRAPH_MODULE_LIBNAME "producer")

set(OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)
add_custom_command(
    TARGET mic_producer POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_PRODUCER} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:mic_producer> > ${OUTFILE})



target_include_directories(
    mic_producer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

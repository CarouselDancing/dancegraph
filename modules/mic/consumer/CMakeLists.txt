
# TODO: Conditionalize this for Windows/Linux
set(SOURCE_FILES
    consumer.cpp
)



set(HEADER_FILES
    consumer.h
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

add_library(mic_consumer SHARED ${ALL_SOURCE_FILES})

target_link_libraries(mic_consumer PUBLIC ${CONTRIB_LIBRARIES} sig ipc spdlog)

set(DANCEGRAPH_MODULE_LIBNAME "ipc")

set(OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)
add_custom_command(
    TARGET mic_consumer POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_CONSUMER} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:mic_consumer> > ${OUTFILE})

copy_to_appdata(mic_consumer)

target_include_directories(
    mic_consumer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

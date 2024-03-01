
# TODO: Conditionalize this for Windows/Linux
set(SOURCE_FILES
    reader.cpp
)



set(HEADER_FILES
    reader.h
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

add_library(mic_reader SHARED ${ALL_SOURCE_FILES})

target_link_libraries(mic_reader PUBLIC ${CONTRIB_LIBRARIES} ipc spdlog)

copy_to_appdata(mic_reader)

target_include_directories(
    mic_reader PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

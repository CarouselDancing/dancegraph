set(SOURCE_FILES
    producer_ipc.cpp
)

set(HEADER_FILES

)

add_definitions(
    ${PLATFORM_COMPILE_FLAGS}
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ${COMMON_INCLUDES} )

add_library(producer_ipc SHARED ${ALL_SOURCE_FILES})

set(DANCEGRAPH_MODULE_LIBNAME "producer_ipc")

set(OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)
add_custom_command(
    TARGET producer_ipc POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_PRODUCER} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:producer_ipc> > ${OUTFILE})

target_link_libraries(producer_ipc PUBLIC ${CONTRIB_LIBRARIES} sig common ipc spdlog)

target_include_directories(
    producer_ipc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

copy_to_appdata(producer_ipc)

# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    producer_ipc PRIVATE -D_EXPORT
)

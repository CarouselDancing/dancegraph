set(SOURCE_FILES
    lib.cpp
)

set(HEADER_FILES
	lib.h
)

add_definitions(
    ${PLATFORM_COMPILE_FLAGS}
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ${COMMON_INCLUDES} )

add_library(multi-dll-lib SHARED ${ALL_SOURCE_FILES})

set(DANCEGRAPH_MODULE_LIBNAME "multi-dll-lib")

set(OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)
add_custom_command(
    TARGET multi-dll-lib POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_CONSUMER} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:multi-dll-lib> > ${OUTFILE})

target_link_libraries(multi-dll-lib PUBLIC ${CONTRIB_LIBRARIES} sig common ipc spdlog)

target_include_directories(
    multi-dll-lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)


# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    multi-dll-lib PRIVATE -D_EXPORT
)

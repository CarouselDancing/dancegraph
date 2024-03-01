
set(SOURCE_FILES
    test_producer.cpp
)

set(HEADER_FILES
    test_producer.h
)

add_definitions(
    ${PLATFORM_COMPILE_FLAGS}
)


set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ${COMMON_INCLUDES} ../ )

add_library(env_test SHARED ${ALL_SOURCE_FILES})


target_link_libraries(env_test PUBLIC ${CONTRIB_LIBRARIES} sig common spdlog)

target_include_directories(
    env_test PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set(DANCEGRAPH_MODULE_LIBNAME "env")

set(DANCEGRAPH_DLL_OUTFILE ${CMAKE_BINARY_DIR}/bin/share/dlls/${DANCEGRAPH_MODULE}_${DANCEGRAPH_MODULE_LIBNAME}.txt)
add_custom_command(
    TARGET env_test POST_BUILD   
    COMMAND echo ${DANCEGRAPH_MODULE} ${DANCEGRAPH_LIBTYPE_PRODUCER} ${DANCEGRAPH_MODULE_LIBNAME} $<TARGET_FILE:env_test> > ${DANCEGRAPH_DLL_OUTFILE})

copy_to_appdata(env_test)


# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    env_test PRIVATE -D_EXPORT
)

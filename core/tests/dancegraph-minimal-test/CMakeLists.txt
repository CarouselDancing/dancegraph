set(SOURCE_FILES
    dancegraph-minimal-test.cpp
)

set(HEADER_FILES
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

add_executable(dancegraph-minimal-test ${ALL_SOURCE_FILES})
set_property(TARGET dancegraph-minimal-test PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
target_link_libraries(dancegraph-minimal-test PUBLIC ipc common net sig dancegraph-minimal spdlog)


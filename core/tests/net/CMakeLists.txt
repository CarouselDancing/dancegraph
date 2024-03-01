set(SOURCE_FILES
    nettest_lib.cpp
)

set(HEADER_FILES
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

add_executable(nettest ${ALL_SOURCE_FILES})
set_property(TARGET nettest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
target_link_libraries(nettest PUBLIC ipc common net sig spdlog)
copy_dancegraph_json(nettest)

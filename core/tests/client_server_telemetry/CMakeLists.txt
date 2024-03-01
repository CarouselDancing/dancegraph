set(SOURCE_FILES
    client_server_telemetry.cpp
)

set(HEADER_FILES
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

add_executable(client_server_telemetry ${ALL_SOURCE_FILES})
set_property(TARGET client_server_telemetry PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
target_link_libraries(client_server_telemetry PUBLIC ipc common net sig dancegraph-minimal spdlog)


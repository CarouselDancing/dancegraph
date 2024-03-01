set(SOURCE_FILES
    ntp.cpp
)

set(HEADER_FILES
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

add_executable(ntp_test ${ALL_SOURCE_FILES})
set_property(TARGET ntp_test PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
target_link_libraries(ntp_test PUBLIC ntp_client net spdlog)


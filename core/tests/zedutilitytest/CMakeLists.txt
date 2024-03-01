set( SOURCE_FILES
    zed_utilitytest.cpp
)

set(HEADER_FILES
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

add_executable(zed_utilitytest ${ALL_SOURCE_FILES})
#set_property(TARGET zed4_utilitytest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
target_link_libraries(zed_utilitytest PUBLIC zed_common spdlog)
#copy_dancegraph_json(nettest)

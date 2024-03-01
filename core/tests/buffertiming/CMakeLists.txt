set(SOURCE_FILES
    buffertiming.cpp
)

set(HEADER_FILES
    
)

# TODO: based on platform, add windows/linux specific source

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ../common ../ipc)

add_executable(buffertiming ${ALL_SOURCE_FILES})
set_property(TARGET buffertiming PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
target_link_libraries(buffertiming PUBLIC ipc common spdlog)


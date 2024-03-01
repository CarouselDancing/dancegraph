set(SOURCE_FILES
    buffertest.cpp
)

set(HEADER_FILES
    csv.h
)

# TODO: based on platform, add windows/linux specific source

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ../common ../ipc)

add_executable(buffertest ${ALL_SOURCE_FILES})
set_property(TARGET buffertest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
target_link_libraries(buffertest PUBLIC ipc common spdlog)


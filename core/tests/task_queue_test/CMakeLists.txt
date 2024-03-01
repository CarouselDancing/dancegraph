set(SOURCE_FILES
    task_queue_test.cpp
)

set(HEADER_FILES
    
)

# TODO: based on platform, add windows/linux specific source

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ../common)

add_executable(task_queue_test ${ALL_SOURCE_FILES})
set_property(TARGET task_queue_test PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
target_link_libraries(task_queue_test PUBLIC common spdlog)


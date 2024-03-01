set(SOURCE_FILES
    undump.cpp
)

set(HEADER_FILES
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

add_executable(undump ${ALL_SOURCE_FILES})
set_property(TARGET undump PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
target_link_libraries(undump PUBLIC ipc common net sig spdlog)
copy_dancegraph_json(undump)
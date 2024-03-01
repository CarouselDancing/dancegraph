set(SOURCE_FILES
    mdll.cpp
)

set(HEADER_FILES
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

add_executable(multi-dll ${ALL_SOURCE_FILES})
set_property(TARGET multi-dll PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
target_link_libraries(multi-dll PUBLIC ipc common net sig dancegraph-minimal spdlog)


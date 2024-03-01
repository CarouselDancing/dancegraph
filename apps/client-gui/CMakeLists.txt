set(SOURCE_FILES
	main.cpp
)

set(HEADER_FILES
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

add_executable(client-gui ${ALL_SOURCE_FILES})
set_property(TARGET client-gui PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
target_link_libraries(client-gui PUBLIC ipc common sig net imgui net-imgui D3D11.lib spdlog)
copy_dancegraph_json(client-gui)

set(SOURCE_FILES
	listener-main.cpp
    listener-gui.cpp
)

set(HEADER_FILES
	listener-gui.h
	imgui_util.h
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

add_executable(listener-gui ${ALL_SOURCE_FILES})
set_property(TARGET listener-gui PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
target_link_libraries(listener-gui PUBLIC ipc common net sig imgui D3D11.lib)



# Working around cmakes limitations; this just copies the resources files to the appdata directory

add_custom_target(resources_dummy ALL
	DEPENDS 
		copy_config_to_appdata
		copy_runtime_config_to_appdata
)

add_custom_command(
	OUTPUT
	copy_config_to_appdata
    COMMAND ${CMAKE_COMMAND} -E copy  
        "${PROJECT_SOURCE_DIR}/resources/dancegraph.json"
		"$ENV{LOCALAPPDATA}\\DanceGraph\\dancegraph.json"
)

add_custom_command(
	OUTPUT
	copy_runtime_config_to_appdata
    COMMAND ${CMAKE_COMMAND} -E copy  
        "${PROJECT_SOURCE_DIR}/resources/dancegraph_rt_presets.json"
		"$ENV{LOCALAPPDATA}/DanceGraph/dancegraph_rt_presets.json"

)

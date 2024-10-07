set(SOURCE_FILES
	env_adapter.cpp
)


set(HEADER_FILES
	env_adapter.h
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ${COMMON_INCLUDES} ../../signal_libraries/env ../../common)

add_library(env_adapter STATIC ${ALL_SOURCE_FILES})

if(WIN32)
target_link_libraries(env_adapter PUBLIC ipc common "shlwapi.lib")
else()
target_link_libraries(env_adapter PUBLIC ipc common )
endif()
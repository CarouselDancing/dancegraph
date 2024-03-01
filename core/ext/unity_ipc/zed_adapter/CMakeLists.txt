set(SOURCE_FILES
	zed_adapter.cpp
)


set(HEADER_FILES
	zed_adapter.h
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ${COMMON_INCLUDES} ../../signal_libraries/zed ../../common)

add_library(zed_adapter STATIC ${ALL_SOURCE_FILES})

if(WIN32)
target_link_libraries(zed_adapter PUBLIC ipc common zed_common "shlwapi.lib")
else()
target_link_libraries(zed_adapter PUBLIC ipc common zed_common)
endif()
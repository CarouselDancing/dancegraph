set(SOURCE_FILES
	latency_adapter.cpp
)


set(HEADER_FILES
	latency_adapter.h
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ${COMMON_INCLUDES} ../../signal_libaries/zed ../../common)

add_library(zed_adapter STATIC ${ALL_SOURCE_FILES})

if(WIN32)
target_link_libraries(latency_adapter PUBLIC ipc common "shlwapi.lib")
else()
target_link_libraries(latency_adapter PUBLIC ipc common)
endif()
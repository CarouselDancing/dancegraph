
set(DANCEGRAPH_MODULE "zed")

set(ZEDCOMMON_SOURCE_FILES 
	zed_common.cpp
)
set(ZEDCOMMON_INCLUDE_FILES
	zed_common.h
)


add_subdirectory(config)
add_subdirectory(extrapolate)
add_subdirectory(testxform)
add_subdirectory(tpose)

include_directories(../../common)

if (${USE_ZED_SDK})
	message("USE_ZED_SDK is ON")
else()
	message("USE_ZED_SDK is OFF")
endif()


# zed4_producer has ZED4 SDK dependencies, so it's hived off to a separate repository
if (${USE_ZED_SDK})
	message("Checking for zed4_producer cmake in ${CMAKE_CURRENT_SOURCE_DIR}/producer/CMakeLists.txt")
	if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/producer/CMakeLists.txt")
		message("Producer found. Building with ZED4 Camera Support")
		add_subdirectory(producer)
	else()
		message("ZED4 Producer Not found. Make sure you've downloaded the submodule")
	endif()
endif()


add_library(zed_common STATIC ${ZEDCOMMON_SOURCE_FILES})
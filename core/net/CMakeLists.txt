# TODO: Conditionalize this for Windows/Linux
set(SOURCE_FILES
	net.cpp
    channel.cpp
    client.cpp
    config.cpp
	config_master.cpp
	listener.cpp
    server.cpp
    message.cpp
    state.cpp
	sync.cpp
	stats.cpp
)



set(HEADER_FILES
    net.h
    client.h
	listener.h
    server.h
    message.h
    state.h
	formatter.h
	channel.h	
    config.h
	config_runtime.h
	config_master.h
	sync.h
	stats.h
	telemetry.h
)

# # TODO: based on platform, add windows/linux specific source

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

add_library(net STATIC ${ALL_SOURCE_FILES})

find_package(WindowsSDK REQUIRED)
if (WIN32)
	if (WINDOWSSDK_LATEST_DIR AND CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
		set(WS2_32_LIBRARY_PATH "${WINDOWSSDK_LATEST_DIR}/Lib/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/um/x64/WS2_32.lib")
	else()
		find_library(WS2_32_LIBRARY_PATH WS2_32)
	endif()
endif()
target_link_libraries(net sig ntp_client spdlog ${WS2_32_LIBRARY_PATH})


#configure_file(moduleopts_net.json ${CMAKE_BINARY_DIR}/bin/share/modules/moduleopts_net.json COPYONLY)
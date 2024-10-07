set(SOURCE_FILES
	utility.cpp
	platform.cpp
	task_queue.cpp
)

set(HEADER_FILES
	platform.h
	utility.h
	task_queue.h
)


set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

add_library(common STATIC ${ALL_SOURCE_FILES})

configure_file(dlls.json ${CMAKE_BINARY_DIR}/bin/share/dlls.json)

if (WIN32)
target_link_libraries(common "shlwapi.lib")
endif()

target_include_directories(
    common PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)


set(SOURCE_FILES
    configtest.cpp
)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(HEADER_FILES    
)

# TODO: based on platform, add windows/linux specific source

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories ( ../../common)

add_executable(configtest ${ALL_SOURCE_FILES})

target_link_libraries(configtest PUBLIC common net spdlog)
set_property(TARGET configtest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

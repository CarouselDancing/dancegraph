set(SOURCE_FILES
    pubsubtest.cpp
)

set(HEADER_FILES    
)

# TODO: based on platform, add windows/linux specific source

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

include_directories (../pubsub)

add_executable(pubsubtest ${ALL_SOURCE_FILES})

#if (WIN32)
#target_link_libraries(zedtest PUBLIC ipc common "shlwapi.lib")
#else()
target_link_libraries(pubsubtest PUBLIC pubsub spdlog)
#endif()

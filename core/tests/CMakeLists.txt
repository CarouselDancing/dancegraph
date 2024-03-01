
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(buffer)
add_subdirectory(buffertiming)
add_subdirectory(net)
add_subdirectory(ntp)

add_subdirectory(config)
#add_subdirectory(undump)
add_subdirectory(pubsub)
add_subdirectory(dancegraph-minimal-test)
add_subdirectory(task_queue_test)
#add_subdirectory(multi-dll)

#add_subdirectory(zedutilitytest)
#add_subdirectory(multi-dll-lib)

add_subdirectory(client_server_telemetry)


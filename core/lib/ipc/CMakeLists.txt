
# TODO: Conditionalize this for Windows/Linux
set(SOURCE_FILES
    ipc.cpp
    ringbuffer.cpp
    ipc_linux.cpp
    ipc_windows.cpp
    ipc_windows.cpp
)



set(HEADER_FILES
    ipc.h
    ringbuffer.h
    ipc_linux.h
    ipc_windows.h
)

# # TODO: based on platform, add windows/linux specific source

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

add_library(ipc STATIC ${ALL_SOURCE_FILES})



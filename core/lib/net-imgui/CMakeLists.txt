
# TODO: Conditionalize this for Windows/Linux
set(SOURCE_FILES
	client.cpp
	listener.cpp
    server.cpp
	common.cpp
)



set(HEADER_FILES
    client.h
	listener.h
    server.h
	common.h
	log.h
)

# # TODO: based on platform, add windows/linux specific source

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

add_library(net-imgui STATIC ${ALL_SOURCE_FILES})

set(SOURCE_FILES
	pubsub.cpp
)

set(HEADER_FILES
    pubsub.h
)

# # TODO: based on platform, add windows/linux specific source

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

add_library(pubsub STATIC ${ALL_SOURCE_FILES})


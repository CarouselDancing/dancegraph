set(SOURCE_FILES
	signal_common.cpp
	signal_producer.cpp
	signal_consumer.cpp
    signal_config.cpp
	signal_transformer.cpp
	signal_history.cpp
)

set(HEADER_FILES
    signal_producer.h
	signal_consumer.h
	signal_transformer.h
    signal_config.h
	signal_common.h	
	signal_history.h
	sig_json.h
)

# # TODO: based on platform, add windows/linux specific source

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

add_library(sig STATIC ${ALL_SOURCE_FILES})


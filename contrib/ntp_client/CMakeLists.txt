
# TODO: Conditionalize this for Windows/Linux
set(SOURCE_FILES
    ntp.cpp    
)



set(HEADER_FILES
    ntp.h
)

# # TODO: based on platform, add windows/linux specific source

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

add_library(ntp_client STATIC ${ALL_SOURCE_FILES})



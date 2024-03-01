set(SOURCE_FILES
    dancegraph.cpp
)

set(HEADER_FILES
    dancegraph.h
)

add_definitions(
    ${PLATFORM_COMPILE_FLAGS}
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)
#source_group("Sources" FILES ${SOURCE_FILES})
#source_group("Headers" FILES ${HEADER_FILES})
#assign_source_group(${ALL_SOURCE_FILES})

include_directories ( ${COMMON_INCLUDES} ../net ../dancegraph/env_adapter)

add_library(dancegraph-minimal SHARED ${ALL_SOURCE_FILES})

copy_to_appdata(dancegraph-minimal)

if (WIN32)
target_link_libraries(dancegraph-minimal PUBLIC ${CONTRIB_LIBRARIES} ipc net common env_adapter)
else()
target_link_libraries(dancegraph-minimal PUBLIC ${CONTRIB_LIBRARIES} ipc net common env_adapter "shlwapi.lib")
endif()

target_include_directories(
    dancegraph-minimal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    dancegraph-minimal PRIVATE -D_EXPORT
)


set(SOURCE_FILES
    zed_torch.cpp
)

set(HEADER_FILES
    zed_torch.h
)

add_definitions(
    ${PLATFORM_COMPILE_FLAGS}
)


set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

set(CMAKE_PREFIX_PATH "C:/Users/Carousel/Documents/Carousel/libtorch/")
#set(CMAKE_PREFIX_PATH "D:/Carousel/libtorch/")

find_package(Torch REQUIRED)

include_directories ( ${COMMON_INCLUDES} ../ )
#include_directories ( ${COMMON_INCLUDES} ../ ${ZED_INCLUDE_DIRS} ${CUDA_INCLUDE_DIRS}  )
#find_package(CUDA ${ZED_CUDA_VERSION} REQUIRED)
#link_directories(${CUDA_LIBRARY_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

add_library(zed_torch SHARED ${ALL_SOURCE_FILES})

set(DANCEGRAPH_MODULE_LIBNAME "zed_torch")

target_link_libraries(zed_torch PUBLIC ${CONTRIB_LIBRARIES} sig common zed_common net ${ZED_LIBS} spdlog ${TORCH_LIBRARIES})

target_include_directories(
    zed_torch PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# sets the _EXPORT #define for the target (library)
# to ensure the functions are exported (visible)
target_compile_definitions(
    zed_torch PRIVATE -D_EXPORT
)


# This is the official workaround for https://github.com/pytorch/pytorch/issues/25457
if (MSVC)
  file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
  add_custom_command(TARGET zed_torch
                     POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                     ${TORCH_DLLS}
                     $ENV{LOCALAPPDATA}/DanceGraph/modules)
endif (MSVC)

copy_to_appdata(zed_torch)




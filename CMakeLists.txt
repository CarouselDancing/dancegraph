
cmake_minimum_required(VERSION 3.17)
project(dancegraph)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CUDA_STANDARD 17)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# This makes gcc link ipc_signal_consumer properly. If Visual Studio Complains, make it OS-conditional
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set (CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/pdb)
# Second, for multi-config builds (e.g. msvc)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )
    set( CMAKE_PDB_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/share)

option(USE_ZED_SDK "Use ZED SDK dependencies" ON)
option(BUILD_UNITY_PLUGIN "Build plugins for the Unity Engine" ON)
option(USE_LIBTORCH "Use Libtorch transformer" ON)



if(UNIX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
elseif(WIN32)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32_LEAN_AND_MEAN -DNOMINMAX")
endif(UNIX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSPDLOG_COMPILED_LIB")

# Create "resources" folder in /bin
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/resources)
# Call this from the various executables so that we always have dancegraph.json in bin
function(copy_dancegraph_json tgt)
add_custom_command(TARGET ${tgt} POST_BUILD        # Adds a post-build event to MyTest
    COMMAND ${CMAKE_COMMAND} -E copy  
        "${PROJECT_SOURCE_DIR}/resources/dancegraph.json"
        "${CMAKE_BINARY_DIR}/bin/resources/dancegraph.json")
endfunction()

# Horrible hack to get cmake to copy the dll and resource files in %LOCALAPPDATA/DanceGraph/modules after being built.
# Call copy_to_appdata(targetname) in every CMakeLists.txt that builds a dll.
# Needed because cmake can't easily be triggered to call a single copy script after everything is built
function(copy_to_appdata tgt)
add_custom_command(TARGET ${tgt} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:${tgt}>    
    $ENV{LOCALAPPDATA}/DanceGraph/modules/$<TARGET_FILE_NAME:${tgt}>
)
add_custom_command(TARGET ${tgt} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_PDB_FILE:${tgt}>    
    $ENV{LOCALAPPDATA}/DanceGraph/modules/$<TARGET_PDB_FILE_NAME:${tgt}>
)
endfunction()

set (DANCEGRAPH_LIBTYPE_TRANSFORMER "transformer")
set (DANCEGRAPH_LIBTYPE_PRODUCER "producer")
set (DANCEGRAPH_LIBTYPE_CONSUMER "consumer")
set (DANCEGRAPH_LIBTYPE_CONFIG "config")

file(REMOVE ${CMAKE_BINARY_DIR}/bin/share/dll_list.txt)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/share/dlls)


include_directories(. contrib core/lib contrib/spdlog/include contrib/BS_thread_pool/include)

# Build other external libraries

add_subdirectory(modules)
add_subdirectory(core)
add_subdirectory(apps)
add_subdirectory(contrib)

add_subdirectory(resources)

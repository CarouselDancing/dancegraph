set(SOURCE_FILES
    imgui_draw.cpp
	imgui_tables.cpp
	imgui_widgets.cpp
	imgui.cpp
	imgui_demo.cpp
	imgui_impl_win32.cpp
	imgui_impl_dx11.cpp
	imgui_stdlib.cpp
)

set(HEADER_FILES
    imgui.h
	imgui_internal.h
	imstb_rectpack.h
	imstb_textedit.h
	imstb_truetype.h
	imgui_impl_win32.h
	imgui_impl_dx11.h
	imconfig.h
	imgui_stdlib.h
)

set( ALL_SOURCE_FILES
     ${SOURCE_FILES}
     ${HEADER_FILES}
)

#assign_source_group(${ALL_SOURCE_FILES})

add_library(imgui STATIC ${ALL_SOURCE_FILES})

target_include_directories(
    imgui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

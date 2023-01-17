cmake_minimum_required(VERSION 3.13)

set(shader_source_files "")

#
# Function for compiling SM5 HLSL shaders with fxc, and then including them as C++ byte arrays.
#
function(fxc shader_name entrypoint profile dependencies)
	list(TRANSFORM dependencies PREPEND "shaders/")

	# Compile .hlsl source into .bin bytecode
	add_custom_command(
		OUTPUT "shaders/${shader_name}.bin"
		BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader_name}.pdb"
		MAIN_DEPENDENCY "shaders/${shader_name}.hlsl"
		COMMENT "Compiling HLSL shader ${shader_name}"
		DEPENDS "${dependencies}"

		COMMAND "fxc" "/E${entrypoint}" "/WX" "/T${profile}" "/Fo${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader_name}.bin" $<$<CONFIG:Debug>:"/Zi"> $<$<CONFIG:DEBUG>:"/Fd${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader_name}.pdb"> "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader_name}.hlsl"
	)

	# Embed .bin bytecode into .h and .cpp files
	add_custom_command(
		OUTPUT "shaders/${shader_name}.cpp"
		BYPRODUCTS "shaders/${shader_name}.h"
		MAIN_DEPENDENCY "shaders/${shader_name}.bin"
		COMMENT "Embedding HLSL shader ${shader_name}"

		COMMAND ${CMAKE_COMMAND} -DSHADER_NAME=${shader_name} -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/shaders -P ${CMAKE_SOURCE_DIR}/Starlight/cmake/ShaderGenerate.cmake
	)

	set(shader_source_files "shaders/${shader_name}.cpp;${shader_source_files}" PARENT_SCOPE)
endfunction()

set(resource_source_files "")

#
# Function for embedding arbitrary files as C++ byte arrays.
#
function(embed_resource resource_name)
	string(REGEX REPLACE "\\.[^.]*$" "" resource_name_no_extension ${resource_name})
	add_custom_command(
		OUTPUT "res/${resource_name_no_extension}.cpp"
		BYPRODUCTS "res/${resource_name_no_extension}.h"
		MAIN_DEPENDENCY "res/${resource_name}"
		COMMENT "Embedding resource ${resource_name}"

		COMMAND ${CMAKE_COMMAND} -DINPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/res" -DINPUT_NAME=${resource_name} -DOUTPUT_NAMESPACE="Resources" -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/res -P ${CMAKE_SOURCE_DIR}/Starlight/cmake/FileEmbed.cmake
	)

	set(resource_source_files "res/${resource_name_no_extension}.cpp;${resource_source_files}" PARENT_SCOPE)
endfunction()

fxc("WidgetVertexShader" "VSMain" "vs_5_0" "")
fxc("WidgetPixelShader" "PSMain" "ps_5_0" "")

embed_resource("ic_tool_move.png")
embed_resource("ic_tool_rotate.png")
embed_resource("ic_tool_select.png")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders")
string(STRIP "${shader_source_files}" shader_source_files)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/res")
string(STRIP "${resource_source_files}" resource_source_files)

add_executable(StarlightShowcase WIN32 "src/Main.cpp" "src/Window.cpp" "src/render/DeferredPipeline.cpp"
	"src/imgui/imgui.cpp" "src/imgui/imgui_demo.cpp" "src/imgui/imgui_draw.cpp" "src/imgui/imgui_tables.cpp" "src/imgui/imgui_widgets.cpp" "src/imgui/imgui_custom.cpp"
	"src/imgui/backend/imgui_impl_win32.cpp" "src/imgui/backend/imgui_impl_dx11.cpp" "src/render/Mesh.cpp" "src/Widget.cpp"
	"${shader_source_files}" "${resource_source_files}" "src/render/Technique.cpp" "src/Camera.cpp" "src/CameraController.cpp" "src/scene/SceneObject.cpp" "src/Showcase.cpp" "src/scene/Light.cpp" "src/scene/PointLight.cpp" "src/stb_image.c" "src/ShowcaseResources.cpp")
target_include_directories(StarlightShowcase PUBLIC "../Starlight/include/" "./include/" "${CMAKE_CURRENT_BINARY_DIR}/shaders" "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(StarlightShowcase PUBLIC starlight "d3d11" "dxgi")

set(all_shaders "")

#
# Function for compiling SM 6.0+ HLSL shaders with dxc
#
function(dxc shader_name entrypoint profile)
	# Compile .hlsl source into .bin bytecode
	add_custom_command(
		#TARGET shaders
		OUTPUT "shaders/${shader_name}.bin"
		BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader_name}.pdb"
		MAIN_DEPENDENCY "shaders/${shader_name}.hlsl"
		COMMENT "Building HLSL shader ${shader_name}"

		COMMAND "dxc" "-E ${entrypoint}" "-WX" "-T ${profile}" "-Fo ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader_name}.bin" $<$<CONFIG:Debug>:"-Zi"> $<$<CONFIG:DEBUG>:"-Fd ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader_name}.pdb"> "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader_name}.hlsl"
	)

	# Embed .bin bytecode into .h and .cpp files
	add_custom_command(
		OUTPUT "shaders/${shader_name}.cpp"
		BYPRODUCTS "shaders/${shader_name}.h"
		MAIN_DEPENDENCY "shaders/${shader_name}.bin"
		COMMENT "Embedding HLSL shader ${shader_name}"

		COMMAND ${CMAKE_COMMAND} -DSHADER_NAME=${shader_name} -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/shaders -P ${CMAKE_SOURCE_DIR}/Starlight/cmake/ShaderGenerate.cmake
	)

	#set(all_shaders "shaders/${shader_name}.bin ${all_shaders}" PARENT_SCOPE)
	set(all_shaders "shaders/${shader_name}.cpp ${all_shaders}" PARENT_SCOPE)
endfunction()

# Shaders
dxc("UVVertexShader" "VSMain" "vs_6_0")

#if (NOT "${all_shaders}" STREQUAL "")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders")
	string(STRIP "${all_shaders}" all_shaders)
#	add_custom_target(shaders ALL DEPENDS ${all_shaders})
#	add_dependencies(starlight shaders)
#	add_dependencies(cstarlight shaders)
#endif()

add_library(starlight STATIC "src/Starlight.cpp" "${all_shaders}")
target_include_directories(starlight PUBLIC "include")
target_include_directories(starlight PUBLIC "../DirectXMath/Inc")
target_include_directories(starlight PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/shaders")
target_link_libraries(starlight PUBLIC "d3d12")

add_library(cstarlight SHARED "src/Starlight.cpp" "${all_shaders}" "src/cstarlight.cpp")
target_include_directories(cstarlight PUBLIC "include")
target_include_directories(cstarlight PRIVATE "../DirectXMath/Inc")
target_include_directories(cstarlight PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/shaders")
target_link_libraries(cstarlight PUBLIC "d3d12")
target_compile_definitions(cstarlight PRIVATE "STARLIGHT_DLL")

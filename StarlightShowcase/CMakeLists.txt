cmake_minimum_required(VERSION 3.13)

add_executable(StarlightShowcase WIN32 "src/Main.cpp" "src/Window.cpp" "src/DeferredPipeline.cpp"
	"src/imgui/imgui.cpp" "src/imgui/imgui_demo.cpp" "src/imgui/imgui_draw.cpp" "src/imgui/imgui_tables.cpp" "src/imgui/imgui_widgets.cpp"
	"src/imgui/backend/imgui_impl_win32.cpp" "src/imgui/backend/imgui_impl_dx11.cpp")
target_include_directories(StarlightShowcase PUBLIC "../Starlight/include/" "./include/")
target_link_libraries(StarlightShowcase PUBLIC starlight "d3d11" "dxgi")
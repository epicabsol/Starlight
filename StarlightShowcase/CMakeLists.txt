cmake_minimum_required(VERSION 3.13)

add_executable(StarlightShowcase WIN32 "src/Main.cpp" "src/Window.cpp" "src/DeferredPipeline.cpp")
target_include_directories(StarlightShowcase PUBLIC "../Starlight/include/" "./include/")
target_link_libraries(StarlightShowcase PUBLIC starlight "d3d11" "dxgi")
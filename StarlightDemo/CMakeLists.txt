add_executable(StarlightDemo WIN32 src/Main.cpp)
target_include_directories(StarlightDemo PUBLIC ../Starlight/include/)
target_link_libraries(StarlightDemo PUBLIC starlight)


add_executable(StarlightDemo WIN32 "src/Main.cpp" "src/stb_image.c" "src/stb_image_write.c" "src/tiny_obj_loader.cpp")
target_include_directories(StarlightDemo PUBLIC "../Starlight/include/" "./include/")
target_link_libraries(StarlightDemo PUBLIC starlight)


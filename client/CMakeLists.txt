add_executable(nr_client main.c)
target_compile_definitions(nr_client PRIVATE CLIENT_VERSION="1.0.0-beta")
target_link_libraries(nr_client PRIVATE needy)
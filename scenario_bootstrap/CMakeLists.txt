add_executable(nr_bootstrap main.c)
target_link_libraries(nr_bootstrap PRIVATE needy)
target_compile_definitions(nr_bootstrap PRIVATE BOOTSTRAP_VERSION="1.0.0-beta")
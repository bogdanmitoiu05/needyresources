cmake_minimum_required(VERSION 3.10)

project(nr_bootstrap C)

add_executable(nr_bootstrap main.c)


target_link_libraries(nr_bootstrap
    jansson
    needy
)
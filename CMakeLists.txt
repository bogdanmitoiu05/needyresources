cmake_minimum_required(VERSION 4.2)
project(NeedyResources C)

set(CMAKE_C_STANDARD 23)

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(scenario_bootstrap)

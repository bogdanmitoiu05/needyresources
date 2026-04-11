cmake_minimum_required(VERSION 4.2)
project(NeedyResources C)

set(CMAKE_C_STANDARD 23)

add_subdirectory(libneedy)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(scenario_bootstrap)

install(TARGETS
        needy
        nr_server
        nr_client
        nr_bootstrap
        RUNTIME           # Following options apply to runtime artifacts.
            COMPONENT Runtime
        LIBRARY           # Following options apply to library artifacts.
            COMPONENT Runtime
            NAMELINK_COMPONENT Development
        ARCHIVE           # Following options apply to archive artifacts.
            COMPONENT Development
            DESTINATION lib/static
        FILE_SET HEADERS  # Following options apply to file set HEADERS.
            COMPONENT Development
)
install(FILES )

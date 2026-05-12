add_executable(nr_server main.c
        server_config.c
        server_config.h
        mq_manager.c
        mq_manager.h)


target_link_libraries(nr_server PRIVATE needy)
target_link_libraries(nr_server PRIVATE pthread)

target_compile_definitions(nr_server PRIVATE SERVER_VERSION="1.0.0-beta")
if (DEFINED ${MOCK_HOME})
    install(FILES systemd/NeedyResourcesServer.service DESTINATION
            .config/systemd/user.control/*)
else ()
    install(FILES systemd/NeedyResourcesServer.service DESTINATION
            $ENV{HOME}/.config/systemd/user.control/*)
endif ()

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/config.json)
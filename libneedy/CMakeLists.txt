add_library(needy client_identification.c
        resource_request.c
        resource_request.h
        message.c
        message.h
        message_type.h
        server_ack.c
        server_ack.h
        client_finalize.c
        client_finalize.h
        constants.h
        message_type.c
        message_queue.c
        message_queue.h
        file_request_response.c
        file_request_response.h
        read_file_request.c
        read_file_request.h
        write_file_request.c
        write_file_request.h
        resource_response.c
        resource_response.h)
target_sources(needy PUBLIC FILE_SET HEADERS TYPE HEADERS FILES needy.h client_identification.h resource_request.h message_type.h message.h response_codes.h BASE_DIRS .)
target_compile_options(needy PUBLIC -std=c11)
target_include_directories(needy PRIVATE private)
target_compile_definitions(needy PUBLIC _POSIX_C_SOURCE=200809L)
target_compile_options(needy PUBLIC -Wall -Wextra -Wpedantic -Werror -Wno-newline-eof)
if (${CMAKE_BUILD_TYPE} EQUAL "Debug")
    target_compile_options(needy PUBLIC -g -fsanitize=thread)
    target_link_options(needy PUBLIC -fsanitize=thread)
else ()
    target_compile_options(needy PUBLIC -O2)
endif ()
target_link_libraries(needy PUBLIC jansson)

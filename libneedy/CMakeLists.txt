add_library(needy client_identification.c
        resource_request.c
        resource_request.h
        message.c
        message.h
        message_type.h
        server_ack.cpp
        server_ack.h
        client_finalize.cpp
        client_finalize.h)
target_sources(needy PUBLIC FILE_SET HEADERS TYPE HEADERS FILES needy.h client_identification.h BASE_DIRS .)
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
target_link_libraries(needy PRIVATE jansson)

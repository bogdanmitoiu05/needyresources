add_library(needy client_identification.c
        resource_request.c
        resource_request.h
        message_frame.c
        message_frame.h
        message_frame_type.h)
target_sources(needy PUBLIC FILE_SET HEADERS TYPE HEADERS FILES needy.h client_identification.h BASE_DIRS .)
target_include_directories(needy PRIVATE private)
target_compile_definitions(needy PUBLIC POSIX_C_SOURCE=200809L)
target_compile_options(needy PUBLIC -Wall -Wextra -Wpedantic -Werror -Wno-newline-eof)
if (${CMAKE_BUILD_TYPE} EQUAL "Debug")
    target_compile_options(needy PUBLIC -g -fsanitize=thread)
    target_link_options(needy PUBLIC -fsanitize=thread)
else ()
    target_compile_options(needy PUBLIC -O2)
endif ()
target_link_libraries(needy PRIVATE Jansson)

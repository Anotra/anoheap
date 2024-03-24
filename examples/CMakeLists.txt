project(timers)
add_executable(${PROJECT_NAME} timers/main.c timers/timers.c)
target_compile_definitions(${PROJECT_NAME} PRIVATE _GNU_SOURCE)
target_link_libraries(${PROJECT_NAME} PRIVATE anoheap)
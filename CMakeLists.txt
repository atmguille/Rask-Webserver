project(practica1)
set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES src/main.c src/daemonize.c)

add_executable(practica1 ${SOURCE_FILES})
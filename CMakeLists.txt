cmake_minimum_required(VERSION 3.11)
project(wizard_language)

find_package(raylib REQUIRED)

add_executable(hi
                main.c
                chunk.c
                compiler.c
                debug.c
                memory.c
                scanner.c
                value.c
                vm.c
                object.c
                visualizer.c
                table.c
)

target_link_libraries(hi raylib m)


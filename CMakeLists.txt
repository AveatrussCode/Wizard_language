cmake_minimum_required(VERSION 3.11)
project(wizard_language)

add_executable(hi
                main.c
                chunk.c
                compiler.c
                debug.c
                memory.c
                scanner.c
                value.c
                vm.c
)
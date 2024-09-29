@echo off

rem download mingw here: https://github.com/skeeto/w64devkit

gcc demo.c -O3 -s -std=c89 -pedantic -Wall -Wextra -Werror -lgdi32 -lwinmm -o demo.exe

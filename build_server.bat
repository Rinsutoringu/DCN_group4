@echo off
set COMPILER=D:\environment\winlibs-x86_64-posix-seh-gcc-14.2.0-mingw-w64msvcrt-12.0.0-r3\mingw64\bin\g++.exe
set SRC_DIR=%~dp0src\server
set OUTPUT=%~dp0bin\server.exe
%COMPILER% -g -O0 -I %~dp0inc %SRC_DIR%\*.cpp -o %OUTPUT% -lws2_32
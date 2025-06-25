# Minimalna wymagana wersja CMake
cmake_minimum_required(VERSION 3.10)

# Nazwa projektu
project(cpp_chess)

# Ustaw standard C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Dodaj katalog z plikami nagłówkowymi
include_directories(include)

# Dodaj pliki źródłowe
file(GLOB SOURCES "src/*.cpp")

# Tworzenie pliku wykonywalnego
add_executable(cpp_chess ${SOURCES})
# Minimalna wymagana wersja CMake
cmake_minimum_required(VERSION 3.10)

# Nazwa projektu
project(cpp_chess)

# Ustaw standard C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Domyślny typ budowania
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Domyślny typ budowania" FORCE)
endif()

# Pliki źródłowe
file( GLOB SOURCES "src/*.cpp")
# Tworzenie pliku wykonywalnego
add_executable(cpp_chess ${SOURCES})

# Dodaj katalog z plikami nagłówkowymi
target_include_directories(cpp_chess PRIVATE include)

# Ustawienia kompilatora dla różnych konfiguracji
target_compile_options(cpp_chess PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3>
)

# Testy
enable_testing()
find_package(GTest REQUIRED)

# Tworzenie pliku wykonywalnego dla testów
add_executable(tests test/test_main.cpp)

# Dodaj katalogi nagłówkowe i biblioteki dla testów
target_include_directories(tests PRIVATE include ${GTEST_INCLUDE_DIRS})
target_link_libraries(tests PRIVATE ${GTEST_LIBRARIES} pthread)

# Rejestracja testów
add_test(NAME cpp_chess_tests COMMAND tests)
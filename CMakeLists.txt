# Minimalna wymagana wersja CMake
cmake_minimum_required(VERSION 3.10)

# Nazwa projektu
project(cpp_chess VERSION 1.0 LANGUAGES CXX)

# Ustaw standard C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# statyczne sprawdzanie kodu
set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")

# Domyślny typ budowania
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Domyślny typ budowania" FORCE)
endif()


# Ustawienia kompilatora dla różnych konfiguracji
function(set_my_compile_options target)
    target_compile_options(${target} PRIVATE
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3>
    )
endfunction()

# Build cli
file(GLOB CLI_SOURCES "cli/*.cpp")
add_executable(cpp_chess_cli ${CLI_SOURCES})
set_my_compile_options(cpp_chess_cli)
target_include_directories(cpp_chess_cli PRIVATE include)
find_package(fmt REQUIRED)
target_link_libraries(cpp_chess_cli PRIVATE fmt::fmt)

# Testy
option(BUILD_TESTING "Build tests" ON)

if(BUILD_TESTING)
    enable_testing()
    find_package(GTest REQUIRED)

    # Tworzenie pliku wykonywalnego dla testów
    add_executable(tests test/test_main.cpp)

    # Dodaj katalogi nagłówkowe i biblioteki dla testów
    target_link_libraries(tests PRIVATE GTest::gtest GTest::gtest_main pthread)

    # Rejestracja testów
    add_test(NAME cpp_chess_tests COMMAND tests)
endif()

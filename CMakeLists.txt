cmake_minimum_required(VERSION 3.20)
project(AcademiaRealColoresFormas LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Esta actividad usa la interfaz gráfica nativa de Windows.")
endif()

add_executable(AcademiaReal WIN32 main.cpp)
target_compile_features(AcademiaReal PRIVATE cxx_std_17)
target_link_libraries(AcademiaReal PRIVATE gdi32 user32)
if(MSVC)
    target_compile_options(AcademiaReal PRIVATE /utf-8)
endif()

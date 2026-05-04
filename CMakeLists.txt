cmake_minimum_required(VERSION 3.10)
project(MRussiaPlugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Создаём shared library (плагин)
add_library(MRussiaPlugin SHARED src/main.cpp)

# Подключаем пути к заголовкам SDK
target_include_directories(MRussiaPlugin PRIVATE ${CMAKE_SOURCE_DIR}/samp-sdk)

# Настройка имени файла
if(WIN32)
    set_target_properties(MRussiaPlugin PROPERTIES 
        PREFIX "" 
        SUFFIX ".dll"
        OUTPUT_NAME "MRussiaPlugin")
else()
    set_target_properties(MRussiaPlugin PROPERTIES 
        PREFIX "" 
        SUFFIX ".so"
        OUTPUT_NAME "MRussiaPlugin")
endif()

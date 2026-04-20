cmake_minimum_required(VERSION 3.16)

project(Checkers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(appcheckers
    main.cpp
    checkergame.cpp
    checkergame.h
)

target_link_libraries(appcheckers PRIVATE Qt6::Widgets)
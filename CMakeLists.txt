cmake_minimum_required(VERSION 3.16)
project(cut_fruits VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 纯 C++ 只需要 Core, Gui, Widgets
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

# 包含所有的源文件
qt_add_executable(appcut_fruits
    main.cpp
    gamewidget.cpp
    gamewidget.h
)

target_link_libraries(appcut_fruits
    PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets
)
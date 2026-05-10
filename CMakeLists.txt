# 声明所需的最低 CMake 构建环境版本
cmake_minimum_required(VERSION 3.16)

# 定义工程的名称，并声明该工程主要使用 CXX (即 C++)
project(AdvancedCheckers LANGUAGES CXX)

# 指定 C++17 编译标准，并要求编译器必须严格执行
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 开启 Qt 独有的三大自动元编译预处理系统：
set(CMAKE_AUTOMOC ON)  # 解析代码中的 Q_OBJECT 宏生成 MOC 文件 (实现信号和槽)
set(CMAKE_AUTORCC ON)  # 将图片等资源 (.qrc) 编译成 C++ 字节码
set(CMAKE_AUTOUIC ON)  # 将设计师界面的 .ui XML 转换成 ui_*.h 头文件

# 查找本机安装的 Qt6 库，并引入必须的 Widgets 和 Gui 模块组件
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui)

# 将源文件链接组合在一起，最终编译为一个名为 "AdvancedCheckers" 的可执行程序（.exe / 产物）
qt_add_executable(AdvancedCheckers
    main.cpp
    GameTypes.h
    GameEngine.h
    GameEngine.cpp
    CheckerWidget.h
    CheckerWidget.cpp
)

# 将前面查找到的 Qt6 Widgets 和 Gui 库的动态链接或静态依赖引入我们的目标程序中
target_link_libraries(AdvancedCheckers PRIVATE Qt6::Widgets Qt6::Gui)
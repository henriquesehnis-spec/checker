# 声明所需的最低 CMake 构建环境版本
cmake_minimum_required(VERSION 3.16)

# 定义工程的名称，并声明该工程主要使用 CXX (即 C++ 语言)
project(AdvancedCheckers LANGUAGES CXX)

# 指定 C++17 编译标准，因为我们在 AI 里用到了现代 C++ 随机数库
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 开启 Qt 独有的三大自动元编译预处理系统（这是 Qt 项目的核心配置）：
set(CMAKE_AUTOMOC ON)  # 自动解析代码中的 Q_OBJECT 宏，生成 MOC 文件
set(CMAKE_AUTORCC ON)  # 【极其关键】自动将图片等资源 (.qrc 文件) 编译成 C++ 字节码打包进 exe
set(CMAKE_AUTOUIC ON)  # 自动将 .ui 界面文件转换成 C++ 头文件

# 查找本机安装的 Qt6 库，并引入必须的 Widgets 和 Gui 模块组件
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui)

# 将源文件链接组合在一起，最终编译为一个可执行程序
qt_add_executable(AdvancedCheckers
    main.cpp
    GameTypes.h
    GameEngine.h
    GameEngine.cpp
    CheckerWidget.h
    CheckerWidget.cpp
    resources.qrc      # 【关键】把资源配置文件加入编译列表
)

# 将查找到的 Qt6 库动态链接引入我们的目标程序中
target_link_libraries(AdvancedCheckers PRIVATE Qt6::Widgets Qt6::Gui)
#include <QApplication>
#include "CheckerWidget.h"

int main(int argc, char *argv[]) {
    // 实例化 Qt 应用程序主控对象
    QApplication a(argc, argv);
    // 采用 "Fusion" 样式风格，确保在 Windows/Mac/Linux 上的控件外观现代且一致
    a.setStyle("Fusion");

    // 实例化并创建我们的主游戏窗口
    CheckerWidget w;
    // 为操作系统应用窗口设置标题名称
    w.setWindowTitle("战术能量跳棋 Pro - MVC架构版");
    // 将窗口从内存展现到显示器上
    w.show();

    // 将执行权移交给 Qt 事件主循环机制（等待绘图指令、鼠标和键盘输入等）
    return a.exec();
}
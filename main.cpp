#include <QApplication>
#include "CheckerWidget.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setStyle("Fusion"); // 使用跨平台最整洁的风格

    CheckerWidget w;
    // 换上新霸气的窗口名字
    w.setWindowTitle("战术能量跳棋 Pro - 玉面手雷王降临版");
    w.show();

    return a.exec();
}
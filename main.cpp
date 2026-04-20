#include <QApplication>
#include "checkergame.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    CheckerGame w;
    w.setWindowTitle("纯C++能量系统国际跳棋");
    w.show();
    return a.exec();
}
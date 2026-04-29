#include <QApplication>
#include "gamewidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    GameWidget game;
    game.show();
    return app.exec();
}
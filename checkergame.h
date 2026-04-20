#ifndef CHECKERGAME_H
#define CHECKERGAME_H

#include <QWidget>
#include <QVector>
#include <QTimer>

enum PieceType { Empty = 0, RedMan = 1, BlackMan = 2, RedKing = 3, BlackKing = 4 };

class CheckerGame : public QWidget {
    Q_OBJECT

public:
    CheckerGame(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override; // 增加键盘控制切换模式

private:
    int board[8][8];
    int energy[8][8];
    int turn = 1;
    int selRow = -1, selCol = -1;
    bool mustCapture = false;
    bool aiMode = true; // 默认开启人机模式

    void initBoard();
    bool isOpponent(int r, int c, int p);
    bool canCapture(int r, int c);
    bool hasAnyCapture(int player);
    bool isValidMove(int r1, int c1, int r2, int c2, bool &isCap, bool &isEnergyMove);
    void performMove(int r1, int c1, int r2, int c2);
    void promote(int r, int c);
    void aiMove(); // AI 决策函数
};

#endif
#ifndef CHECKERWIDGET_H
#define CHECKERWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QPixmap>
#include "GameEngine.h"

// 负责整个界面的绘制与事件分发，实现了菜单、规则和游戏的切换
class CheckerWidget : public QWidget {
    Q_OBJECT // 允许使用 Qt 信号槽机制

public:
    CheckerWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    GameEngine engine;
    DisplayState currentState = Menu;             // UI状态机：默认主菜单
    int selRow = -1, selCol = -1;                 // 选中的棋子
    bool aiMode = true;                           // 对战模式开关

    // --- 新增：玉面手雷王爆炸动画状态变量 ---
    bool isExploding = false;    // 标记当前是否正在播放手雷王特效
    int explodeR = -1;           // 爆炸中心的行坐标
    int explodeC = -1;           // 爆炸中心的列坐标
    int explodeFrame = 0;        // 动画帧计数器（0~30）
    QTimer* explodeTimer;        // 驱动动画的定时器
    QPixmap bombImage;           // 存储加载进来的恶搞表情包

    // 三大界面绘制分离器
    void drawMenu(QPainter &p);
    void drawRules(QPainter &p);
    void drawGame(QPainter &p);

    void aiMove(); // AI 驱动函数

private slots:
    void updateExplosion(); // 每一帧更新爆炸画面的槽函数
};

#endif // CHECKERWIDGET_H
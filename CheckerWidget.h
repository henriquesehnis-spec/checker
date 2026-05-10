#ifndef CHECKERWIDGET_H
#define CHECKERWIDGET_H

#include <QWidget>
#include <QTimer>
#include "GameEngine.h"

// UI 层：继承 QWidget 以实现界面的绘制和事件捕获
class CheckerWidget : public QWidget {
    Q_OBJECT // 必备宏，允许使用 Qt 的核心机制如信号与槽

public:
    CheckerWidget(QWidget *parent = nullptr); // 默认构造函数

protected:
    // 覆盖重写 QWidget 的三大核心虚函数
    void paintEvent(QPaintEvent *event) override;       // 负责视觉呈现（画出棋盘与棋子）
    void mousePressEvent(QMouseEvent *event) override;  // 监听并处理鼠标点击事件
    void keyPressEvent(QKeyEvent *event) override;      // 监听键盘输入（如 S 键切换模式）

private:
    GameEngine engine;              // Model：包含全盘数据和规则的黑盒引擎
    int selRow = -1, selCol = -1;   // UI状态记录：当前高亮的选中棋子 (-1 表示未选中)
    bool aiMode = true;             // UI状态记录：标记开启的是人机模式还是人人模式

    // 定时器调用的 AI 延迟走棋辅助函数
    void aiMove();
};

#endif // CHECKERWIDGET_H
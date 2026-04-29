#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QList>
#include <QPoint>
#include <QColor>
#include <QString>

// 果汁粒子特效结构
struct Particle {
    double x, y, vx, vy;
    int alpha; // 透明度 (寿命)
    QColor color;
    int radius;
};

// 飘浮文字结构 (用于显示连击和加分)
struct FloatingText {
    double x, y;
    QString text;
    int alpha;
    QColor color;
    int size;
};

// 水果数据结构
struct Fruit {
    double x, y, vx, vy;
    int type; // 0: 普通, 1: 金苹果, 2: 炸弹, 3: 冰冻水果(新)
    bool isCut;
    int radius;
    QColor color;
    double rotation; // 自转角度
};

class GameWidget : public QWidget {
    Q_OBJECT

public:
    GameWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void updateGame();
    void spawnFruits();

private:
    void checkCollision(const QPoint &pos);
    void createExplosion(double x, double y, QColor color); // 生成果汁粒子
    void addFloatingText(double x, double y, QString text, QColor color, int size);
    void restartGame();

    QTimer *gameTimer;
    QTimer *spawnTimer;

    QList<Fruit> fruits;
    QList<QPoint> trail;
    QList<Particle> particles;
    QList<FloatingText> floatingTexts;

    int score;
    int lives;
    bool gameOver;
    int bombFlashCounter;

    // 创新机制变量
    int freezeTimer; // 子弹时间倒计时
    int comboCount;  // 当前连击数
    int comboTimer;  // 连击有效时间窗口
};

#endif // GAMEWIDGET_H
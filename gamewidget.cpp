#include "gamewidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QPainterPath>
#include <cmath>

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent), score(0), lives(10), gameOver(false),
    bombFlashCounter(0), freezeTimer(0), comboCount(0), comboTimer(0)
{
    setWindowTitle("切水果 - 冰火连击硬核版");
    setFixedSize(800, 600);
    setMouseTracking(false);

    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &GameWidget::updateGame);
    gameTimer->start(16); // 约60帧

    spawnTimer = new QTimer(this);
    connect(spawnTimer, &QTimer::timeout, this, &GameWidget::spawnFruits);
    spawnTimer->start(1200);
}

void GameWidget::spawnFruits() {
    if (gameOver) return;

    // 如果处于冰冻时间，生成的数量变少
    int count = (freezeTimer > 0) ? QRandomGenerator::global()->bounded(1, 2)
                                  : QRandomGenerator::global()->bounded(1, 4);

    for (int i = 0; i < count; ++i) {
        Fruit f;
        f.radius = 40;
        f.x = QRandomGenerator::global()->bounded(50, width() - 100);
        f.y = height() + 50;
        f.vx = (QRandomGenerator::global()->generateDouble() - 0.5) * 12;
        f.vy = -16 - QRandomGenerator::global()->generateDouble() * 7;
        f.isCut = false;
        f.rotation = 0;

        double randType = QRandomGenerator::global()->generateDouble();
        if (randType > 0.9) {
            f.type = 2; // 10% 炸弹
            f.radius = 35;
            f.color = QColor(28, 28, 30);
        } else if (randType > 0.8) {
            f.type = 3; // 10% 冰冻水果 (新)
            f.radius = 30;
            f.color = QColor(0, 255, 255); // 蓝绿色
        } else if (randType > 0.7) {
            f.type = 1; // 10% 金苹果
            f.color = QColor(255, 204, 0);
        } else {
            f.type = 0; // 70% 普通水果
            QList<QColor> colors = {QColor(255, 59, 48), QColor(76, 217, 100), QColor(255, 149, 0)};
            f.color = colors[QRandomGenerator::global()->bounded(3)];
        }
        fruits.append(f);
    }

    // 动态难度，分数越高生成越快，最低400ms
    spawnTimer->setInterval(std::max(400, 1200 - score * 2));
}

void GameWidget::updateGame() {
    bombFlashCounter++;

    // 更新冰冻时间和连击时间
    if (freezeTimer > 0) freezeTimer--;
    if (comboTimer > 0) comboTimer--;
    else comboCount = 0; // 连击中断

    // 冰冻状态下全局速度变慢 0.3倍
    double timeScale = (freezeTimer > 0) ? 0.3 : 1.0;

    // 刀光轨迹消散
    if (!trail.isEmpty()) trail.removeFirst();

    // 更新水果物理
    for (int i = fruits.size() - 1; i >= 0; --i) {
        fruits[i].x += fruits[i].vx * timeScale;
        fruits[i].y += fruits[i].vy * timeScale;
        fruits[i].vy += 0.45 * timeScale; // 重力
        fruits[i].rotation += fruits[i].vx * timeScale;

        // 掉出屏幕
        if (fruits[i].y > height() + 100) {
            if (!fruits[i].isCut && fruits[i].type != 2 && !gameOver) {
                lives--;
                if (lives <= 0) gameOver = true;
            }
            fruits.removeAt(i);
        }
    }

    // 更新粒子特效
    for (int i = particles.size() - 1; i >= 0; --i) {
        particles[i].x += particles[i].vx * timeScale;
        particles[i].y += particles[i].vy * timeScale;
        particles[i].vy += 0.5 * timeScale;
        particles[i].alpha -= 5; // 粒子渐隐
        if (particles[i].alpha <= 0) particles.removeAt(i);
    }

    // 更新飘浮文字
    for (int i = floatingTexts.size() - 1; i >= 0; --i) {
        floatingTexts[i].y -= 1.0 * timeScale; // 向上飘
        floatingTexts[i].alpha -= 3;
        if (floatingTexts[i].alpha <= 0) floatingTexts.removeAt(i);
    }

    update(); // 触发界面的重绘
}

// 创新的果汁粒子生成器
void GameWidget::createExplosion(double x, double y, QColor color) {
    int particleCount = QRandomGenerator::global()->bounded(8, 15);
    for (int i = 0; i < particleCount; i++) {
        Particle p;
        p.x = x; p.y = y;
        p.vx = (QRandomGenerator::global()->generateDouble() - 0.5) * 20;
        p.vy = (QRandomGenerator::global()->generateDouble() - 0.5) * 20;
        p.alpha = 255;
        p.color = color;
        p.radius = QRandomGenerator::global()->bounded(3, 8);
        particles.append(p);
    }
}

// 添加飘浮文字
void GameWidget::addFloatingText(double x, double y, QString text, QColor color, int size) {
    FloatingText ft = {x, y, text, 255, color, size};
    floatingTexts.append(ft);
}

void GameWidget::checkCollision(const QPoint &pos) {
    for (int i = 0; i < fruits.size(); ++i) {
        if (!fruits[i].isCut) {
            double centerX = fruits[i].x + fruits[i].radius;
            double centerY = fruits[i].y + fruits[i].radius;
            double dx = centerX - pos.x();
            double dy = centerY - pos.y();
            if (std::sqrt(dx * dx + dy * dy) < fruits[i].radius + 25) {
                fruits[i].isCut = true;

                // 连击判定
                comboCount++;
                comboTimer = 30; // 给约半秒钟的时间接续连击
                int comboMultiplier = (comboCount > 1) ? comboCount : 1;

                int gainedScore = 0;

                if (fruits[i].type == 0) gainedScore = 10 * comboMultiplier;
                else if (fruits[i].type == 1) gainedScore = 30 * comboMultiplier;
                else if (fruits[i].type == 3) {
                    gainedScore = 10 * comboMultiplier;
                    freezeTimer = 180; // 冰冻 3 秒 (180帧)
                    addFloatingText(width()/2, 100, "FREEZE!", QColor(0, 255, 255), 40);
                }
                else if (fruits[i].type == 2) {
                    lives -= 2; // 炸弹扣2滴血
                    comboCount = 0;
                    if (lives <= 0) gameOver = true;
                    createExplosion(centerX, centerY, QColor(255, 59, 48));
                    addFloatingText(centerX, centerY, "BOOM!", QColor(255, 59, 48), 30);
                }

                if (fruits[i].type != 2) {
                    score += gainedScore;
                    createExplosion(centerX, centerY, fruits[i].color);
                    QString scoreText = QString("+%1").arg(gainedScore);
                    if (comboCount > 1) scoreText += QString(" (Combo x%1)").arg(comboCount);
                    addFloatingText(centerX, centerY, scoreText, fruits[i].color, 20);
                }

                // 切开后的视觉变化
                fruits[i].vx = fruits[i].vx > 0 ? fruits[i].vx + 5 : fruits[i].vx - 5;
                fruits[i].vy = -2;
                fruits[i].color = (fruits[i].type == 2) ? QColor(255, 59, 48) : QColor(142, 142, 147, 150);
                fruits[i].radius /= 1.5;
            }
        }
    }
}

void GameWidget::mousePressEvent(QMouseEvent *event) {
    if (gameOver) {
        QRect restartBtn(width() / 2 - 80, height() / 2 + 50, 160, 50);
        if (restartBtn.contains(event->pos())) restartGame();
        return;
    }
    trail.clear();
    trail.append(event->pos());
    checkCollision(event->pos());
}

void GameWidget::mouseMoveEvent(QMouseEvent *event) {
    if (gameOver) return;
    trail.append(event->pos());
    if (trail.size() > 15) trail.removeFirst();
    checkCollision(event->pos());
}

void GameWidget::mouseReleaseEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    trail.clear();
    comboCount = 0; // 松开鼠标连击中断
}

void GameWidget::restartGame() {
    score = 0; lives = 10; gameOver = false;
    freezeTimer = 0; comboCount = 0;
    fruits.clear(); trail.clear(); particles.clear(); floatingTexts.clear();
}

void GameWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 背景：如果在冰冻期间，背景泛蓝
    if (freezeTimer > 0) painter.fillRect(rect(), QColor(20, 40, 60));
    else painter.fillRect(rect(), QColor(44, 62, 80));

    // 绘制粒子 (果汁)
    painter.setPen(Qt::NoPen);
    for (const Particle &p : particles) {
        QColor c = p.color; c.setAlpha(p.alpha);
        painter.setBrush(c);
        painter.drawEllipse(p.x, p.y, p.radius * 2, p.radius * 2);
    }

    // 绘制水果
    for (const Fruit &f : fruits) {
        painter.save();
        painter.translate(f.x + f.radius, f.y + f.radius);
        painter.rotate(f.rotation);

        painter.setBrush(f.color);
        if (f.type == 2 && !f.isCut) {
            painter.setPen(QPen((bombFlashCounter % 15 < 7) ? QColor(255, 59, 48) : Qt::white, 4));
            painter.drawRoundedRect(-f.radius, -f.radius, f.radius * 2, f.radius * 2, 10, 10);
        } else if (f.type == 3 && !f.isCut) {
            // 冰冻水果画成菱形
            painter.setPen(QPen(Qt::white, 3));
            QPolygon polygon;
            polygon << QPoint(0, -f.radius) << QPoint(f.radius, 0) << QPoint(0, f.radius) << QPoint(-f.radius, 0);
            painter.drawPolygon(polygon);
        } else {
            painter.setPen(QPen(Qt::white, 2));
            painter.drawEllipse(-f.radius, -f.radius, f.radius * 2, f.radius * 2);
        }
        painter.restore();
    }

    // 绘制飘浮文字
    for (const FloatingText &ft : floatingTexts) {
        QColor c = ft.color; c.setAlpha(ft.alpha);
        painter.setPen(c);
        painter.setFont(QFont("Arial", ft.size, QFont::Bold));
        painter.drawText(ft.x - 50, ft.y, ft.text);
    }

    // 绘制刀光
    if (trail.size() > 1) {
        QPainterPath path;
        path.moveTo(trail.first());
        for (int i = 1; i < trail.size(); ++i) path.lineTo(trail[i]);
        // 冰冻时刀光变蓝
        QColor trailColor = (freezeTimer > 0) ? QColor(0, 255, 255, 200) : QColor(255, 255, 255, 200);
        painter.setPen(QPen(trailColor, 6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }

    // 绘制 UI 状态栏
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 24, QFont::Bold));
    painter.drawText(20, 50, QString("分数: %1").arg(score));

    painter.setPen(QColor(255, 59, 48));
    QString hearts;
    for(int i = 0; i < lives; i++) hearts += "❤";
    painter.drawText(250, 50, QString("生命(%1/10): %2").arg(lives).arg(hearts));

    // Game Over
    if (gameOver) {
        painter.fillRect(rect(), QColor(0, 0, 0, 200));
        painter.setPen(QColor(255, 59, 48));
        painter.setFont(QFont("Arial", 48, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, "游戏结束\n最终得分: " + QString::number(score));

        QRect restartBtn(width() / 2 - 80, height() / 2 + 50, 160, 50);
        painter.setBrush(QColor(76, 217, 100));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(restartBtn, 25, 25);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 16, QFont::Bold));
        painter.drawText(restartBtn, Qt::AlignCenter, "重新开始");
    }
}
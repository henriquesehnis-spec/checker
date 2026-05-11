#include "CheckerWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <algorithm>

CheckerWidget::CheckerWidget(QWidget *parent) : QWidget(parent) {
    setFixedSize(600, 720);
    setFocusPolicy(Qt::StrongFocus);

    // 初始化玉面手雷王的动画引擎
    explodeTimer = new QTimer(this);
    connect(explodeTimer, &QTimer::timeout, this, &CheckerWidget::updateExplosion);

    // 【关键】从 QRC 资源文件里加载你的恶搞表情包！
    bombImage.load(":/7d8a76975f12ca0e00e1eef76cd4b5bf.jpg");
}

// 动画每帧更新逻辑 (每30毫秒调用一次)
void CheckerWidget::updateExplosion() {
    explodeFrame++; // 推进一帧

    // 动画长度设定为 30 帧 (大约 0.9 秒)
    if (explodeFrame > 30) {
        explodeTimer->stop();   // 动画结束，停下计时器
        isExploding = false;    // 解除动画屏蔽锁定

        // 动画放完后，如果是 AI 的回合（也就是刚才玩家扔了炸弹，现在轮到AI反击了），唤醒 AI
        if (engine.status == Playing && aiMode && engine.turn == 2) {
            QTimer::singleShot(400, this, &CheckerWidget::aiMove);
        }
    }
    update(); // 触发 paintEvent 画下一帧
}

// 视图分配器：根据当前所处状态机渲染不同画面
void CheckerWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    switch (currentState) {
    case Menu:   drawMenu(p);  break; // 画主菜单
    case Rules:  drawRules(p); break; // 画规则书
    case Gaming: drawGame(p);  break; // 画棋盘
    }
}

// --- 绘制主菜单 ---
void CheckerWidget::drawMenu(QPainter &p) {
    // 渐变深蓝背景
    QLinearGradient bg(0, 0, 0, height());
    bg.setColorAt(0, QColor(44, 62, 80));
    bg.setColorAt(1, QColor(52, 73, 94));
    p.fillRect(rect(), bg);

    p.setPen(Qt::white);
    p.setFont(QFont("Microsoft YaHei", 36, QFont::Bold));
    p.drawText(QRect(0, 100, 600, 100), Qt::AlignCenter, "战术能量跳棋");

    // 绘制进入游戏按钮
    p.setBrush(QColor(46, 204, 113));
    p.drawRoundedRect(150, 300, 300, 60, 10, 10);
    p.setFont(QFont("Microsoft YaHei", 18, QFont::Bold));
    p.drawText(QRect(150, 300, 300, 60), Qt::AlignCenter, "开始推演 (START)");

    // 绘制规则说明按钮
    p.setBrush(QColor(52, 152, 219));
    p.drawRoundedRect(150, 400, 300, 60, 10, 10);
    p.drawText(QRect(150, 400, 300, 60), Qt::AlignCenter, "查看规则 (RULES)");
}

// --- 绘制规则界面 ---
void CheckerWidget::drawRules(QPainter &p) {
    p.fillRect(rect(), QColor(236, 240, 241));

    p.setPen(QColor(44, 62, 80));
    p.setFont(QFont("Microsoft YaHei", 24, QFont::Bold));
    p.drawText(20, 60, "战术手册 & 规则说明");

    int y = 120;
    // 辅助闭包：用于优雅排版文本段落
    auto drawSection = [&](QString title, QStringList lines, QColor color) {
        p.setPen(color);
        p.setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
        p.drawText(30, y, title);
        y += 30;
        p.setPen(Qt::black);
        p.setFont(QFont("Microsoft YaHei", 11));
        for (const QString& line : lines) {
            p.drawText(50, y, "• " + line);
            y += 25;
        }
        y += 20;
    };

    drawSection("1. 基础移动", {"交替行动，棋子仅沿对角线向前走", "越过敌方且落点为空即为'吃子'", "若能吃子，系统强制执行，且支持连续跳杀"}, QColor(39, 174, 96));
    drawSection("2. 能量机制", {"移动积攒1点能量，吃子触发Combo高额加成", "满5点能量吃子：落地释放'震荡波'削弱周围敌军"}, QColor(211, 84, 0));
    // 文本更新：引入玉面手雷王
    drawSection("3. 战术大招", {"[右键点击]: 满5点兵原地'重构升王'(耗尽本回合)", "[按 O 键]: 满5点兵化身'玉面手雷王'，嘲讽并摧毁3x3敌军"}, QColor(192, 57, 43));
    drawSection("4. 界面操作", {"[S]: 切换人机/双人对弈 | [H]: 切换AI困难/简单算法", "[Esc]: 随时返回主菜单 | [左键]: 选中与落子"}, QColor(142, 68, 173));

    // 返回菜单按钮
    p.setBrush(QColor(44, 62, 80));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(200, 630, 200, 50, 5, 5);
    p.setPen(Qt::white);
    p.setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
    p.drawText(QRect(200, 630, 200, 50), Qt::AlignCenter, "我知道了 (BACK)");
}

// --- 绘制对战棋盘核心画面 ---
void CheckerWidget::drawGame(QPainter &p) {
    int s = 75;

    // 1. 画棋盘和尚存的棋子
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            p.setBrush((r+c)%2==0 ? QColor(245,245,220) : QColor(139,69,19));
            p.setPen(Qt::NoPen);
            p.drawRect(c*s, r*s, s, s);

            if (r == selRow && c == selCol) {
                p.setBrush(QColor(255,255,0,150));
                p.drawRect(c*s, r*s, s, s);
            }

            int piece = engine.board[r][c];
            if (piece != Empty) {
                p.setBrush(QColor(0,0,0,50));
                p.drawEllipse(c*s+12, r*s+12, s-20, s-20);

                bool isRed = (piece % 2 != 0);
                QRadialGradient grad(c*s+s/2, r*s+s/2, s/2);
                grad.setColorAt(0, isRed ? QColor(255,100,100) : QColor(80,80,80));
                grad.setColorAt(1, isRed ? Qt::red : Qt::black);
                p.setBrush(grad);
                p.drawEllipse(c*s+10, r*s+10, s-20, s-20);

                if (piece >= 3) {
                    p.setPen(QPen(Qt::yellow, 2));
                    p.drawText(QRect(c*s, r*s+25, s, 20), Qt::AlignCenter, "KING");
                }

                int e = engine.energy[r][c];
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(40,40,40));
                p.drawRoundedRect(c*s+15, r*s+60, s-30, 6, 3, 3);
                p.setBrush(e >= 5 ? Qt::magenta : Qt::cyan);
                p.drawRoundedRect(c*s+15, r*s+60, (s-30)*(e/5.0), 6, 3, 3);
            }
        }
    }

    // 2. 【核心新增】特效渲染层：如果当前正在播放爆炸，把玉面手雷王画在最顶层
    if (isExploding) {
        // 计算爆炸中心的绝对像素点
        int cx = explodeC * s + s / 2;
        int cy = explodeR * s + s / 2;

        // 步骤 A: 画扩散的火红冲击波
        int radius = explodeFrame * 8; // 圈圈随着帧数变大
        // 外圈烈焰，随时间逐渐透明
        p.setPen(QPen(QColor(255, 100, 0, std::max(0, 255 - explodeFrame * 8)), 6));
        p.drawEllipse(QPoint(cx, cy), radius, radius);
        // 内圈高光
        p.setPen(QPen(QColor(255, 0, 0, std::max(0, 255 - explodeFrame * 8)), 3));
        p.drawEllipse(QPoint(cx, cy), radius + 15, radius + 15);

        // 步骤 B: 绘制玉面手雷王的图片 (缩放+淡出特效)
        // 透明度算法：前 20 帧完全不透明，后 10 帧迅速淡出消失
        double opacity = (explodeFrame < 20) ? 1.0 : std::max(0.0, 1.0 - ((explodeFrame - 20) / 10.0));
        p.setOpacity(opacity);

        // 缩放特效算法：图片尺寸从 50px 迅速膨胀到 140px，带来极强的视觉冲击力 (从小变大弹出)
        int imgSize = 50 + (explodeFrame * 3);

        // 如果成功读取了你的神图，就把它以缩放的形式画在爆炸中心
        if (!bombImage.isNull()) {
            p.drawPixmap(cx - imgSize/2, cy - imgSize/2, imgSize, imgSize, bombImage);
        } else {
            // 防呆后路：如果因为文件路径不对等原因没找到图片，画个巨大的爆炸 Emoji 兜底
            p.setFont(QFont("Microsoft YaHei", imgSize/2));
            p.drawText(cx - imgSize/2, cy - imgSize/2, imgSize, imgSize, Qt::AlignCenter, "💥");
        }
        p.setOpacity(1.0); // 恢复正常的画笔透明度，以免影响底层 UI
    }

    // 3. 画底部状态信息栏
    p.setPen(Qt::black);
    p.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
    QString diffStr = aiMode ? (engine.aiDifficulty==Hard ? "[困难算力]" : "[简单算力]") : "";
    p.drawText(20, 620, QString("回合归属: %1 | 模式: %2 %3").arg(engine.turn==1?"红方(您)":"黑方(敌)").arg(aiMode?"人机":"双人").arg(diffStr));

    if (engine.mustCapture) {
        p.setPen(Qt::red);
        p.drawText(20, 650, "⚠️ 触发猎杀锁定：必须吃子！(特殊技能暂时离线)");
    }
    else {
        p.setPen(QColor(0, 100, 200));
        // 面板更新：玉面手雷王
        p.drawText(20, 650, "★ [玉面手雷王] 选中满能量兵按 O 键，召唤嘲讽炸弹摧毁 3x3 敌军");
    }

    // 4. 画结算黑布
    if (engine.status != Playing) {
        p.setBrush(QColor(0, 0, 0, 180));
        p.drawRect(0, 0, width(), height());
        p.setFont(QFont("Microsoft YaHei", 40, QFont::Bold));
        p.setPen(Qt::white);
        p.drawText(QRect(0, 250, 600, 100), Qt::AlignCenter, engine.status == RedWin ? "红方 胜利！" : "黑方 胜利！");
    }
}

// 鼠标交互控制分发器
void CheckerWidget::mousePressEvent(QMouseEvent *e) {
    if (currentState == Menu) {
        if (QRect(150, 300, 300, 60).contains(e->pos())) {
            currentState = Gaming; engine.reset();
        }
        else if (QRect(150, 400, 300, 60).contains(e->pos())) {
            currentState = Rules;
        }
    }
    else if (currentState == Rules) {
        if (QRect(200, 630, 200, 50).contains(e->pos())) { currentState = Menu; }
    }
    else if (currentState == Gaming) {
        if (engine.status != Playing) { engine.reset(); selRow = -1; selCol = -1; update(); return; }

        // 【关键保护】拒绝在“玉面手雷王”爆炸动画播放时或 AI 思考时乱点，防止破坏数据
        if (isExploding || (aiMode && engine.turn == 2)) return;

        int r = e->position().y() / 75;
        int c = e->position().x() / 75;
        if (r < 0 || r > 7 || c < 0 || c > 7) return;

        // 右键：主动升王
        if (e->button() == Qt::RightButton) {
            if (engine.performActivePromotion(r, c)) {
                selRow = -1;
                if (aiMode && engine.turn == 2) QTimer::singleShot(600, this, &CheckerWidget::aiMove);
            }
        }
        // 左键：物理移动
        else {
            if (selRow == -1) {
                int p = engine.board[r][c];
                if (p != Empty && ((p+1)%2+1 == engine.turn)) {
                    if (engine.mustCapture && !engine.canCapture(r, c)) return;
                    selRow = r; selCol = c;
                }
            } else {
                bool con = engine.performMove(selRow, selCol, r, c);
                if (con) { selRow = r; selCol = c; }
                else {
                    selRow = -1;
                    if (aiMode && engine.turn == 2) QTimer::singleShot(600, this, &CheckerWidget::aiMove);
                }
            }
        }
    }
    update();
}

// 键盘交互分发器
void CheckerWidget::keyPressEvent(QKeyEvent *event) {
    // 任何状态按 Esc 回主菜单
    if (event->key() == Qt::Key_Escape) {
        currentState = Menu; update(); return;
    }

    if (currentState == Gaming && !isExploding) {
        if (event->key() == Qt::Key_H) {
            engine.aiDifficulty = (engine.aiDifficulty == Easy) ? Hard : Easy;
        }
        else if (event->key() == Qt::Key_S) {
            aiMode = !aiMode;
        }
        // 按下 O 键：触发大招
        else if (event->key() == Qt::Key_O) {
            if (selRow != -1 && engine.status == Playing && !(aiMode && engine.turn == 2)) {

                // 提前记住自爆中心坐标，因为一旦底层执行引爆，这个棋子就没了
                int er = selRow, ec = selCol;

                // 向底层呼叫玉面手雷王
                if (engine.performJadeGrenade(selRow, selCol)) {
                    selRow = -1;

                    // 【核心】启动震天动地的嘲讽动画引擎！
                    isExploding = true;
                    explodeR = er;
                    explodeC = ec;
                    explodeFrame = 0;
                    explodeTimer->start(30); // 每 30ms 跑一帧动画
                    // 注意：回合交接的 AI 唤醒逻辑放在了 updateExplosion 动画彻底放完之后
                }
            }
        }

        if (aiMode && engine.turn == 2 && engine.status == Playing && !isExploding) {
            aiMove();
        }
        update();
    }
}

// 供 AI 使用的虚拟鼠标
void CheckerWidget::aiMove() {
    if (engine.turn != 2 || engine.status != Playing || isExploding) return;

    int r1, c1, r2, c2, aT;

    // AI 获取最优解
    if (engine.generateAIMove(r1, c1, r2, c2, aT, selRow, selCol)) {
        if (aT == 1) {
            // AI 被逼急了，决定向玩家扔出玉面手雷王！
            engine.performJadeGrenade(r1, c1);
            selRow = -1;

            // 触发华丽且嘲讽的满屏动画
            isExploding = true;
            explodeR = r1;
            explodeC = c1;
            explodeFrame = 0;
            explodeTimer->start(30);
        }
        else if (aT == 2) {
            engine.performActivePromotion(r1, c1);
            selRow = -1;
        }
        else {
            bool con = engine.performMove(r1, c1, r2, c2);
            if (con) {
                selRow = r2; selCol = c2;
                QTimer::singleShot(500, this, &CheckerWidget::aiMove);
                return;
            }
            selRow = -1;
        }
        update();
    }
}
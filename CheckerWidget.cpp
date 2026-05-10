#include "CheckerWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainterPath>

// 构造函数：初始化自定义的 QWidget
CheckerWidget::CheckerWidget(QWidget *parent) : QWidget(parent) {
    // 设置固定的窗口大小，宽600，高720（下方预留了120像素的高度用于显示文本信息和能量提示）
    setFixedSize(600, 720);
    // 设置焦点策略为 StrongFocus，使得该组件能够捕获并响应键盘事件（用于监听 'S' 键切换模式）
    setFocusPolicy(Qt::StrongFocus);
}

// 绘图事件：负责绘制棋盘、棋子、状态文本栏和游戏结算界面
// 只要界面需要重绘（如调用了 update()），系统就会自动触发此函数
void CheckerWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    // 开启抗锯齿，使圆形棋子、圆角矩形和文字的边缘更加平滑
    p.setRenderHint(QPainter::Antialiasing);
    // 定义每个棋盘格子的边长为 75 像素 (600 / 8 = 75)
    int s = 75;

    // 1. 遍历并绘制 8x8 的棋盘和棋子
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            // 绘制深浅交替的棋盘格子。(r+c) 为偶数时使用浅色，奇数时使用深色
            p.setBrush((r+c)%2==0 ? QColor(245,245,220) : QColor(139,69,19));
            p.setPen(Qt::NoPen); // 去除矩形的黑色边框线
            p.drawRect(c*s, r*s, s, s);

            // 如果当前网格正好是被玩家选中的棋子所在位置，绘制一个半透明的黄色高亮框
            if (r == selRow && c == selCol) {
                p.setBrush(QColor(255,255,0,150));
                p.drawRect(c*s, r*s, s, s);
            }

            // 获取当前底层核心引擎在该位置的棋子状态
            int piece = engine.board[r][c];

            // 如果该位置有棋子（状态不是 Empty）
            if (piece != Empty) {
                // 先在棋子稍微偏右下方的位置绘制一个半透明的黑色阴影，增加立体感
                p.setBrush(QColor(0,0,0,50));
                p.drawEllipse(c*s+12, r*s+12, s-20, s-20);

                // 判断棋子所属阵营：根据你的底层设定，奇数应该是红方，偶数是黑方
                bool isRed = (piece % 2 != 0);

                // 使用径向渐变 (QRadialGradient) 来绘制棋子本体，模拟光泽感
                QRadialGradient grad(c*s+s/2, r*s+s/2, s/2);
                grad.setColorAt(0, isRed ? QColor(255,100,100) : QColor(80,80,80)); // 内部中心较亮
                grad.setColorAt(1, isRed ? Qt::red : Qt::black);                    // 外部边缘较暗
                p.setBrush(grad);
                p.drawEllipse(c*s+10, r*s+10, s-20, s-20);

                // 如果棋子数值 >= 3，说明它已经晋升为“王(King)”，需要特殊标识
                if (piece >= 3) {
                    p.setPen(QPen(Qt::yellow, 2));
                    p.drawText(QRect(c*s, r*s+25, s, 20), Qt::AlignCenter, "KING");
                }

                // 2. 绘制该棋子专属的能量槽 (Energy Bar)
                int e = engine.energy[r][c];
                p.setPen(Qt::NoPen);
                // 绘制能量槽的深灰色底层背景
                p.setBrush(QColor(40,40,40));
                p.drawRoundedRect(c*s+15, r*s+60, s-30, 6, 3, 3);

                // 根据能量是否攒满 (>=5) 选择不同颜色（满为洋红，不满为青色）
                p.setBrush(e >= 5 ? Qt::magenta : Qt::cyan);
                // 根据当前的能量百分比 (e/5.0) 动态计算并绘制当前进度条的宽度
                p.drawRoundedRect(c*s+15, r*s+60, (s-30)*(e/5.0), 6, 3, 3);
            }
        }
    }

    // 3. 绘制棋盘下方的信息面板（状态栏）
    p.setPen(Qt::black);
    QFont font = p.font();
    font.setBold(true);
    p.setFont(font);

    // 动态显示当前轮到哪一方操作
    p.drawText(20, 630, QString("当前回合: %1").arg(engine.turn==1 ? "红方 (玩家)" : "黑方"));
    // 显示当前的对战模式状态
    p.drawText(20, 655, QString("战斗模式: %1 | [S] 切换").arg(aiMode ? "人机对战" : "本地双人"));

    // 显示核心机制提示文字：如果存在必须吃子的强制规则
    if (engine.mustCapture) {
        p.setPen(Qt::red);
        p.drawText(20, 685, "⚠️ 必须吃子！选择闪烁的路径。");
    } else {
        // 正常状态下的特色玩法提示
        p.setPen(QColor(50,100,50));
        p.drawText(20, 685, "提示: 满能量吃子可触发能量震荡，削弱周围敌军。");
    }

    // 4. 绘制游戏结束（结算）的遮罩画面
    if (engine.status != Playing) {
        // 画一层大面积的半透明黑色遮罩盖住整局棋盘，突出文字
        p.setBrush(QColor(0, 0, 0, 180));
        p.drawRect(0, 0, width(), height());

        // 设置大号加粗字体显示胜利者
        QFont vFont("Microsoft YaHei", 40, QFont::Bold);
        p.setFont(vFont);
        QString winText = (engine.status == RedWin ? "红方 胜利！" : "黑方 胜利！");
        p.setPen(Qt::white);
        p.drawText(QRect(0, 250, 600, 100), Qt::AlignCenter, winText);

        // 显示重新开始的引导语
        p.setFont(QFont("Microsoft YaHei", 18));
        p.drawText(QRect(0, 350, 600, 50), Qt::AlignCenter, "点击任意处重新开始游戏");
    }
}

// 鼠标按下事件：处理玩家选择棋子和走棋的全部交互逻辑
void CheckerWidget::mousePressEvent(QMouseEvent *e) {
    // 拦截 1：如果游戏已经结算，任意点击都会使核心引擎重置，并恢复初始状态
    if (engine.status != Playing) {
        engine.reset();
        selRow = -1; selCol = -1;
        update();
        return;
    }

    // 拦截 2：如果处于人机对战模式，且当前是 AI（黑方，turn==2）的回合，屏蔽玩家一切鼠标操作
    if (aiMode && engine.turn == 2) return;

    // 根据鼠标点击的具体坐标 (x, y) 映射回底层的二维数组索引
    int r = static_cast<int>(e->position().y()) / 75;
    int c = static_cast<int>(e->position().x()) / 75;

    // 安全校验：如果点击在有效棋盘外（比如点到了底部的信息面板区域），则直接无视
    if (r < 0 || r > 7 || c < 0 || c > 7) return;

    // 【阶段 A：当前尚未选中任何己方棋子】
    if (selRow == -1) {
        int p = engine.board[r][c];
        // 校验逻辑：判断点击位置不为空，且该棋子必须属于当前行动方
        // 数学处理 (p+1)%2+1 是利用取余将 1/3 映射为 1(红方)，2/4 映射为 2(黑方)
        if (p != Empty && ((p+1)%2+1 == engine.turn)) {
            // 如果底层引擎强制规定当前必须吃子，但你选中的这颗棋子无法吃子，则不允许你选它
            if (engine.mustCapture && !engine.canCapture(r, c)) return;

            // 校验通过，锁定选中棋子的坐标
            selRow = r;
            selCol = c;
        }
    }
    // 【阶段 B：已经选中了己方棋子，正在点击目标落点】
    else {
        // 尝试让引擎执行移动，engine.performMove 将处理诸如斜线校验、连跳、能量逻辑与晋升等底层操作
        bool continueTurn = engine.performMove(selRow, selCol, r, c);

        if (continueTurn) {
            // 如果返回 true，说明发生了吃子且这颗棋子还可以继续“连跳”
            // 此时不结束回合，而是把当前选中坐标更新到落点，让玩家继续走下一步连跳
            selRow = r;
            selCol = c;
        } else {
            // 如果返回 false，说明走步完毕（普通移动，或者不能再连跳了）
            // 释放选定状态
            selRow = -1;
            selCol = -1;

            // 回合结束，检查如果还是进行状态，是 AI 模式，且现在轮到 AI 操作
            if (engine.status == Playing && aiMode && engine.turn == 2) {
                // 使用单次定时器延迟 600 毫秒后再触发 AI 走棋
                // （主要是为了留给玩家视觉上的反应缓冲时间，避免 UI 瞬移变化太快）
                QTimer::singleShot(600, this, &CheckerWidget::aiMove);
            }
        }
    }
    // 处理完点击逻辑后，务必调用 update() 来触发 paintEvent 重绘画面
    update();
}

// 键盘按键事件：监听处理快捷键操作
void CheckerWidget::keyPressEvent(QKeyEvent *event) {
    // 按下了 'S' 键，用于即时切换对战模式 (人机对战 <-> 本地双人对战)
    if (event->key() == Qt::Key_S) {
        aiMode = !aiMode;
        // 如果正好在黑方回合时，玩家按 S 键从双人模式切换到了人机模式，应当立即唤醒 AI 进行走步
        if (aiMode && engine.turn == 2 && engine.status == Playing) {
            aiMove();
        }
        update();
    }
}

// AI 行动函数：驱动核心算法在人机模式中自动走步
void CheckerWidget::aiMove() {
    // 容错校验：只有当轮到黑方且游戏进行中，才允许 AI 往下跑
    if (engine.turn != 2 || engine.status != Playing) return;

    int r1, c1, r2, c2;
    // 如果 AI 当前处在多重连跳中途，它需要根据上一次连跳落点 (selRow, selCol) 继续寻找下一步
    // generateAIMove 返回 AI 计算的移动起点 (r1,c1) 和落点 (r2,c2)
    if (engine.generateAIMove(r1, c1, r2, c2, selRow, selCol)) {

        // 把 AI 计算出的走法送入核心引擎执行
        bool continueTurn = engine.performMove(r1, c1, r2, c2);

        if (continueTurn) {
            // AI 完成了吃子并发现还能继续连跳，把自己的当前落点记作下一轮选定点
            selRow = r2;
            selCol = c2;
            // 延迟 500 毫秒后再次调用自己，展现出连跳动画效果
            QTimer::singleShot(500, this, &CheckerWidget::aiMove);
        } else {
            // AI 这轮彻底走完了，清空选定状态，并把控制权交还给下回合的玩家
            selRow = -1;
            selCol = -1;
        }
        // 更新画面让玩家看到 AI 的落子效果
        update();
    }
}
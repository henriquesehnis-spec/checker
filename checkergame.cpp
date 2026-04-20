#include "checkergame.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTimer>
#include <random>   // 现代 C++ 随机数库
#include <ctime>

CheckerGame::CheckerGame(QWidget *parent) : QWidget(parent) {
    setFixedSize(600, 680);
    setFocusPolicy(Qt::StrongFocus); // 确保窗口能接收键盘 S 键
    initBoard();
}

void CheckerGame::initBoard() {
    for(int r=0; r<8; ++r) {
        for(int c=0; c<8; ++c) {
            board[r][c] = Empty; energy[r][c] = 0;
            if ((r + c) % 2 != 0) {
                if (r < 3) board[r][c] = BlackMan;
                else if (r > 4) board[r][c] = RedMan;
            }
        }
    }
    mustCapture = hasAnyCapture(turn);
}

bool CheckerGame::isOpponent(int r, int c, int p) {
    if (r < 0 || r > 7 || c < 0 || c > 7) return false;
    int target = board[r][c];
    if (target == Empty) return false;
    if (p == 1) return (target == BlackMan || target == BlackKing);
    return (target == RedMan || target == RedKing);
}

bool CheckerGame::canCapture(int r, int c) {
    if (board[r][c] == Empty) return false;
    int p = (board[r][c] % 2 != 0) ? 1 : 2;
    bool isKing = (board[r][c] >= 3 || energy[r][c] >= 5);
    int dirs[4][2] = {{1,1}, {1,-1}, {-1,1}, {-1,-1}};
    for (auto d : dirs) {
        if (!isKing) {
            int tr = r + 2*d[0], tc = c + 2*d[1];
            if (tr>=0 && tr<8 && tc>=0 && tc<8 && isOpponent(r+d[0], c+d[1], p) && board[tr][tc] == Empty) return true;
        } else {
            for (int i=1; i<7; ++i) {
                int mr = r+i*d[0], mc = c+i*d[1], tr = r+(i+1)*d[0], tc = c+(i+1)*d[1];
                if (tr<0 || tr>7 || tc<0 || tc>7) break;
                if (board[mr][mc] != Empty) {
                    if (isOpponent(mr, mc, p) && board[tr][tc] == Empty) return true;
                    break;
                }
            }
        }
    }
    return false;
}

bool CheckerGame::hasAnyCapture(int p) {
    for (int r=0; r<8; ++r)
        for (int c=0; c<8; ++c)
            if (board[r][c] != Empty && ((board[r][c]+1)%2+1 == p))
                if (canCapture(r, c)) return true;
    return false;
}

bool CheckerGame::isValidMove(int r1, int c1, int r2, int c2, bool &isCap, bool &isE) {
    isCap = false; isE = false;
    if (r1==r2 && c1==c2) return false;
    if (r2 < 0 || r2 > 7 || c2 < 0 || c2 > 7 || board[r2][c2] != Empty) return false;
    int p = (board[r1][c1] % 2 != 0) ? 1 : 2;
    bool isKing = (board[r1][c1] >= 3 || energy[r1][c1] >= 5);
    int dr = r2 - r1, dc = c2 - c1;

    if (std::abs(dr) >= 2 && std::abs(dr) == std::abs(dc)) {
        int ur = dr/std::abs(dr), uc = dc/std::abs(dc);
        if (!isKing && std::abs(dr) == 2) {
            if (isOpponent(r1+ur, c1+uc, p)) { isCap = true; return true; }
        } else if (isKing) {
            int ops = 0;
            for (int i=1; i<std::abs(dr); ++i) {
                if (board[r1+i*ur][c1+i*uc] != Empty) {
                    if (isOpponent(r1+i*ur, c1+i*uc, p)) ops++; else return false;
                }
            }
            if (ops == 1) { isCap = true; return true; }
        }
    }
    if (mustCapture) return false;
    if (dr == 0 && std::abs(dc) == 1 && energy[r1][c1] >= 3) { isE = true; return true; }
    if (!isKing) {
        if (dr == (p==1 ? -1 : 1) && std::abs(dc) == 1) return true;
    } else if (std::abs(dr) == std::abs(dc)) {
        for (int i=1; i<std::abs(dr); ++i)
            if (board[r1+i*(dr/std::abs(dr))][c1+i*(dc/std::abs(dc))] != Empty) return false;
        return true;
    }
    return false;
}

void CheckerGame::performMove(int r1, int c1, int r2, int c2) {
    bool isCap = false, isE = false;
    if (isValidMove(r1, c1, r2, c2, isCap, isE)) {
        int curE = energy[r1][c1];
        int pType = board[r1][c1];
        board[r2][c2] = pType; board[r1][c1] = Empty;
        if (isE) curE -= 3; else curE = std::min(5, curE + 1);

        if (isCap) {
            int ur = (r2-r1)/std::abs(r2-r1), uc = (c2-c1)/std::abs(c2-c1);
            for(int i=1; i<std::abs(r2-r1); ++i) board[r1+i*ur][c1+i*uc] = Empty;
            energy[r2][c2] = curE; promote(r2, c2);
            if (canCapture(r2, c2)) {
                selRow = r2; selCol = c2; mustCapture = true; update();
                if (aiMode && turn == 2) QTimer::singleShot(500, this, &CheckerGame::aiMove);
                return;
            }
        }
        energy[r2][c2] = curE; energy[r1][c1] = 0; promote(r2, c2);
        turn = (turn == 1) ? 2 : 1; selRow = -1; selCol = -1;
        mustCapture = hasAnyCapture(turn);
        update();
        if (aiMode && turn == 2) QTimer::singleShot(600, this, &CheckerGame::aiMove);
    }
}

void CheckerGame::aiMove() {
    if (turn != 2) return;
    QVector<QPair<QPoint, QPoint>> caps, normals;
    for (int r=0; r<8; ++r) {
        for (int c=0; c<8; ++c) {
            if (board[r][c] == BlackMan || board[r][c] == BlackKing) {
                for (int tr = 0; tr < 8; ++tr) {
                    for (int tc = 0; tc < 8; ++tc) {
                        bool ic, ie;
                        if (isValidMove(r, c, tr, tc, ic, ie)) {
                            if (ic) caps.append({QPoint(r,c), QPoint(tr,tc)});
                            else normals.append({QPoint(r,c), QPoint(tr,tc)});
                        }
                    }
                }
            }
        }
    }

    // 使用现代 C++ 随机数生成器，彻底告别 rand
    static std::mt19937 gen(std::time(nullptr));
    if (!caps.isEmpty()) {
        std::uniform_int_distribution<> dis(0, caps.size() - 1);
        auto m = caps[dis(gen)];
        performMove(m.first.x(), m.first.y(), m.second.x(), m.second.y());
    } else if (!normals.isEmpty()) {
        std::uniform_int_distribution<> dis(0, normals.size() - 1);
        auto m = normals[dis(gen)];
        performMove(m.first.x(), m.first.y(), m.second.x(), m.second.y());
    }
}

void CheckerGame::promote(int r, int c) {
    if (board[r][c] == RedMan && r == 0) board[r][c] = RedKing;
    if (board[r][c] == BlackMan && r == 7) board[r][c] = BlackKing;
}

void CheckerGame::paintEvent(QPaintEvent *) {
    QPainter p(this); int s = 75;
    for (int r=0; r<8; ++r) {
        for (int c=0; c<8; ++c) {
            p.setBrush((r+c)%2==0 ? QColor(240,217,181) : QColor(181,136,99));
            p.drawRect(c*s, r*s, s, s);
            if (r == selRow && c == selCol) { p.setBrush(QColor(255,255,0,120)); p.drawRect(c*s, r*s, s, s); }
            if (board[r][c] != Empty) {
                p.setBrush((board[r][c]%2!=0) ? Qt::red : Qt::black);
                p.drawEllipse(c*s+10, r*s+10, s-20, s-20);
                if (board[r][c] >= 3) { p.setPen(Qt::yellow); p.drawText(c*s+20, r*s+40, "KING"); }
                p.setPen(Qt::NoPen); p.setBrush(Qt::cyan);
                p.drawRect(c*s+15, r*s+60, (s-30)*(energy[r][c]/5.0), 4);
                p.setPen(Qt::white); p.drawText(c*s+32, r*s+70, QString::number(energy[r][c]));
            }
        }
    }
    p.setPen(Qt::black);
    p.drawText(20, 630, QString("回合: %1 | 模式: %2 (点一下窗口按S切换)").arg(turn==1?"红方":"黑方").arg(aiMode?"人机":"双人"));
    if (mustCapture) { p.setPen(Qt::red); p.drawText(20, 655, "【强制规则】必须吃子！点击能跳吃的棋子。"); }
    else { p.setPen(Qt::darkGray); p.drawText(20, 655, "提示：能量>=3可横移，满5临时变王。"); }
}

void CheckerGame::mousePressEvent(QMouseEvent *e) {
    if (aiMode && turn == 2) return;
    // Qt 6 推荐使用 position()
    int r = static_cast<int>(e->position().y())/75;
    int c = static_cast<int>(e->position().x())/75;
    if (r < 0 || r > 7 || c < 0 || c > 7) return;
    if (selRow == -1) {
        int p = board[r][c];
        if (p != Empty && ((p+1)%2+1 == turn)) {
            if (mustCapture && !canCapture(r, c)) return;
            selRow = r; selCol = c;
        }
    } else {
        performMove(selRow, selCol, r, c);
        if (!mustCapture) { selRow = -1; selCol = -1; }
    }
    update();
}

void CheckerGame::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_S) {
        aiMode = !aiMode;
        if (aiMode && turn == 2) aiMove();
        update();
    }
}
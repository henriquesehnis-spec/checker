#include "GameEngine.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <ctime>

GameEngine::GameEngine() { reset(); }

void GameEngine::initBoard() {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            board[r][c] = Empty; energy[r][c] = 0;
            // 跳棋规则：棋子只能走在深色格子上
            if ((r + c) % 2 != 0) {
                if (r < 3) board[r][c] = BlackMan;       // 黑棋在上方 (0,1,2)
                else if (r > 4) board[r][c] = RedMan;    // 红棋在下方 (5,6,7)
            }
        }
    }
}

void GameEngine::reset() {
    initBoard(); turn = 1; status = Playing; comboCount = 0;
    mustCapture = hasAnyCapture(turn);
}

bool GameEngine::isOpponent(int r, int c, int p) const {
    if (r < 0 || r > 7 || c < 0 || c > 7) return false;
    int target = board[r][c];
    if (target == Empty) return false;
    return (p == 1) ? (target == BlackMan || target == BlackKing) : (target == RedMan || target == RedKing);
}

bool GameEngine::canCapture(int r, int c) const {
    if (board[r][c] == Empty) return false;
    int p = (board[r][c] % 2 != 0) ? 1 : 2;
    bool isKing = (board[r][c] >= 3); // 只有真正的王棋才能远距离吃子

    int dirs[4][2] = {{1,1}, {1,-1}, {-1,1}, {-1,-1}};
    for (auto d : dirs) {
        if (!isKing) {
            // 普通棋子吃子：跨度固定为2
            int tr = r + 2*d[0], tc = c + 2*d[1];
            if (tr>=0 && tr<8 && tc>=0 && tc<8 && isOpponent(r+d[0], c+d[1], p) && board[tr][tc] == Empty) return true;
        } else {
            // 王棋吃子：射线扫描
            for (int i = 1; i < 7; ++i) {
                int mr = r + i*d[0], mc = c + i*d[1];
                int tr = r + (i+1)*d[0], tc = c + (i+1)*d[1];
                if (tr < 0 || tr > 7 || tc < 0 || tc > 7) break;
                if (board[mr][mc] != Empty) {
                    if (isOpponent(mr, mc, p) && board[tr][tc] == Empty) return true;
                    break;
                }
            }
        }
    }
    return false;
}

bool GameEngine::hasAnyCapture(int p) const {
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c)
            if (board[r][c] != Empty && ((board[r][c]+1)%2+1 == p))
                if (canCapture(r, c)) return true;
    return false;
}

bool GameEngine::hasAnyMove(int p) const {
    bool pMustCapture = hasAnyCapture(p);
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (board[r][c] != Empty && ((board[r][c]+1)%2+1 == p)) {
                for(int tr = 0; tr < 8; ++tr) {
                    for(int tc = 0; tc < 8; ++tc) {
                        bool ic, is;
                        if (isValidMoveInternal(r, c, tr, tc, ic, is, pMustCapture)) return true;
                    }
                }
            }
        }
    }
    return false;
}

bool GameEngine::isValidMove(int r1, int c1, int r2, int c2, bool &isCap, bool &isSpecial) const {
    return isValidMoveInternal(r1, c1, r2, c2, isCap, isSpecial, mustCapture);
}

bool GameEngine::isValidMoveInternal(int r1, int c1, int r2, int c2, bool &isCap, bool &isSpecial, bool currentMustCapture) const {
    isCap = false; isSpecial = false;
    if (r1 == r2 && c1 == c2) return false;
    if (r2 < 0 || r2 > 7 || c2 < 0 || c2 > 7 || board[r2][c2] != Empty) return false;

    int p = (board[r1][c1] % 2 != 0) ? 1 : 2;
    bool isKing = (board[r1][c1] >= 3);
    int dr = r2 - r1, dc = c2 - c1;

    if (std::abs(dr) >= 2 && std::abs(dr) == std::abs(dc)) {
        int ur = dr/std::abs(dr), uc = dc/std::abs(dc);
        if (!isKing && std::abs(dr) == 2) {
            if (isOpponent(r1+ur, c1+uc, p)) { isCap = true; return true; }
        } else if (isKing) {
            int ops = 0;
            for (int i = 1; i < std::abs(dr); ++i) {
                if (board[r1+i*ur][c1+i*uc] != Empty) {
                    if (isOpponent(r1+i*ur, c1+i*uc, p)) ops++;
                    else return false;
                }
            }
            if (ops == 1) { isCap = true; return true; }
        }
    }

    if (currentMustCapture) return false; // 拦截非吃子动作

    int backwardDir = (p == 1) ? 1 : -1;
    if (dr == backwardDir && std::abs(dc) == 1 && energy[r1][c1] >= 4) { isSpecial = true; return true; }
    if (dr == 0 && std::abs(dc) == 1 && energy[r1][c1] >= 3) { isSpecial = true; return true; }

    if (!isKing) {
        if (dr == (p==1 ? -1 : 1) && std::abs(dc) == 1) return true;
    } else if (std::abs(dr) == std::abs(dc)) {
        for (int i = 1; i < std::abs(dr); ++i) {
            if (board[r1+i*(dr/std::abs(dr))][c1+i*(dc/std::abs(dc))] != Empty) return false;
        }
        return true;
    }
    return false;
}

void GameEngine::applyEnergyShockwave(int r, int c) {
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            int nr = r + dr, nc = c + dc;
            if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8 && board[nr][nc] != Empty) {
                if (((board[nr][nc]+1)%2+1) != turn) energy[nr][nc] = std::max(0, energy[nr][nc] - 1);
            }
        }
    }
}

// ---------------------------------------------------
// 战术大招 1：玉面手雷王！
// ---------------------------------------------------
bool GameEngine::performJadeGrenade(int r, int c) {
    if (mustCapture || board[r][c] == Empty) return false;
    int p = (board[r][c] % 2 != 0) ? 1 : 2;
    if (p != turn || energy[r][c] < 5) return false; // 必须是自己的回合且满5点能量

    board[r][c] = Empty; // 抹除自己
    energy[r][c] = 0;

    // 席卷 3x3 范围
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            int nr = r + dr, nc = c + dc;
            if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
                if (isOpponent(nr, nc, p)) {
                    board[nr][nc] = Empty;   // 敌人被炸死
                    energy[nr][nc] = 0;
                }
            }
        }
    }

    comboCount = 0;
    turn = (turn == 1) ? 2 : 1;        // 交接回合
    mustCapture = hasAnyCapture(turn);
    checkVictory();
    return true;
}

// ---------------------------------------------------
// 战术大招 2：主动重构升王
// ---------------------------------------------------
bool GameEngine::performActivePromotion(int r, int c) {
    if (mustCapture || board[r][c] == Empty) return false;
    int type = board[r][c]; int p = (type % 2 != 0) ? 1 : 2;
    if (p != turn || type == RedKing || type == BlackKing || energy[r][c] < 5) return false;

    energy[r][c] = 0; // 抽干能量
    if (type == RedMan) board[r][c] = RedKing; else if (type == BlackMan) board[r][c] = BlackKing;

    comboCount = 0; turn = (turn == 1) ? 2 : 1; mustCapture = hasAnyCapture(turn); checkVictory();
    return true;
}

void GameEngine::promote(int r, int c) {
    if (board[r][c] == RedMan && r == 0) board[r][c] = RedKing;
    if (board[r][c] == BlackMan && r == 7) board[r][c] = BlackKing;
}

bool GameEngine::performMove(int r1, int c1, int r2, int c2) {
    bool isCap = false, isSpec = false;
    if (!isValidMove(r1, c1, r2, c2, isCap, isSpec)) return false;

    int curE = energy[r1][c1];
    int pType = board[r1][c1];
    bool wasFullEnergy = (curE >= 5);

    board[r2][c2] = pType; board[r1][c1] = Empty;

    if (isSpec) {
        if (std::abs(r2-r1) == 0) curE -= 3; else curE -= 4;
    }
    else if (isCap) {
        comboCount++;
        int ur = (r2-r1)/std::abs(r2-r1), uc = (c2-c1)/std::abs(c2-c1);
        int capE = 0;

        for(int i = 1; i < std::abs(r2-r1); ++i) {
            if(board[r1+i*ur][c1+i*uc] != Empty) {
                if (wasFullEnergy) applyEnergyShockwave(r1+i*ur, c1+i*uc);
                board[r1+i*ur][c1+i*uc] = Empty;
                capE++;
            }
        }

        curE = std::min(5, curE + comboCount + capE);
        energy[r2][c2] = curE;
        promote(r2, c2);

        if (canCapture(r2, c2)) { mustCapture = true; return true; }
    } else curE = std::min(5, curE + 1);

    energy[r2][c2] = curE; energy[r1][c1] = 0; promote(r2, c2);
    comboCount = 0; turn = (turn == 1) ? 2 : 1; mustCapture = hasAnyCapture(turn); checkVictory();
    return false;
}

void GameEngine::checkVictory() {
    bool rM = hasAnyMove(1); bool bM = hasAnyMove(2);
    if (!rM) status = BlackWin; else if (!bM) status = RedWin;
}

int GameEngine::evaluateBoard() const {
    if (status == BlackWin) return 999999;
    if (status == RedWin) return -999999;

    int s = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            int p = board[r][c]; int e = energy[r][c];
            if (p == BlackMan) s += (20 + e * 2 + r);
            else if (p == BlackKing) s += (60 + e * 2);
            else if (p == RedMan) s -= (20 + e * 2 + (7 - r));
            else if (p == RedKing) s -= (60 + e * 2);
        }
    }
    return s;
}

bool GameEngine::generateAIMove(int &r1, int &c1, int &r2, int &c2, int &aT, int cSR, int cSC) const {
    static std::mt19937 gen(std::time(nullptr));

    // 简单模式：不放技能，贪心吃子或乱走
    if (aiDifficulty == Easy) {
        std::vector<std::pair<std::pair<int,int>, std::pair<int,int>>> caps, normals;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                if (board[r][c] == BlackMan || board[r][c] == BlackKing) {
                    if (cSR != -1 && (r != cSR || c != cSC)) continue;
                    for (int tr = 0; tr < 8; ++tr) {
                        for (int tc = 0; tc < 8; ++tc) {
                            bool ic, is;
                            if (isValidMove(r, c, tr, tc, ic, is)) {
                                if (ic) caps.push_back({{r,c}, {tr,tc}}); else normals.push_back({{r,c}, {tr,tc}});
                            }
                        }
                    }
                }
            }
        }
        aT = 0;
        if (!caps.empty()) {
            std::uniform_int_distribution<> dis(0, caps.size() - 1); auto m = caps[dis(gen)];
            r1 = m.first.first; c1 = m.first.second; r2 = m.second.first; c2 = m.second.second; return true;
        } else if (!normals.empty()) {
            std::uniform_int_distribution<> dis(0, normals.size() - 1); auto m = normals[dis(gen)];
            r1 = m.first.first; c1 = m.first.second; r2 = m.second.first; c2 = m.second.second; return true;
        }
        return false;
    }

    // 困难模式：脑海沙盒推演计算
    int bestScore = -9999999; bool foundMove = false;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (board[r][c] == BlackMan || board[r][c] == BlackKing) {
                if (cSR != -1 && (r != cSR || c != cSC)) continue;

                // 预判 1：如果释放“玉面手雷王”
                GameEngine sE = *this;
                if (sE.performJadeGrenade(r, c)) {
                    int sc = sE.evaluateBoard();
                    if (sc > bestScore) { bestScore = sc; r1 = r; c1 = c; aT = 1; foundMove = true; }
                }

                // 预判 2：如果释放主动升王
                GameEngine sP = *this;
                if (sP.performActivePromotion(r, c)) {
                    int sc = sP.evaluateBoard();
                    if (sc > bestScore) { bestScore = sc; r1 = r; c1 = c; aT = 2; foundMove = true; }
                }

                // 预判 3：常规物理移动
                for (int tr = 0; tr < 8; ++tr) {
                    for (int tc = 0; tc < 8; ++tc) {
                        bool ic, is;
                        if (isValidMove(r, c, tr, tc, ic, is)) {
                            GameEngine sM = *this; bool con = sM.performMove(r, c, tr, tc);
                            int cR = tr, cC = tc;
                            while(con) {
                                int nR = -1, nC = -1; bool fN = false;
                                for(int nr=0; nr<8; ++nr) {
                                    for(int nc=0; nc<8; ++nc) {
                                        bool nic, nis;
                                        if (sM.isValidMove(cR, cC, nr, nc, nic, nis) && nic) { nR = nr; nC = nc; fN = true; break; }
                                    }
                                    if (fN) break;
                                }
                                if (fN) { con = sM.performMove(cR, cC, nR, nC); cR = nR; cC = nC; } else break;
                            }
                            int sc = sM.evaluateBoard() + (rand() % 5); // 加点随机噪音
                            if (sc > bestScore) { bestScore = sc; r1 = r; c1 = c; r2 = tr; c2 = tc; aT = 0; foundMove = true; }
                        }
                    }
                }
            }
        }
    }
    return foundMove;
}
#include "GameEngine.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <ctime>

// 构造函数：创建引擎时自动重置为开局状态
GameEngine::GameEngine() {
    reset();
}

// 棋盘初始化逻辑：标准的 8x8 国际跳棋开局布阵
void GameEngine::initBoard() {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            board[r][c] = Empty;
            energy[r][c] = 0;
            // 棋子只放在深色格子（行号+列号为奇数）
            if ((r + c) % 2 != 0) {
                if (r < 3) board[r][c] = BlackMan;       // 0,1,2 行放黑棋
                else if (r > 4) board[r][c] = RedMan;    // 5,6,7 行放红棋
            }
        }
    }
}

// 重置游戏状态
void GameEngine::reset() {
    initBoard();
    turn = 1;                     // 红方先手
    status = Playing;             // 状态设为进行中
    comboCount = 0;               // 连跳重置
    mustCapture = hasAnyCapture(turn); // 检查开局第一步是否就必须吃子（正常开局不会，但为了逻辑严密）
}

// 敌我识别函数
bool GameEngine::isOpponent(int r, int c, int p) const {
    // 越界检查
    if (r < 0 || r > 7 || c < 0 || c > 7) return false;
    int target = board[r][c];
    if (target == Empty) return false;
    // p==1(红方)的敌人是黑方普通棋和王棋；p==2的敌人是红方
    return (p == 1) ? (target == BlackMan || target == BlackKing) : (target == RedMan || target == RedKing);
}

// 判断某颗特定的棋子能否吃子
bool GameEngine::canCapture(int r, int c) const {
    if (board[r][c] == Empty) return false;
    int p = (board[r][c] % 2 != 0) ? 1 : 2; // 提取阵营
    bool isKing = (board[r][c] >= 3 || energy[r][c] >= 5); // 满能量的普通棋子也拥有王棋的视野

    // 4 个对角线方向向量 (右下, 右上, 左下, 左上)
    int dirs[4][2] = {{1,1}, {1,-1}, {-1,1}, {-1,-1}};
    for (auto d : dirs) {
        if (!isKing) {
            // 普通棋子只能跳过相邻格子吃子（跨度为2）
            int tr = r + 2*d[0], tc = c + 2*d[1];
            // 检查落点是否在棋盘内、相邻位置是否是敌人、落点是否为空
            if (tr>=0 && tr<8 && tc>=0 && tc<8 && isOpponent(r+d[0], c+d[1], p) && board[tr][tc] == Empty) return true;
        } else {
            // 王棋的“长兵”吃子逻辑：可以沿着对角线扫描很远
            for (int i=1; i<7; ++i) {
                int mr = r+i*d[0], mc = c+i*d[1];       // 探测路径上的格子 (mr, mc)
                int tr = r+(i+1)*d[0], tc = c+(i+1)*d[1]; // 如果跨过探测格，落点是 (tr, tc)
                if (tr<0 || tr>7 || tc<0 || tc>7) break;  // 越界则停止该方向搜索

                // 如果路径上碰到了棋子
                if (board[mr][mc] != Empty) {
                    // 如果碰到的恰好是敌人，且敌人背后是空的，说明可以吃！
                    if (isOpponent(mr, mc, p) && board[tr][tc] == Empty) return true;
                    // 如果碰到了友军，或者敌人背后被挡住了，由于不能穿透，跳出循环
                    break;
                }
            }
        }
    }
    return false;
}

// 检查指定阵营是否有任何棋子可以吃子
bool GameEngine::hasAnyCapture(int p) const {
    for (int r=0; r<8; ++r)
        for (int c=0; c<8; ++c)
            // 找到属于玩家 p 的棋子
            if (board[r][c] != Empty && ((board[r][c]+1)%2+1 == p))
                if (canCapture(r, c)) return true;
    return false;
}

// 独立计算特定玩家的 mustCapture，避免互相干扰
// 判断指定阵营是否还有合法移动，以此判定输赢
bool GameEngine::hasAnyMove(int p) const {
    // 提前计算当前玩家是否处于强制吃子状态
    bool pMustCapture = hasAnyCapture(p);
    for (int r=0; r<8; ++r)
        for (int c=0; c<8; ++c) {
            if (board[r][c] != Empty && ((board[r][c]+1)%2+1 == p)) {
                // 遍历棋盘上所有可能的目标落点
                for(int tr=0; tr<8; ++tr)
                    for(int tc=0; tc<8; ++tc) {
                        bool ic, is;
                        // 只要找到哪怕一种合法的走法，说明还没输
                        if (isValidMoveInternal(r, c, tr, tc, ic, is, pMustCapture)) return true;
                    }
            }
        }
    return false;
}

// 外部调用的包装接口
bool GameEngine::isValidMove(int r1, int c1, int r2, int c2, bool &isCap, bool &isSpecial) const {
    return isValidMoveInternal(r1, c1, r2, c2, isCap, isSpecial, mustCapture);
}

// 逻辑底层判断，隔离全局变量
// 验证走步是否合法的底层核心引擎，currentMustCapture 使其可以作为纯沙盒工具调用
bool GameEngine::isValidMoveInternal(int r1, int c1, int r2, int c2, bool &isCap, bool &isSpecial, bool currentMustCapture) const {
    isCap = false; isSpecial = false;
    if (r1==r2 && c1==c2) return false; // 原地不动，非法
    if (r2 < 0 || r2 > 7 || c2 < 0 || c2 > 7 || board[r2][c2] != Empty) return false; // 越界或落点被占用，非法

    int p = (board[r1][c1] % 2 != 0) ? 1 : 2;
    bool isKing = (board[r1][c1] >= 3 || energy[r1][c1] >= 5);
    int dr = r2 - r1, dc = c2 - c1;

    // 【判断 1：尝试吃子移动】（行距和列距相同，即走对角线）
    if (std::abs(dr) >= 2 && std::abs(dr) == std::abs(dc)) {
        int ur = dr/std::abs(dr), uc = dc/std::abs(dc); // 步进方向的单位向量 (+1或-1)
        if (!isKing && std::abs(dr) == 2) {
            // 普通棋子吃子：跨越 2 格，中间 1 格必须是敌人
            if (isOpponent(r1+ur, c1+uc, p)) { isCap = true; return true; }
        } else if (isKing) {
            // 王棋吃子：可以跨越多格，但路径上只能有恰好 1 个敌人，且不能有自己人
            int ops = 0;
            for (int i=1; i<std::abs(dr); ++i) {
                if (board[r1+i*ur][c1+i*uc] != Empty) {
                    if (isOpponent(r1+i*ur, c1+i*uc, p)) ops++;
                    else return false; // 撞到自己人，不合法
                }
            }
            if (ops == 1) { isCap = true; return true; }
        }
    }

    // 这里使用传入的 currentMustCapture，不会误杀无辜
    // 如果走到这里，说明刚才判定这不是吃子动作。如果当前存在“必须吃子”规则，则普通移动不合法！
    if (currentMustCapture) return false;

    // 【判断 2：自定义的特殊能量消耗技能】
    int backwardDir = (p == 1) ? 1 : -1; // 正常的后退方向
    // 耗费4点能量进行一次大后退特殊移动
    if (dr == backwardDir && std::abs(dc) == 1 && energy[r1][c1] >= 4) { isSpecial = true; return true; }
    // 耗费3点能量进行一次平移横行
    if (dr == 0 && std::abs(dc) == 1 && energy[r1][c1] >= 3) { isSpecial = true; return true; }

    // 【判断 3：常规移动】
    if (!isKing) {
        // 普通棋子只能往前走1格
        if (dr == (p==1 ? -1 : 1) && std::abs(dc) == 1) return true;
    } else if (std::abs(dr) == std::abs(dc)) {
        // 王棋可以沿着空白对角线飞任意距离
        for (int i=1; i<std::abs(dr); ++i)
            if (board[r1+i*(dr/std::abs(dr))][c1+i*(dc/std::abs(dc))] != Empty) return false;
        return true;
    }
    return false;
}

// 满能量吃子时触发的 AOE 技能：九宫格削减敌军能量
void GameEngine::applyEnergyShockwave(int r, int c) {
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            int nr = r + dr, nc = c + dc;
            // 扫描以当前位置为中心的 3x3 范围
            if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8 && board[nr][nc] != Empty) {
                // 如果该棋子不是我方的回合
                if (((board[nr][nc]+1)%2+1) != turn) {
                    // 扣减敌方1点能量，最低不低于0
                    energy[nr][nc] = std::max(0, energy[nr][nc] - 1);
                }
            }
        }
    }
}

// 玩家或AI确立移动后的业务执行逻辑，更新所有的矩阵和状态
bool GameEngine::performMove(int r1, int c1, int r2, int c2) {
    bool isCap = false, isSpec = false;
    // 再次校验合法性（防误触），并通过引用拿到这是哪种移动
    if (!isValidMove(r1, c1, r2, c2, isCap, isSpec)) return false;

    int curE = energy[r1][c1];
    int pType = board[r1][c1];
    bool wasFullEnergy = (curE >= 5);

    // 棋子移动过去
    board[r2][c2] = pType;
    board[r1][c1] = Empty;

    if (isSpec) {
        // 特殊技能扣除相应能量
        if (std::abs(r2-r1) == 0) curE -= 3;
        else curE -= 4;
    } else if (isCap) {
        // 成功触发吃子移动！
        comboCount++; // 连跳次数累加

        // 抹除被跨越吃掉的敌方棋子，同时记录本次真正吃掉的数量
        int ur = (r2-r1)/std::abs(r2-r1), uc = (c2-c1)/std::abs(c2-c1);
        int capturedPiecesExtraEnergy = 0; // 【新增】用于额外奖励能量的计数器

        for(int i=1; i<std::abs(r2-r1); ++i) {
            if(board[r1+i*ur][c1+i*uc] != Empty) {
                // 若刚才移动前是满能量，则在此刻被吃棋子的中心释放震荡波
                if (wasFullEnergy) applyEnergyShockwave(r1+i*ur, c1+i*uc);

                board[r1+i*ur][c1+i*uc] = Empty; // 杀敌（将该位置置空）
                capturedPiecesExtraEnergy++;     // 【新增】每真正吃掉一个棋子，就额外记1点能量
            }
        }

        // 【核心修改】
        // 能量计算公式更新：基础能量 + 连击奖励(comboCount) + 额外吃子奖励(capturedPiecesExtraEnergy)
        // 使用 std::min 确保总能量加成不会超过上限 5 点
        curE = std::min(5, curE + comboCount + capturedPiecesExtraEnergy);

        energy[r2][c2] = curE;
        promote(r2, c2); // 检查到达底线是否能升王

        // 核心连跳机制：吃完子后如果此时在这个新位置还能继续吃子
        if (canCapture(r2, c2)) {
            mustCapture = true; // 强制要求继续吃
            return true; // 【返回 true】告诉 UI 层回合别结束，要求玩家或AI继续接着落子
        }
    } else {
        // 普通移动积累1点能量
        curE = std::min(5, curE + 1);
    }

    // 更新终点能量，清空起点能量
    energy[r2][c2] = curE;
    energy[r1][c1] = 0;
    promote(r2, c2);

    // 回合正式结束交接
    comboCount = 0;
    turn = (turn == 1) ? 2 : 1; // 交换攻守
    mustCapture = hasAnyCapture(turn); // 检查对方接盘时是否必须吃子
    checkVictory(); // 判定输赢

    return false; // 【返回 false】告诉 UI 层本回合已彻底走完
}

// 底线升王机制
void GameEngine::promote(int r, int c) {
    if (board[r][c] == RedMan && r == 0) board[r][c] = RedKing;       // 红棋到达最顶端
    if (board[r][c] == BlackMan && r == 7) board[r][c] = BlackKing;   // 黑棋到达最底端
}

// 胜负判定引擎
void GameEngine::checkVictory() {
    bool redHasMove = hasAnyMove(1);
    bool blackHasMove = hasAnyMove(2);
    // 谁陷入了无棋可走（或棋子全被吃光）的境地，谁就输了
    if (!redHasMove) status = BlackWin;
    else if (!blackHasMove) status = RedWin;
}

// AI 行走步生成（随机决策算法）
bool GameEngine::generateAIMove(int &r1, int &c1, int &r2, int &c2, int currentSelRow, int currentSelCol) const {
    // 两个集合分别用来存放：可以吃子的方案、普通移动的方案
    std::vector<std::pair<std::pair<int,int>, std::pair<int,int>>> caps, normals;

    // 全盘扫描寻找 AI 的棋子
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (board[r][c] == BlackMan || board[r][c] == BlackKing) {
                // 如果当前处在连跳中（传入了前一次落点的坐标），AI 只能选用这颗特定的棋子接着跳
                if (currentSelRow != -1 && (r != currentSelRow || c != currentSelCol)) continue;

                // 穷举该棋子去 64 个格子的可能性
                for (int tr = 0; tr < 8; ++tr) {
                    for (int tc = 0; tc < 8; ++tc) {
                        bool ic, is;
                        if (isValidMove(r, c, tr, tc, ic, is)) {
                            // 分类收集合法路径
                            if (ic) caps.push_back({{r,c}, {tr,tc}});
                            else normals.push_back({{r,c}, {tr,tc}});
                        }
                    }
                }
            }
        }
    }

    // 使用现代 C++ 随机数生成器确保 AI 走法的不可预测性
    static std::mt19937 gen(std::time(nullptr));

    // 贪心策略：优先执行能吃子的走法
    if (!caps.empty()) {
        std::uniform_int_distribution<> dis(0, caps.size() - 1);
        auto m = caps[dis(gen)];
        r1 = m.first.first; c1 = m.first.second;
        r2 = m.second.first; c2 = m.second.second;
        return true;
    }
    // 如果没有子可吃，随机选一个普通移动
    else if (!normals.empty()) {
        std::uniform_int_distribution<> dis(0, normals.size() - 1);
        auto m = normals[dis(gen)];
        r1 = m.first.first; c1 = m.first.second;
        r2 = m.second.first; c2 = m.second.second;
        return true;
    }
    // 死局（无路可走）
    return false;
}
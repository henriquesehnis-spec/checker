#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include "GameTypes.h"
#include <vector>
#include <utility>

// 核心引擎类：处理跳棋的所有规则、能量机制与AI计算
class GameEngine {
public:
    int board[8][8];      // 8x8 棋盘二维数组，存储 PieceType
    int energy[8][8];     // 8x8 能量二维数组，存储每个棋子的当前能量值
    int turn;             // 记录当前轮到哪一方走棋 (1:红方玩家, 2:黑方/AI)
    bool mustCapture;     // 标记当前回合是否存在“必须吃子”的强制规则约束
    GameStatus status;    // 游戏当前状态（进行中、红胜、黑胜）
    int comboCount;       // 连跳计数器，用于触发能量积攒的加成机制

    GameEngine();         // 构造函数
    void reset();         // 初始化/重置游戏到开局状态

    // 验证从 (r1, c1) 移动到 (r2, c2) 是否符合规则，并通过引用返回是否吃子或触发特殊移动
    bool isValidMove(int r1, int c1, int r2, int c2, bool &isCap, bool &isSpecial) const;
    // 检查指定坐标的棋子当前是否能吃掉敌方棋子
    bool canCapture(int r, int c) const;
    // 实际执行移动逻辑（会更新棋盘、能量并判断是否连跳/回合结束），返回该棋子是否能继续连跳
    bool performMove(int r1, int c1, int r2, int c2);
    // 生成 AI 的合法走步，并写入参数 r1,c1 (起点) 和 r2,c2 (终点)
    bool generateAIMove(int &r1, int &c1, int &r2, int &c2, int currentSelRow = -1, int currentSelCol = -1) const;

private:
    void initBoard();                               // 将棋子按照初始规则摆放
    bool isOpponent(int r, int c, int p) const;     // 判断目标坐标上的棋子是否为玩家 p 的敌方
    bool hasAnyCapture(int p) const;                // 全局扫描：玩家 p 是否有任何一颗棋子可以吃子
    bool hasAnyMove(int p) const;                   // 全局扫描：玩家 p 是否存在任何合法的移动（若无则判负）
    void applyEnergyShockwave(int r, int c);        // 满能量吃子时触发：削减周围敌军的能量
    void promote(int r, int c);                     // 检查并执行普通棋子到达底线晋升为“王”的逻辑
    void checkVictory();                            // 检查对局是否结束（某方无路可走）

    // 核心修复点：新增一个带状态隔离的内部判断函数
    // 允许在不破坏全局 mustCapture 状态的前提下，传入 currentMustCapture 进行沙盒预测
    bool isValidMoveInternal(int r1, int c1, int r2, int c2, bool &isCap, bool &isSpecial, bool currentMustCapture) const;
};

#endif // GAMEENGINE_H
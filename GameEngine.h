#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include "GameTypes.h"
#include <vector>

// 核心引擎类：纯粹的数据与规则计算大脑，不涉及任何图形界面绘制
class GameEngine {
public:
    int board[8][8];                  // 8x8 物理棋盘矩阵
    int energy[8][8];                 // 8x8 能量矩阵，存储棋子当前能量 (0~5)

    int turn;                         // 当前行动方 (1:红方/玩家, 2:黑方/电脑)
    bool mustCapture;                 // 核心规则：标记当前是否“必须吃子”
    GameStatus status;                // 当前游戏胜负状态
    int comboCount;                   // 连击计数器：用于连跳额外能量奖励
    AIDifficulty aiDifficulty = Hard; // 默认将 AI 设置为困难推演模式

    GameEngine();
    void reset();

    // 核心物理与移动接口
    bool isValidMove(int r1, int c1, int r2, int c2, bool &isCap, bool &isSpecial) const;
    bool canCapture(int r, int c) const;
    bool performMove(int r1, int c1, int r2, int c2);

    // AI 大脑接口：生成 AI 的走步。actionType 区分动作：0=普通移动/吃子, 1=玉面手雷王, 2=主动升王
    bool generateAIMove(int &r1, int &c1, int &r2, int &c2, int &actionType, int currentSelRow = -1, int currentSelCol = -1) const;

    // --- 战术大招接口 ---
    // 技能 1：玉面手雷王（满5点能量触发，抹杀自身，摧毁周围3x3所有敌人）
    bool performJadeGrenade(int r, int c);

    // 技能 2：主动原地重构升王（满5点能量触发，消耗本回合直接升王）
    bool performActivePromotion(int r, int c);

private:
    void initBoard();                               // 开局摆子
    bool isOpponent(int r, int c, int p) const;     // 阵营判别
    bool hasAnyCapture(int p) const;                // 全局必须吃子扫描
    bool hasAnyMove(int p) const;                   // 全局死局检测
    void applyEnergyShockwave(int r, int c);        // 被动技能：满能量吃子震荡波
    void promote(int r, int c);                     // 经典规则：走到底线升王
    void checkVictory();                            // 胜负裁判

    // 沙盒预测引擎：供 AI 平行宇宙推演用的内部验证器
    bool isValidMoveInternal(int r1, int c1, int r2, int c2, bool &isCap, bool &isSpecial, bool currentMustCapture) const;
    // 局势打分系统：困难 AI 专属，给未来的棋盘战斗力打分
    int evaluateBoard() const;
};

#endif // GAMEENGINE_H
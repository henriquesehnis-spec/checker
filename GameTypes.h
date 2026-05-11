#ifndef GAMETYPES_H
#define GAMETYPES_H

// 枚举：定义棋盘上 8x8 共 64 个格子中可能存在的物理状态
enum PieceType {
    Empty = 0,       // 空白格子
    RedMan = 1,      // 红方普通棋子
    BlackMan = 2,    // 黑方普通棋子
    RedKing = 3,     // 红方王棋
    BlackKing = 4    // 黑方王棋
};

// 枚举：定义当前对局的胜负状态
enum GameStatus {
    Playing,         // 游戏进行中
    RedWin,          // 红方胜利
    BlackWin         // 黑方胜利
};

// 枚举：定义 AI 的智力水平（难度）
enum AIDifficulty {
    Easy,            // 简单：贪心吃子，不会用大招
    Hard             // 困难：沙盒推演预测未来局势
};

// 枚举：定义 UI 界面当前处于哪个显示状态（用于实现多界面切换）
enum DisplayState {
    Menu,            // 主菜单界面
    Rules,           // 规则说明界面
    Gaming           // 战斗棋盘界面
};

#endif // GAMETYPES_H
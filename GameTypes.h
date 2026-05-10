#ifndef GAMETYPES_H
#define GAMETYPES_H

// 枚举：棋盘上每个格子的状态/棋子类型
// 0:空, 1:红方普通棋子, 2:黑方普通棋子, 3:红方王棋, 4:黑方王棋
enum PieceType { Empty = 0, RedMan = 1, BlackMan = 2, RedKing = 3, BlackKing = 4 };

// 枚举：当前对局的整体状态
// Playing:进行中, RedWin:红方胜利, BlackWin:黑方胜利
enum GameStatus { Playing, RedWin, BlackWin };

#endif // GAMETYPES_H
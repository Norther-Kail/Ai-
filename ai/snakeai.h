// ============================================================
// ai/snakeai.h — 自动寻路 AI（每步 BFS 安全验证）
// 作用: 每步选朝向食物的方向，走之前 BFS 验证能否追到蛇尾
// ============================================================

#ifndef SNAKEAI_H
#define SNAKEAI_H

#include <QPoint>
#include <QVector>
#include "../game/snakegame.h"

class SnakeAI
{
public:
    SnakeAI();

    Direction computeNextMove(const SnakeGame &game);

    bool isActive() const { return m_active; }
    void setActive(bool active) { m_active = active; }

private:
    bool m_active;

    // BFS 最短路径（支持穿墙）
    QVector<QPoint> bfsPath(const QPoint &start, const QPoint &target,
                            bool obstacle[SnakeGame::GRID_WIDTH][SnakeGame::GRID_HEIGHT]) const;

    // BFS 能否从 start 到达 target（只判断连通性）
    bool bfsCanReach(const QPoint &start, const QPoint &target,
                     bool obstacle[SnakeGame::GRID_WIDTH][SnakeGame::GRID_HEIGHT]) const;

    // 走一步后的安全检查: 从 nextPos 能否 BFS 到蛇尾
    bool isMoveSafe(const SnakeGame &game, const QPoint &nextPos) const;

    // Flood fill 计算可达空格数
    int floodFill(const QPoint &start,
                  bool obstacle[SnakeGame::GRID_WIDTH][SnakeGame::GRID_HEIGHT]) const;

    Direction pointsToDirection(const QPoint &from, const QPoint &to) const;
    bool isOpposite(Direction a, Direction b) const;
    Direction survivalMove(const SnakeGame &game,
                           bool obstacle[SnakeGame::GRID_WIDTH][SnakeGame::GRID_HEIGHT]) const;
};

#endif // SNAKEAI_H

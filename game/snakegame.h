// ============================================================
// game/snakegame.h — 贪吃蛇游戏核心逻辑（纯数据层）
// 作用: 管理蛇、食物、分数、游戏状态
// 特点: 不依赖 Qt GUI，可以独立测试和复用
// ============================================================

#ifndef SNAKEGAME_H
#define SNAKEGAME_H

#include <QPoint>      // 坐标点 (x, y)
#include <QVector>     // 动态数组（存蛇身）
#include <QObject>     // 信号与槽基类

// ---- 游戏状态 ----
enum GameState {      //enum 枚举
    STATE_READY,      // 等待开始
    STATE_PLAYING,    // 游戏中
    STATE_PAUSED,     // 暂停
    STATE_GAMEOVER    // 游戏结束
};

// ---- 方向 ----
enum Direction {
    DIR_NONE = 0,     // 静止
    DIR_UP,           // 上
    DIR_DOWN,         // 下
    DIR_LEFT,         // 左
    DIR_RIGHT         // 右
};

// ---- 难度等级 ----
enum Difficulty {
    DIFF_EASY,       // 简单: 150ms 慢速
    DIFF_NORMAL,    // 普通: 100ms 中速
    DIFF_HARD        // 困难: 70ms 快速
};

class SnakeGame : public QObject
{
    Q_OBJECT

public:
    explicit SnakeGame(QObject *parent = nullptr); //explicit 禁止隐式类转换

    // ---- 常量（可外部读取）----
    static const int GRID_WIDTH  = 30;   // 网格宽度（格数）
    static const int GRID_HEIGHT = 30;  // 网格高度（格数）
    static const int BASE_INTERVAL = 100; // 基础速度（毫秒）
    static const int MIN_INTERVAL  = 50;  // 最快速度（毫秒）
    static const int SCORE_PER_FOOD = 10; // 每个食物的分数

    // ---- 游戏控制 ----
    void reset();           // 重置游戏（重新开始）
    void start();           // 开始游戏
    void pause();           // 暂停
    void resume();          // 继续
    void togglePause();     // 切换暂停/继续

    // ---- 输入 ----
    bool setDirection(Direction dir);  // 设置蛇的方向，返回是否成功

    // ---- 每帧更新 ----
    void tick();            // 执行一帧逻辑（移动、碰撞、吃食物）

    // ---- 难度设置 ----
    void setDifficulty(Difficulty diff);
    Difficulty difficulty() const { return m_difficulty; }

    // ---- 最高分持久化 ----
    void loadHighScore();
    void saveHighScore() const;

    // ---- 状态查询 ----
    GameState state() const { return m_state; }
    int score() const { return m_score; }
    int highScore() const { return m_highScore; }
    int snakeLength() const { return m_body.size() + 1; } // +1 算蛇头
    int currentInterval() const;  // 当前速度间隔

    // ---- 数据获取（供 UI 绘制使用）----
    QPoint head() const { return m_head; }
    const QVector<QPoint>& body() const { return m_body; }
    QPoint food() const { return m_food; }
    Direction direction() const { return m_direction; }

signals:
    void foodEaten();  // 吃到食物信号（供 UI 播放音效）

private:
    GameState  m_state;       // 当前游戏状态
    int        m_score;       // 当前分数
    int        m_highScore;   // 最高分

    // 难度相关参数
    Difficulty m_difficulty;     // 当前难度
    int        m_baseInterval;   // 基础速度间隔（随难度变化）
    int        m_speedStep;      // 每吃一个食物加速的毫秒数

    QPoint          m_head;       // 蛇头坐标
    QVector<QPoint> m_body;       // 蛇身体坐标列表（第0个紧挨着蛇头）
    Direction       m_direction;  // 当前移动方向
    Direction       m_prevDir;    // 上一帧方向（防止180度掉头）

    QPoint  m_food;        // 食物坐标

    // ---- 内部函数 ----
    void spawnFood();      // 生成新食物（不与蛇重叠）
    bool isOnSnake(const QPoint &p) const;  // 某点是否在蛇身上（含蛇头，给spawnFood用）
    bool hitsBody(const QPoint &p) const;   // 某点是否撞到身体（不含蛇头，给碰撞检测用）
    QPoint directionToDelta(Direction dir) const;  // 方向 -> 坐标增量
    bool isOpposite(Direction a, Direction b) const; // 两个方向是否相反
};

#endif // SNAKEGAME_H

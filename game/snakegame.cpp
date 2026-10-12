// ============================================================
// game/snakegame.cpp — 贪吃蛇核心逻辑实现
// ============================================================
#include "snakegame.h"
#include <QRandomGenerator>
#include <QSettings>
#include <QCoreApplication>
// ============================================================
// 构造函数: 初始化游戏状态
// ============================================================
SnakeGame::SnakeGame(QObject *parent)
    : QObject(parent)
    , m_state(STATE_READY)         // 游戏状态：就绪
    , m_score(0)                    // 当前得分
    , m_highScore(0)                // 历史最高分
    , m_difficulty(DIFF_NORMAL)     // 当前难度等级
    , m_baseInterval(BASE_INTERVAL) // 定时器基础间隔(ms)
    , m_speedStep(5)                // 每吃食物一次速度提升步长
    , m_direction(DIR_NONE)         // 当前移动方向
    , m_prevDir(DIR_NONE)           // 上一帧实际移动方向
{
    loadHighScore();
    reset();
}
// ============================================================
// reset() — 重置游戏到初始状态
// ============================================================
void SnakeGame::reset()
{
    m_state = STATE_READY;
    m_score = 0;
    m_direction = DIR_NONE;
    m_prevDir = DIR_NONE;
    // 蛇头放中央
    m_head = QPoint(GRID_WIDTH / 2, GRID_HEIGHT / 2);
    m_body.clear();  // 清空蛇身
    spawnFood();     // 生成第一个食物
}
// ============================================================
// 游戏控制: start / pause / resume / togglePause
// ============================================================
void SnakeGame::start()
{
    if (m_state == STATE_READY || m_state == STATE_GAMEOVER) {
        if (m_state == STATE_GAMEOVER) reset();
        m_state = STATE_PLAYING;
    }
}
void SnakeGame::pause()
{
    if (m_state == STATE_PLAYING) m_state = STATE_PAUSED;
}
void SnakeGame::resume()
{
    if (m_state == STATE_PAUSED) m_state = STATE_PLAYING;
}
void SnakeGame::togglePause()
{
    if (m_state == STATE_PLAYING) m_state = STATE_PAUSED;
    else if (m_state == STATE_PAUSED) m_state = STATE_PLAYING;
}
// ============================================================
// setDifficulty() — 设置难度，调整速度参数
// ============================================================
void SnakeGame::setDifficulty(Difficulty diff)
{
    m_difficulty = diff;
    switch (diff) {
    case DIFF_EASY:   m_baseInterval = 150; m_speedStep = 3; break;
    case DIFF_NORMAL: m_baseInterval = 100; m_speedStep = 5; break;
    case DIFF_HARD:   m_baseInterval = 70;  m_speedStep = 8; break;
    }
}
// ============================================================
// setDirection() — 设置移动方向
// 返回 true 表示设置成功，false 表示被忽略（反方向）
// ============================================================
bool SnakeGame::setDirection(Direction dir)
{
    // 静止状态下（READY），任何方向都接受
    if (m_prevDir != DIR_NONE && isOpposite(m_prevDir, dir)) {
        return false;  // 180度掉头，忽略
    }
    m_direction = dir;
    return true;
}
// ============================================================
// tick() — 执行一帧游戏逻辑
// 调用时机: 定时器每帧调用一次（仅当 STATE_PLAYING 时）
// ============================================================
void SnakeGame::tick()
{
    if (m_state != STATE_PLAYING) return;
    if (m_direction == DIR_NONE) return;  // 没方向就不动
    // ---- 1. 将旧蛇头加入身体链表头部 ----
    m_body.prepend(m_head);
    // ---- 2. 计算新蛇头位置 ----
    QPoint delta = directionToDelta(m_direction);
    m_head += delta;
    // ----3.穿墙逻辑 ----
    if (m_head.x() >= GRID_WIDTH)  m_head.setX(0);
    else if (m_head.x() < 0)       m_head.setX(GRID_WIDTH - 1);
    if (m_head.y() >= GRID_HEIGHT) m_head.setY(0);
    else if (m_head.y() < 0)       m_head.setY(GRID_HEIGHT - 1);
    // ----4. 判断是否吃到食物 ----
    bool eatFood = (m_head == m_food);
    if (eatFood)
    {
        m_score += SCORE_PER_FOOD; // 每颗食物获得分数
        spawnFood();
        emit foodEaten();
        //吃到食物，尾巴保留，不删除，蛇变长
    }
    else
    {
        //没吃到食物：先移除尾巴！尾巴消失之后再做碰撞检测！
        m_body.removeLast();
    }
    // ----5.现在再做碰撞检测！尾巴已经处理完毕 ----
    if (hitsBody(m_head))
    {
        m_state = STATE_GAMEOVER;
        if (m_score > m_highScore) {
            m_highScore = m_score;
            saveHighScore();
        }
        return;
    }
    //记录本帧方向
    m_prevDir = m_direction;
}

// ============================================================
// currentInterval() — 根据分数计算当前速度（毫秒）
// ============================================================
int SnakeGame::currentInterval() const
{
    int speedUps = m_score / SCORE_PER_FOOD; // 加速次数：已吃掉食物数量
    int interval = m_baseInterval - speedUps * m_speedStep;
    if (interval < MIN_INTERVAL) interval = MIN_INTERVAL; // 最小间隔下限，防止无限加速
    return interval;
}
// ============================================================
// spawnFood() — 随机生成食物，确保不在蛇身上
// ============================================================
void SnakeGame::spawnFood()
{
    do {
        m_food.setX(QRandomGenerator::global()->bounded(GRID_WIDTH));
        m_food.setY(QRandomGenerator::global()->bounded(GRID_HEIGHT));
    } while (isOnSnake(m_food));
}
// ============================================================
// isOnSnake() — 判断某点是否在蛇身上（含蛇头）
// 用途: spawnFood 用，确保食物不在蛇的任何部位
// ============================================================
bool SnakeGame::isOnSnake(const QPoint &p) const
{
    if (p == m_head) return true;
    for (const QPoint &seg : m_body) {
        if (seg == p) return true;
    }
    return false;
}
// ============================================================
// hitsBody() — 判断某点是否撞到蛇身（不含蛇头）
// 用途: 碰撞检测用，蛇头移动后检查是否撞到自己身体
// ============================================================
bool SnakeGame::hitsBody(const QPoint &p) const
{
    for (const QPoint &seg : m_body) {
        if (seg == p) return true;
    }
    return false;
}
// ============================================================
// directionToDelta() — 方向枚举转坐标增量
// ============================================================
QPoint SnakeGame::directionToDelta(Direction dir) const
{
    switch (dir) {
    case DIR_UP:    return QPoint(0, -1);
    case DIR_DOWN:  return QPoint(0,  1);
    case DIR_LEFT:  return QPoint(-1, 0);
    case DIR_RIGHT: return QPoint(1,  0);
    default:        return QPoint(0,  0);
    }
}
// ============================================================
// loadHighScore() — 从 QSettings 读取最高分
// ============================================================
void SnakeGame::loadHighScore()
{
    QSettings settings("YuanKang", "SnakeQt");
    m_highScore = settings.value("highScore", 0).toInt();
}
// ============================================================
// saveHighScore() — 保存最高分到 QSettings
// ============================================================
void SnakeGame::saveHighScore() const
{
    QSettings settings("YuanKang", "SnakeQt");
    settings.setValue("highScore", m_highScore);
}
// ============================================================
// isOpposite() — 判断两个方向是否相反
// ============================================================
bool SnakeGame::isOpposite(Direction a, Direction b) const
{
    return (a == DIR_UP    && b == DIR_DOWN)  ||
           (a == DIR_DOWN  && b == DIR_UP)    ||
           (a == DIR_LEFT  && b == DIR_RIGHT) ||
           (a == DIR_RIGHT && b == DIR_LEFT);
}

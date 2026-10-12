// ============================================================
// ai/snakeai.cpp — 每步 BFS 安全验证
//
// 策略:
//   每帧对 4 个方向尝试，选最优的:
//   1. 不撞蛇身（蛇尾位置可以走，因为尾巴下一帧会移走）
//   2. 走完这一步之后BFS能够追到蛇尾（防止把自己困死闭环）
//   3. 满足前面两个安全条件的方向集合里，选择BFS到食物距离最近方向
//
//   如果没有任何安全方向可以去吃食物:
//   4. BFS寻路，优先追逐蛇尾，保命优先
//   5. 兜底策略：flood‑fill洪水填充，选择可达空间最大方向苟活
//
// 安全保证:
//   每一步模拟未来一步局面，校验新蛇头是否可达蛇尾；
//   属于保守跟随尾巴策略，优先保证不死，其次吃食物。
//   ⚠️注意：仅校验下一步瞬间连通，不会做多步预判，长蛇吃食物后仍存在自我围困可能性。
// ============================================================
#include "snakeai.h"
#include <QQueue>
#include <cstring>

SnakeAI::SnakeAI() : m_active(false) {}

// ============================================================
// computeNextMove() — AI核心入口，每一帧调用，返回选出的移动方向
// 输入：game 游戏只读状态（蛇头、蛇身、食物、当前方向）
// 输出：Direction，AI决定下一步往哪个方向走
// ============================================================
Direction SnakeAI::computeNextMove(const SnakeGame &game)
{
    QPoint head = game.head();         // 当前蛇头坐标
    QPoint food = game.food();         // 当前食物坐标
    const QVector<QPoint> &body = game.body(); // 蛇身体数组，body.last()是尾巴
    int W = SnakeGame::GRID_WIDTH;
    int H = SnakeGame::GRID_HEIGHT;

    // dx,dy四个方向偏移：上、下、左、右
    static const int dx[] = {0, 0, -1, 1};
    static const int dy[] = {-1, 1, 0, 0};
    static const Direction dirs[] = {DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT};

    // ---- 收集所有安全方向候选 ----
    Direction bestDir = DIR_NONE;    // 最优方向，初始静止
    int bestFoodDist = 999999;       // 记录最优候选到食物距离，初始无穷大
    int bestSpace = -1;               // 记录最优候选的可达空格大小

    // 遍历上下左右4个方向逐个评估
    for (int i = 0; i < 4; i++)
    {
        Direction dir = dirs[i];
        // 禁止180度掉头，不能直接反向走
        if (game.direction() != DIR_NONE && isOpposite(game.direction(), dir))
            continue;

        // 计算往该方向走一步后的新蛇头坐标，取模实现穿墙
        int nx = (head.x() + dx[i] + W) % W;
        int ny = (head.y() + dy[i] + H) % H;
        QPoint next(nx, ny);

        // ---- 检查1：是否直接撞到蛇身体；尾巴允许踩，下一帧尾巴移开 ----
        bool isTail = !body.isEmpty() && next == body.last();
        bool hitBody = false;
        for (const QPoint &seg : body)
        {
            if (seg == next)
            {
                if (isTail) continue; // 如果是尾巴，允许踏入，不算碰撞
                hitBody = true;
                break;
            }
        }
        if (hitBody) continue; // 撞到普通身体片段，放弃这个方向

        // ---- 检查2：安全校验 isMoveSafe
        // 模拟走完这一步整个蛇的新状态，判断新蛇头是否还能够到达尾巴
        if (!isMoveSafe(game, next)) continue;

        // ---- 检查3：计算该候选方向下，新蛇头到食物BFS最短距离 ----
        // 构建【走一步之后】的障碍物虚拟地图，AI内部模拟，不修改真实游戏
        bool obstacle[SnakeGame::GRID_WIDTH][SnakeGame::GRID_HEIGHT];
        std::memset(obstacle, 0, sizeof(obstacle));

        bool eating = (next == food); // 判断这一步会不会吃到食物
        obstacle[head.x()][head.y()] = true; //旧蛇头变为身体片段，标记为障碍物
        // body[0] ~ body[size‑2]保留；最后尾巴要看是否吃食物决定是否保留
        for (int j = 0; j < body.size() - 1; j++)
        {
            obstacle[body[j].x()][body[j].y()] = true;
        }
        if (eating)
        {
            obstacle[body.last().x()][body.last().y()] = true; //吃到食物尾巴不移走，保留障碍物
        }

        // BFS寻路获取从新蛇头到食物路径
        QVector<QPoint> path = bfsPath(next, food, obstacle);
        int foodDist = path.isEmpty() ? 999999 : path.size() - 1;

        // FloodFill洪水填充：统计这个位置可以走到多少空格，作为次要排序条件
        int space = floodFill(next, obstacle);

        //择优逻辑：优先距离食物更近；距离相同选可用空间更大
        if (foodDist < bestFoodDist ||
            (foodDist == bestFoodDist && space > bestSpace))
        {
            bestFoodDist = foodDist;
            bestSpace = space;
            bestDir = dir;
        }
    }

    // 如果找到了合法安全候选方向，直接返回最优方向
    if (bestDir != DIR_NONE)
        return bestDir;

    // ----4. 没有可以安全走向食物的方向，降级策略：放弃吃食物，追逐尾巴保命 ----
    if (!body.isEmpty())
    {
        // 构造真实当前地图障碍物
        bool obstacle[SnakeGame::GRID_WIDTH][SnakeGame::GRID_HEIGHT];
        std::memset(obstacle, 0, sizeof(obstacle));
        for (const QPoint &p : body)
        {
            obstacle[p.x()][p.y()] = true;
        }
        obstacle[body.last().x()][body.last().y()] = false; //尾巴下一帧移走，取消障碍

        QPoint tail = body.last();
        QVector<QPoint> pathToTail = bfsPath(head, tail, obstacle);
        //路径至少两个点：起点+第一步，才有移动意义
        if (pathToTail.size() >= 2)
        {
            Direction dir = pointsToDirection(head, pathToTail[1]);
            //防止掉头
            if (!(game.direction() != DIR_NONE && isOpposite(game.direction(), dir)))
            {
                return dir;
            }
        }
    }

    // ----5.终极兜底策略：连追尾巴路径也没有，floodFill选空间最大方向尽量延缓死亡 ----
    bool obstacle[SnakeGame::GRID_WIDTH][SnakeGame::GRID_HEIGHT];
    std::memset(obstacle, 0, sizeof(obstacle));
    for (const QPoint &p : body)
    {
        obstacle[p.x()][p.y()] = true;
    }
    if (!body.isEmpty())
    {
        obstacle[body.last().x()][body.last().y()] = false;
    }
    return survivalMove(game, obstacle);
}

// ============================================================
// isMoveSafe() 安全校验函数
// 参数：game当前游戏状态；nextPos：假设下一步蛇头位置
// 返回true：走这一步是安全；false：不安全，会自我封闭
// 逻辑：模拟走完一步后的蛇身局面，看新蛇头是否可以BFS到达新尾巴
// ============================================================
bool SnakeAI::isMoveSafe(const SnakeGame &game, const QPoint &nextPos) const
{
    const QVector<QPoint> &body = game.body();
    if (body.isEmpty()) return true; //蛇没有身体，一定安全

    bool eating = (nextPos == game.food()); //是否这一步吃到食物

    //构建虚拟障碍物地图：模拟【走完这一步之后】蛇身体布局
    bool obstacle[SnakeGame::GRID_WIDTH][SnakeGame::GRID_HEIGHT];
    std::memset(obstacle, 0, sizeof(obstacle));
    obstacle[game.head().x()][game.head().y()] = true; //旧蛇头变成身体片段

    for (int i = 0; i < body.size() - 1; i++)
    {
        obstacle[body[i].x()][body[i].y()] = true;
    }
    if (eating)
    {
        obstacle[body.last().x()][body.last().y()] = true; //吃食物尾巴不动
    }

    //确定走完一步之后新尾巴坐标
    QPoint targetTail;
    if (eating)
    {
        targetTail = body.last();       //吃到食物尾巴不变
    }
    else
    {
        if (body.size() >= 2)
        {
            targetTail = body[body.size() - 2]; //不吃食物尾巴前移一格，原来last消失，新尾巴是倒数第二个
        }
        else
        {
            return true; //身体只剩一节，走完之后身体清空，安全
        }
    }

    obstacle[targetTail.x()][targetTail.y()] = false; //尾巴本身允许通行，不能当成墙
    //BFS连通检测：新蛇头能不能到达新尾巴
    return bfsCanReach(nextPos, targetTail, obstacle);
}

// ============================================================
// bfsCanReach()
// BFS广度优先搜索，只做连通判断，不需要完整路径
// start起始点，target目标点，obstacle障碍物地图
// 返回true：两点之间可以通行；false：被蛇身隔断走不通
// 支持穿墙，坐标取模W H
// ============================================================
bool SnakeAI::bfsCanReach(const QPoint &start, const QPoint &target,
                          bool obstacle[SnakeGame::GRID_WIDTH][SnakeGame::GRID_HEIGHT]) const
{
    if (start == target) return true;
    int W = SnakeGame::GRID_WIDTH;
    int H = SnakeGame::GRID_HEIGHT;

    bool visited[30][30]; //访问标记数组，标记BFS已经走过的格子
    std::memset(visited, 0, sizeof(visited));
    QQueue<QPoint> queue;
    queue.enqueue(start);
    visited[start.x()][start.y()] = true;

    static const int dx[] = {0, 0, -1, 1};
    static const int dy[] = {-1, 1, 0, 0};

    while (!queue.isEmpty())
    {
        QPoint cur = queue.dequeue();
        if (cur == target) return true; //找到目标，连通，直接返回true
        //遍历四个方向扩展
        for (int i = 0; i < 4; i++)
        {
            int nx = (cur.x() + dx[i] + W) % W;
            int ny = (cur.y() + dy[i] + H) % H;
            if (obstacle[nx][ny]) continue; //障碍物跳过
            if (visited[nx][ny]) continue;  //已经访问过跳过
            visited[nx][ny] = true;
            queue.enqueue(QPoint(nx, ny));
        }
    }
    return false; //队列走完没有找到目标，两点不连通
}

// ============================================================
// floodFill() 洪水填充算法
// start：起始坐标；obstacle障碍物地图
// 返回值：从start出发，一共可以到达多少个空格子数量
// 用途：评估某位置的生存空间大小，空间越大越安全
// ============================================================
int SnakeAI::floodFill(const QPoint &start,
                       bool obstacle[SnakeGame::GRID_WIDTH][SnakeGame::GRID_HEIGHT]) const
{
    if (obstacle[start.x()][start.y()]) return 0;
    int W = SnakeGame::GRID_WIDTH;
    int H = SnakeGame::GRID_HEIGHT;

    bool visited[30][30];
    std::memset(visited, 0, sizeof(visited));
    QQueue<QPoint> q;
    q.enqueue(start);
    visited[start.x()][start.y()] = true;
    int count = 0; //统计可达空格数目

    static const int dx[] = {0, 0, -1, 1};
    static const int dy[] = {-1, 1, 0, 0};

    while (!q.isEmpty())
    {
        QPoint cur = q.dequeue();
        count++;
        for (int i = 0; i < 4; i++)
        {
            int nx = (cur.x() + dx[i] + W) % W;
            int ny = (cur.y() + dy[i] + H) % H;
            if (!visited[nx][ny] && !obstacle[nx][ny])
            {
                visited[nx][ny] = true;
                q.enqueue(QPoint(nx, ny));
            }
        }
    }
    return count;
}

// ============================================================
// bfsPath() BFS广度优先寻路，返回完整路径点列表
// start起点，target终点，obstacle障碍物地图
// 返回QVector<QPoint>路径，path[0]起点，path.last终点；找不到返回空数组
// 实现回溯父节点方式重建路径；支持穿墙取模运算
// ============================================================
QVector<QPoint> SnakeAI::bfsPath(const QPoint &start, const QPoint &target,
                                 bool obstacle[SnakeGame::GRID_WIDTH][SnakeGame::GRID_HEIGHT]) const
{
    if (start == target) return QVector<QPoint>{start};
    int W = SnakeGame::GRID_WIDTH;
    int H = SnakeGame::GRID_HEIGHT;

    bool visited[30][30];          //标记是否访问
    int  parentX[30][30];          //记录每个格子父坐标x，用于回溯路径
    int  parentY[30][30];          //记录每个格子父坐标y，用于回溯路径
    std::memset(visited, 0, sizeof(visited));
    std::memset(parentX, -1, sizeof(parentX));
    std::memset(parentY, -1, sizeof(parentY));

    QQueue<QPoint> queue;
    queue.enqueue(start);
    visited[start.x()][start.y()] = true;

    static const int dx[] = {0, 0, -1, 1};
    static const int dy[] = {-1, 1, 0, 0};
    bool found = false;

    //BFS搜索过程
    while (!queue.isEmpty())
    {
        QPoint cur = queue.dequeue();
        if (cur == target)
        {
            found = true;
            break;
        }
        for (int i = 0; i < 4; i++)
        {
            int nx = (cur.x() + dx[i] + W) % W;
            int ny = (cur.y() + dy[i] + H) % H;
            if (obstacle[nx][ny]) continue;
            if (visited[nx][ny]) continue;
            visited[nx][ny] = true;
            parentX[nx][ny] = cur.x();
            parentY[nx][ny] = cur.y();
            queue.enqueue(QPoint(nx, ny));
        }
    }

    QVector<QPoint> path;
    if (found)
    {
        //从终点反向回溯到起点，再prepend向前插入构建完整路径
        int cx = target.x();
        int cy = target.y();
        while (cx != start.x() || cy != start.y())
        {
            path.prepend(QPoint(cx, cy));
            int px = parentX[cx][cy];
            int py = parentY[cx][cy];
            cx = px;
            cy = py;
        }
        path.prepend(start);
    }
    return path;
}

// ============================================================
// pointsToDirection()
// from 当前点；to 相邻下一个点（只走一步）
// 根据两点坐标差算出移动方向，兼容穿墙情况
// 穿墙场景：例如x=0向左走到x=29，dx=29，代表向左；x=29向右走到x=0，dx=-29代表向右
// ============================================================
Direction SnakeAI::pointsToDirection(const QPoint &from, const QPoint &to) const
{
    int dx = to.x() - from.x();
    int dy = to.y() - from.y();
    if (dx == 1 || dx == -(SnakeGame::GRID_WIDTH - 1))  return DIR_RIGHT;
    if (dx == -1 || dx == SnakeGame::GRID_WIDTH - 1)     return DIR_LEFT;
    if (dy == 1 || dy == -(SnakeGame::GRID_HEIGHT - 1))  return DIR_DOWN;
    if (dy == -1 || dy == SnakeGame::GRID_HEIGHT - 1)   return DIR_UP;
    return DIR_NONE;
}

// ============================================================
// survivalMove() 最后的生存策略
// 遍历4个合法方向，模拟走一步，floodFill求该方向可达空间，选择空间最大方向
// 没有目标食物、没有目标尾巴，纯粹最大化生存空间尽量晚死
// ============================================================
Direction SnakeAI::survivalMove(const SnakeGame &game,
                                 bool obstacle[SnakeGame::GRID_WIDTH][SnakeGame::GRID_HEIGHT]) const
{
    QPoint head = game.head();
    int W = SnakeGame::GRID_WIDTH;
    int H = SnakeGame::GRID_HEIGHT;
    Direction dirs[] = {DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT};
    Direction bestDir = DIR_NONE;
    int bestSpace = -1;

    for (int i = 0; i < 4; i++)
    {
        Direction dir = dirs[i];
        //禁止掉头
        if (game.direction() != DIR_NONE && isOpposite(game.direction(), dir))
            continue;
        int dx, dy;
        switch (dir)
        {
        case DIR_UP:    dx = 0;  dy = -1; break;
        case DIR_DOWN:  dx = 0;  dy =  1; break;
        case DIR_LEFT:  dx = -1; dy =  0; break;
        case DIR_RIGHT: dx = 1;  dy =  0; break;
        default: continue;
        }
        //穿墙计算新坐标
        int nx = (head.x() + dx + W) % W;
        int ny = (head.y() + dy + H) % H;
        if (obstacle[nx][ny]) continue;
        //计算该方向的可用空间大小
        int space = floodFill(QPoint(nx, ny), obstacle);
        if (space > bestSpace)
        {
            bestSpace = space;
            bestDir = dir;
        }
    }
    return bestDir;
}

// ============================================================
// isOpposite() 判断两个方向是否互为反向（上下相反，左右相反）
// ============================================================
bool SnakeAI::isOpposite(Direction a, Direction b) const
{
    return (a == DIR_UP    && b == DIR_DOWN)  ||
           (a == DIR_DOWN  && b == DIR_UP)    ||
           (a == DIR_LEFT  && b == DIR_RIGHT) ||
           (a == DIR_RIGHT && b == DIR_LEFT);
}

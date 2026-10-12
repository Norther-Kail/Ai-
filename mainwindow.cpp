// ============================================================
// mainwindow.cpp — 主窗口实现（所有游戏逻辑都在这里）
// ============================================================

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>        // 画笔: 绘制方块、文字
#include <QRandomGenerator> // 随机数生成器: 食物位置
#include <QFont>
#include <QPainterPath>

// ============================================================
// 构造函数: 初始化窗口、定时器、游戏数据
// ============================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);  // 加载 Qt 设计器生成的 UI

    // 计算并设置窗口大小:
    //   宽 = (20格 + 左围墙 + 右围墙) × 25px = 22 × 25 = 550px
    //   高 = (20格 + 上围墙 + 下围墙) × 25px + 40px底部分数栏 = 590px
    int w = (WIDTH + 2) * BLOCK_SIZE;
    int h = (HEIGHT + 2) * BLOCK_SIZE + 40;
    this->setFixedSize(w, h);  // 固定窗口大小，不允许拉伸
    this->setWindowTitle("贪吃蛇 - Snake Game");

    // 初始化游戏数据
    highScore = 0;
    state = STATE_READY;

    // 创建定时器: 每隔 currentInterval() 毫秒触发 gameLoop()
    // 这是整个游戏的心脏 — 定时器不停 -> 蛇不停
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::gameLoop);

    // 调用 setup() 把蛇、食物等重置到初始状态
    setup();
}

MainWindow::~MainWindow()
{
    delete ui;  // 释放 UI 资源
}

// ============================================================
// setup() — 初始化/重置游戏
// 把所有变量恢复到开局状态，按 R 时也调用此函数
// ============================================================
void MainWindow::setup()
{
    gameover = false;
    score = 0;

    // 蛇头放在正中央
    x = WIDTH / 2;
    y = HEIGHT / 2;

    tailLength = 0;  // 开局没有身体

    // 方向归零，蛇静止不动，等待玩家按键
    directionX = 0;
    directionY = 0;
    prevDirectionX = 0;
    prevDirectionY = 0;

    // 清空蛇身坐标数组，防止旧数据残留
    for (int i = 0; i < 100; i++) {
        tailX[i] = 0;
        tailY[i] = 0;
    }

    spawnFood();  // 生成第一个食物
}

// ============================================================
// spawnFood() — 随机生成食物位置
// 循环直到食物不在蛇头和蛇身上
// ============================================================
void MainWindow::spawnFood()
{
    bool onSnake;
    do {
        onSnake = false;
        // 随机生成坐标 (0 ~ WIDTH-1)
        fruitX = QRandomGenerator::global()->bounded(WIDTH);
        fruitY = QRandomGenerator::global()->bounded(HEIGHT);

        // 检查是否和蛇头重叠
        if (fruitX == x && fruitY == y) onSnake = true;

        // 检查是否和蛇身任何一段重叠
        for (int i = 0; i < tailLength; i++) {
            if (tailX[i] == fruitX && tailY[i] == fruitY) {
                onSnake = true;
                break;
            }
        }
    } while (onSnake);  // 如果重叠就重新生成
}

// ============================================================
// currentInterval() — 计算当前刷新速度
// 分数越高，间隔越短（蛇越快），最低不低于50ms
// ============================================================
int MainWindow::currentInterval()
{
    // 每得10分(吃一个食物) 快5ms
    int interval = BASE_INTERVAL - score / 10 * 5;
    if (interval < 50) interval = 50;  // 不能快于50ms
    return interval;
}

// ============================================================
// gameLoop() — 游戏主循环（定时器每帧调用）
// 流程: 逻辑更新 -> 调整速度 -> 触发重绘
// ============================================================
void MainWindow::gameLoop()
{
    // 如果不在游戏中，什么都不做
    if (state != STATE_PLAYING) return;

    logic();  // 执行一帧的游戏逻辑

    // 根据当前分数动态调整蛇的速度
    int interval = currentInterval();
    if (timer->interval() != interval) {
        timer->setInterval(interval);
    }

    update();  // 请求 Qt 重绘画面（触发 paintEvent）
}

// ============================================================
// logic() — 一帧的核心逻辑
// 1. 蛇身跟随: 每段移到前一段的位置
// 2. 蛇头移动: 按方向前进，穿墙
// 3. 碰撞检测: 撞自己则游戏结束
// 4. 吃食物: 碰到食物则加分、变长、刷新食物
// ============================================================
void MainWindow::logic()
{
    // ---- 1. 蛇身跟随 ----
    // 原理: 从第一段开始，每段取前一段的旧坐标
    // tailX[0] 先记下旧值，然后赋值为蛇头位置
    // 后续每段取前一段的旧值，依次传递
    int prevX = tailX[0];
    int prevY = tailY[0];
    int prev2X, prev2Y;
    tailX[0] = x;   // 第一段身体 = 蛇头旧位置
    tailY[0] = y;
    for (int i = 1; i < tailLength; i++) {
        prev2X = tailX[i];  // 暂存当前段
        prev2Y = tailY[i];
        tailX[i] = prevX;    // 当前段 = 前一段的旧值
        tailY[i] = prevY;
        prevX = prev2X;      // 更新"前一段旧值"供下一轮用
        prevY = prev2Y;
    }

    // ---- 2. 蛇头移动 ----
    x += directionX;  // 向左: x-1, 向右: x+1, 静止: x+0
    y += directionY;  // 向上: y-1, 向下: y+1, 静止: y+0

    // 穿墙: 从一边出去从另一边进来
    if (x >= WIDTH)  x = 0;
    else if (x < 0)  x = WIDTH - 1;
    if (y >= HEIGHT) y = 0;
    else if (y < 0)  y = HEIGHT - 1;

    // ---- 3. 碰撞检测: 撞自己则游戏结束 ----
    for (int i = 0; i < tailLength; i++) {
        if (tailX[i] == x && tailY[i] == y) {
            gameover = true;
            state = STATE_GAMEOVER;
            if (score > highScore) highScore = score;  // 更新最高分
            timer->stop();  // 停止定时器，蛇不再移动
            update();       // 刷新画面显示"游戏结束"
            return;
        }
    }

    // ---- 4. 吃到食物 ----
    if (x == fruitX && y == fruitY) {
        score += 10;        // 加分
        tailLength++;        // 蛇变长
        spawnFood();         // 生成新食物
    }

    // 记录本帧方向，下一帧用它来防止180度掉头
    prevDirectionX = directionX;
    prevDirectionY = directionY;
}

// ============================================================
// drawCenterText() — 在游戏区域居中绘制文字
// 参数: 画笔, 文字内容, Y轴偏移, 字号
// ============================================================
void MainWindow::drawCenterText(QPainter &p, const QString &text, int yOffset, int fontSize)
{
    QFont font("Microsoft YaHei", fontSize, QFont::Bold);
    p.setFont(font);
    p.setPen(Qt::white);
    // 计算文字宽度，使其水平居中
    int textWidth = p.fontMetrics().horizontalAdvance(text);
    int cx = (WIDTH + 2) * BLOCK_SIZE / 2;  // 窗口水平中点
    int cy = (HEIGHT + 2) * BLOCK_SIZE / 2 + yOffset;  // 垂直中点 + 偏移
    p.drawText(cx - textWidth / 2, cy, text);
}

// ============================================================
// paintEvent() — 绘制整个游戏画面
// Qt 每次调用 update() 时自动触发此函数
// 绘制顺序: 背景 -> 围墙 -> 蛇身 -> 蛇头(含眼睛) -> 食物 -> 分数 -> 状态文字
// ============================================================
void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);  // 开启抗锯齿，方块边缘更平滑

    // ---- 背景: 深蓝灰色 ----
    painter.fillRect(rect(), QColor(30, 30, 46));

    // ---- 围墙: 围绕游戏区域的深色方块边框 ----
    QColor wallColor(60, 60, 80);
    painter.setBrush(wallColor);
    painter.setPen(Qt::NoPen);  // 无描边
    // 画上、下围墙
    for (int i = 0; i < WIDTH + 2; i++) {
        painter.drawRoundedRect(i * BLOCK_SIZE, 0, BLOCK_SIZE, BLOCK_SIZE, 4, 4);
        painter.drawRoundedRect(i * BLOCK_SIZE, (HEIGHT + 1) * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE, 4, 4);
    }
    // 画左、右围墙
    for (int i = 0; i < HEIGHT + 2; i++) {
        painter.drawRoundedRect(0, i * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE, 4, 4);
        painter.drawRoundedRect((WIDTH + 1) * BLOCK_SIZE, i * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE, 4, 4);
    }

    // ---- 蛇身: 从尾巴向头部画，头部会覆盖在上层 ----
    // 偶数段和奇数段用不同深浅的绿色，产生条纹效果
    for (int k = tailLength - 1; k >= 0; k--) {
        QColor bodyColor = (k % 2 == 0) ? QColor(46, 204, 113) : QColor(39, 174, 96);
        painter.setBrush(bodyColor);
        // 坐标 +1 是因为要跳过左围墙的偏移
        painter.drawRoundedRect(
            (tailX[k] + 1) * BLOCK_SIZE + 1,
            (tailY[k] + 1) * BLOCK_SIZE + 1,
            BLOCK_SIZE - 2,    // -2 留1px间隙
            BLOCK_SIZE - 2,
            6, 6               // 圆角半径
        );
    }

    // ---- 蛇头: 比身体颜色更亮 ----
    QColor headColor(0, 200, 83);
    painter.setBrush(headColor);
    painter.drawRoundedRect(
        (x + 1) * BLOCK_SIZE + 1,
        (y + 1) * BLOCK_SIZE + 1,
        BLOCK_SIZE - 2,
        BLOCK_SIZE - 2,
        8, 8   // 蛇头圆角更大一点
    );

    // ---- 蛇头眼睛: 根据移动方向放在不同位置 ----
    painter.setBrush(Qt::white);
    int eyeSize = BLOCK_SIZE / 5;     // 眼睛大小
    int eyeOffset = BLOCK_SIZE / 4;  // 眼睛距方块边的偏移

    if (directionX == 1) { // 向右: 眼睛在右侧
        painter.drawEllipse((x + 1) * BLOCK_SIZE + eyeOffset * 2, (y + 1) * BLOCK_SIZE + eyeOffset, eyeSize, eyeSize);
        painter.drawEllipse((x + 1) * BLOCK_SIZE + eyeOffset * 2, (y + 1) * BLOCK_SIZE + eyeOffset * 2, eyeSize, eyeSize);
    } else if (directionX == -1) { // 向左: 眼睛在左侧
        painter.drawEllipse((x + 1) * BLOCK_SIZE + eyeOffset, (y + 1) * BLOCK_SIZE + eyeOffset, eyeSize, eyeSize);
        painter.drawEllipse((x + 1) * BLOCK_SIZE + eyeOffset, (y + 1) * BLOCK_SIZE + eyeOffset * 2, eyeSize, eyeSize);
    } else if (directionY == -1) { // 向上: 眼睛在上方
        painter.drawEllipse((x + 1) * BLOCK_SIZE + eyeOffset, (y + 1) * BLOCK_SIZE + eyeOffset, eyeSize, eyeSize);
        painter.drawEllipse((x + 1) * BLOCK_SIZE + eyeOffset * 2, (y + 1) * BLOCK_SIZE + eyeOffset, eyeSize, eyeSize);
    } else if (directionY == 1) { // 向下: 眼睛在下方
        painter.drawEllipse((x + 1) * BLOCK_SIZE + eyeOffset, (y + 1) * BLOCK_SIZE + eyeOffset * 2, eyeSize, eyeSize);
        painter.drawEllipse((x + 1) * BLOCK_SIZE + eyeOffset * 2, (y + 1) * BLOCK_SIZE + eyeOffset * 2, eyeSize, eyeSize);
    } else { // 静止: 默认放在上方
        painter.drawEllipse((x + 1) * BLOCK_SIZE + eyeOffset, (y + 1) * BLOCK_SIZE + eyeOffset, eyeSize, eyeSize);
        painter.drawEllipse((x + 1) * BLOCK_SIZE + eyeOffset * 2, (y + 1) * BLOCK_SIZE + eyeOffset, eyeSize, eyeSize);
    }

    // ---- 食物: 红色圆角方块 ----
    painter.setBrush(QColor(231, 76, 60));
    painter.drawRoundedRect(
        (fruitX + 1) * BLOCK_SIZE + 2,
        (fruitY + 1) * BLOCK_SIZE + 2,
        BLOCK_SIZE - 4,    // 比蛇身小一点
        BLOCK_SIZE - 4,
        BLOCK_SIZE / 3, BLOCK_SIZE / 3  // 大圆角，看起来像苹果
    );

    // ---- 底部分数栏 ----
    painter.setPen(Qt::white);
    QFont scoreFont("Consolas", 12, QFont::Bold);
    painter.setFont(scoreFont);
    int baseY = (HEIGHT + 2) * BLOCK_SIZE + 25;
    painter.drawText(15,  baseY, "Score: "  + QString::number(score));
    painter.drawText(150, baseY, "High: "   + QString::number(highScore));
    painter.drawText(280, baseY, "Length: " + QString::number(tailLength + 1));  // +1 = 算上蛇头

    // ---- 状态文字: 根据游戏状态显示不同提示 ----
    if (state == STATE_READY) {
        // 开机界面: 标题 + 操作说明
        drawCenterText(painter, "贪吃蛇", -20, 36);
        drawCenterText(painter, "按 方向键 或 WASD 开始", 25, 18);
        drawCenterText(painter, "P 暂停  |  R 重新开始  |  X 退出", 55, 14);
    } else if (state == STATE_PAUSED) {
        // 暂停: 半透明黑色遮罩 + 提示文字
        painter.fillRect(BLOCK_SIZE, BLOCK_SIZE, WIDTH * BLOCK_SIZE, HEIGHT * BLOCK_SIZE, QColor(0, 0, 0, 150));
        drawCenterText(painter, "已暂停", -10, 30);
        drawCenterText(painter, "按 P 继续", 30, 16);
    } else if (state == STATE_GAMEOVER) {
        // 游戏结束: 半透明遮罩 + 得分 + 重新开始提示
        painter.fillRect(BLOCK_SIZE, BLOCK_SIZE, WIDTH * BLOCK_SIZE, HEIGHT * BLOCK_SIZE, QColor(0, 0, 0, 150));
        drawCenterText(painter, "游戏结束", -25, 32);
        drawCenterText(painter, "得分: " + QString::number(score), 15, 20);
        drawCenterText(painter, "最高分: " + QString::number(highScore), 45, 18);
        drawCenterText(painter, "按 R 重新开始", 80, 16);
    }
}

// ============================================================
// keyPressEvent() — 键盘输入处理
// W/A/S/D 或方向键: 控制蛇移动
// P: 暂停/继续
// R: 重新开始
// X: 退出游戏
// ============================================================
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();

    // ---- R: 重新开始（任何时候都可以按）----
    if (key == Qt::Key_R) {
        setup();
        state = STATE_READY;
        update();
        return;
    }

    // ---- X: 退出 ----
    if (key == Qt::Key_X) {
        close();
        return;
    }

    // ---- P: 暂停 / 继续 ----
    if (key == Qt::Key_P) {
        if (state == STATE_PLAYING) {
            state = STATE_PAUSED;   // 游戏中 -> 暂停
            update();
        } else if (state == STATE_PAUSED) {
            state = STATE_PLAYING;  // 暂停 -> 继续
            update();
        }
        return;
    }

    // ---- 方向控制: 同时支持 WASD 和方向键 ----
    int newDX = directionX, newDY = directionY;
    bool pressed = false;

    switch (key) {
    case Qt::Key_A:
    case Qt::Key_Left:
        newDX = -1; newDY = 0; pressed = true; break;   // 左
    case Qt::Key_D:
    case Qt::Key_Right:
        newDX = 1;  newDY = 0; pressed = true; break;   // 右
    case Qt::Key_W:
    case Qt::Key_Up:
        newDX = 0;  newDY = -1; pressed = true; break;  // 上
    case Qt::Key_S:
    case Qt::Key_Down:
        newDX = 0;  newDY = 1;  pressed = true; break;  // 下
    default:
        break;
    }

    if (!pressed) return;  // 不是方向键就忽略

    // ---- 防止180度掉头 ----
    // 例如: 正在向右走，不能直接按左键
    // 判断: 如果新方向是旧方向的反方向，则忽略
    if (prevDirectionX == -newDX && prevDirectionY == -newDY
        && (prevDirectionX != 0 || prevDirectionY != 0)) {
        return;
    }

    // 应用新方向
    directionX = newDX;
    directionY = newDY;

    // ---- 如果在等待开始状态，按方向键即开始游戏 ----
    if (state == STATE_READY) {
        state = STATE_PLAYING;
        timer->start(currentInterval());  // 启动定时器
    }
}

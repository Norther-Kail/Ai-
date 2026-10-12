// ============================================================
// mainwindow.h — 主窗口类声明（头文件）
// 作用: 声明贪吃蛇游戏的所有变量和函数
// ============================================================

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>   // 主窗口基类
#include <QTimer>         // 定时器，驱动游戏循环
#include <QKeyEvent>      // 键盘事件
#include <QPainter>       // 画笔，用于绘制蛇、食物等

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }  // Qt 设计器自动生成的 UI 类
QT_END_NAMESPACE

// ---- 游戏状态枚举 ----
// 控制游戏在不同阶段的显示和行为
enum GameState {
    STATE_READY,    // 等待开始: 显示标题和操作说明，按方向键开始
    STATE_PLAYING,  // 游戏中:  蛇正常移动，定时器运行
    STATE_PAUSED,   // 暂停:    蛇停止移动，显示暂停遮罩
    STATE_GAMEOVER  // 游戏结束: 显示得分，按R重新开始
};

class MainWindow : public QMainWindow
{
    Q_OBJECT    // Qt 宏: 启用信号与槽机制

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // ---- 游戏常量 ----
    const int WIDTH = 20;          // 游戏区域格子数（宽）
    const int HEIGHT = 20;         // 游戏区域格子数（高）
    const int BLOCK_SIZE = 25;     // 每个格子的像素大小（25px）
    const int BASE_INTERVAL = 120; // 基础刷新间隔，越小越快（毫秒）

    // ---- 游戏状态变量 ----
    bool gameover;                 // 是否游戏结束
    int  score;                    // 当前得分
    int  highScore;                // 历史最高分

    // ---- 蛇的数据 ----
    int x, y;                      // 蛇头坐标（格子坐标，0~WIDTH-1）
    int fruitX, fruitY;            // 食物坐标
    int tailX[100], tailY[100];   // 蛇身每一段的坐标，最多100段
    int tailLength;                // 当前蛇身长度（不含蛇头）

    // ---- 方向控制 ----
    int directionX, directionY;         // 当前移动方向 (-1/0/1)
    int prevDirectionX, prevDirectionY; // 上一帧方向，防止180度掉头

    GameState state;               // 当前游戏状态
    QTimer *timer;                  // 定时器，每隔N毫秒触发一次 gameLoop()

    // ---- 核心函数 ----
    void setup();          // 初始化游戏，重置所有变量
    void logic();          // 游戏逻辑: 移动、碰撞检测、吃食物
    void spawnFood();      // 生成食物，确保不与蛇身重叠
    int  currentInterval(); // 根据分数计算刷新速度（越吃越快）

    // ---- 绘制与输入 ----
    void drawCenterText(QPainter &p, const QString &text, int yOffset = 0, int fontSize = 24);
    void paintEvent(QPaintEvent *event) override;  // Qt绘图事件: 画蛇、食物、围墙、分数
    void keyPressEvent(QKeyEvent *event) override;  // Qt键盘事件: WASD/方向键控制

private slots:
    void gameLoop();  // 定时器回调: 每帧调用 logic() + update()

private:
    Ui::MainWindow *ui;  // Qt 设计器生成的 UI 指针
};
#endif // MAINWINDOW_H

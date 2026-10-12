// ============================================================
// ui/mainwindow.h — 主窗口（UI 层）
// 作用: 只负责绘制画面和接收键盘输入
//       游戏逻辑全部委托给 SnakeGame 类
// ============================================================

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QKeyEvent>
#include <QPainter>
#include <QWheelEvent>
#include <QSoundEffect>
#include <QUrl>
#include "../game/snakegame.h"
#include "../ai/snakeai.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void onTimer();

private:
    Ui::MainWindow *ui;
    SnakeGame   *m_game;
    QTimer      *m_timer;
    int         m_blockSize;
    SnakeAI    *m_ai;
    bool        m_aiMode;
    bool        m_showChoice;  // 是否显示模式选择界面
    int         m_menuStep;    // 菜单步骤: 0=选模式, 1=选难度
    Difficulty  m_difficulty;
    QSoundEffect *m_eatSound;
    static const int MIN_BLOCK = 15;
    static const int MAX_BLOCK = 50;
    static const int DEFAULT_BLOCK = 25;
    static const int BOTTOM_BAR = 40;

    void applyWindowSize();

    void drawBackground(QPainter &p);
    void drawWall(QPainter &p);
    void drawSnake(QPainter &p);
    void drawFood(QPainter &p);
    void drawScoreBar(QPainter &p);
    void drawStateOverlay(QPainter &p);
    void drawChoiceScreen(QPainter &p);
    void drawMenuBox(QPainter &p, int x, int y, int w, int h,
                     const QString &num, const QString &title,
                     const QString &desc, const QColor &color);
    void generateEatSound();
    void drawCenterText(QPainter &p, const QString &text, int yOffset, int fontSize);

    QRect gridToPixel(int gridX, int gridY, int margin = 1) const;
};

#endif // MAINWINDOW_H

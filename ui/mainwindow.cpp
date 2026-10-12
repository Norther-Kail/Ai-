// ============================================================
// ui/mainwindow.cpp — 主窗口实现（纯 UI 层）
// 所有游戏逻辑通过 m_game 对象调用，这里只管画和输入
// ============================================================

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QFont>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QSoundEffect>
#include <QDir>
#include <QFile>
#include <QDataStream>
#include <QtMath>

// ============================================================
// 构造函数: 创建游戏对象 + 定时器
// ============================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_blockSize(DEFAULT_BLOCK)
    , m_ai(new SnakeAI())
    , m_aiMode(false)
    , m_showChoice(true)
    , m_menuStep(0)
    , m_difficulty(DIFF_NORMAL)
    , m_eatSound(new QSoundEffect(this))
{
    ui->setupUi(this);

    applyWindowSize();
    setWindowTitle("贪吃蛇 - Snake Game");

    // ---- 创建游戏核心对象 ----
    m_game = new SnakeGame(this);

    // ---- 创建定时器 ----
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::onTimer);

    // ---- 生成吃食物音效（延迟到事件循环，避免启动卡顿）----
    QTimer::singleShot(0, this, [this]() {
        generateEatSound();
        m_eatSound->setSource(QUrl::fromLocalFile(QDir::tempPath() + "/snake_eat.wav"));
        m_eatSound->setVolume(0.5f);
    });

    // ---- 吃到食物时播放音效 ----
    connect(m_game, &SnakeGame::foodEaten, this, [this]() {
        m_eatSound->play();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ============================================================
// applyWindowSize() — 根据 blockSize 重算窗口大小
// ============================================================
void MainWindow::applyWindowSize()
{
    int w = (SnakeGame::GRID_WIDTH  + 2) * m_blockSize;
    int h = (SnakeGame::GRID_HEIGHT + 2) * m_blockSize + BOTTOM_BAR;
    setFixedSize(w, h);
}

// ============================================================
// onTimer() — 每帧调用: 更新游戏逻辑 + 重绘画面
// ============================================================
void MainWindow::onTimer()
{
    // ---- AI 模式: 自动计算方向 ----
    if (m_aiMode && m_game->state() == STATE_PLAYING) {
        Direction dir = m_ai->computeNextMove(*m_game);
        m_game->setDirection(dir);
    }

    m_game->tick();

    int interval = m_game->currentInterval();
    if (m_timer->interval() != interval) {
        m_timer->setInterval(interval);
    }

    // ---- 游戏结束: 停止定时器 ----
    if (m_game->state() == STATE_GAMEOVER) {
        m_timer->stop();
    }

    update();
}

// ============================================================
// paintEvent() — 绘制整个画面
// 顺序: 背景+网格 → 围墙 → 食物 → 蛇 → 分数栏 → 状态遮罩
// ============================================================
void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_showChoice) {
        drawChoiceScreen(painter);
        return;
    }

    drawBackground(painter);
    drawWall(painter);
    drawFood(painter);
    drawSnake(painter);
    drawScoreBar(painter);
    drawStateOverlay(painter);
}

// ============================================================
// drawBackground() — 白色背景 + 浅灰网格线
// ============================================================
void MainWindow::drawBackground(QPainter &p)
{
    // 整体背景: 浅灰
    p.fillRect(rect(), QColor(240, 240, 240));

    // 游戏区域: 白色
    int gameW = SnakeGame::GRID_WIDTH  * m_blockSize;
    int gameH = SnakeGame::GRID_HEIGHT * m_blockSize;
    p.fillRect(m_blockSize, m_blockSize, gameW, gameH, Qt::white);

    // 网格线: 黑色实线
    QPen gridPen(QColor(0, 0, 0), 1, Qt::SolidLine);
    p.setPen(gridPen);
    p.setBrush(Qt::NoBrush);

    // 竖线
    for (int i = 0; i <= SnakeGame::GRID_WIDTH; i++) {
        int x = (i + 1) * m_blockSize;
        p.drawLine(x, m_blockSize, x, m_blockSize + gameH);
    }
    // 横线
    for (int i = 0; i <= SnakeGame::GRID_HEIGHT; i++) {
        int y = (i + 1) * m_blockSize;
        p.drawLine(m_blockSize, y, m_blockSize + gameW, y);
    }

    // 右上角署名 "袁康"（离墙有间距）
    QFont nameFont("Microsoft YaHei", 20, QFont::Bold);
    p.setFont(nameFont);
    p.setPen(QColor(80, 80, 100));
    int nameW = p.fontMetrics().horizontalAdvance("袁康");
    int nameH = p.fontMetrics().height();
    // 距右墙 15px，距上墙 15px
    p.drawText(m_blockSize + gameW - nameW - 70,
              m_blockSize + 15 + nameH, "袁康37");
}

// ============================================================
// drawWall() — 绘制围墙边框
// ============================================================
void MainWindow::drawWall(QPainter &p)
{
    QColor wallColor(0, 0, 0);
    p.setBrush(wallColor);
    p.setPen(Qt::NoPen);

    // 上、下围墙
    for (int i = 0; i < SnakeGame::GRID_WIDTH + 2; i++) {
        p.drawRoundedRect(i * m_blockSize, 0, m_blockSize, m_blockSize, 4, 4);
        p.drawRoundedRect(i * m_blockSize, (SnakeGame::GRID_HEIGHT + 1) * m_blockSize, m_blockSize, m_blockSize, 4, 4);
    }
    // 左、右围墙
    for (int i = 0; i < SnakeGame::GRID_HEIGHT + 2; i++) {
        p.drawRoundedRect(0, i * m_blockSize, m_blockSize, m_blockSize, 4, 4);
        p.drawRoundedRect((SnakeGame::GRID_WIDTH + 1) * m_blockSize, i * m_blockSize, m_blockSize, m_blockSize, 4, 4);
    }
}

// ============================================================
// drawSnake() — 绘制蛇
// 蛇身: 渐变色方块 + 内部高光
// 蛇头: 圆润方块 + 眼睛 + 瞳孔 + 舌头
// ============================================================
void MainWindow::drawSnake(QPainter &p)
{
    const QVector<QPoint> &body = m_game->body();
    int bs = m_blockSize;

    // ---- 蛇身: 从尾到头画 ----
    for (int i = body.size() - 1; i >= 0; i--) {
        // 颜色渐变: 尾部偏暗，靠近头部偏亮
        float ratio = (body.size() > 0) ? (float)i / body.size() : 0;
        int g1 = 174 + (int)((204 - 174) * (1 - ratio));
        int g2 = 96  + (int)((113 - 96)  * (1 - ratio));
        QColor bodyColor(46, g1, g2);

        QRect r = gridToPixel(body[i].x(), body[i].y(), 1);
        p.setBrush(bodyColor);
        p.setPen(QPen(QColor(30, 130, 70), 1));
        p.drawRoundedRect(r, bs / 5, bs / 5);

        // 内部高光: 左上角小亮点，增加立体感
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255, 25));
        p.drawRoundedRect(r.x() + 2, r.y() + 2, r.width() / 2, r.height() / 2, 3, 3);
    }

    // ---- 蛇头 ----
    QRect headRect = gridToPixel(m_game->head().x(), m_game->head().y(), 1);
    QRadialGradient headGrad(headRect.center(), headRect.width());
    headGrad.setColorAt(0, QColor(80, 230, 120));
    headGrad.setColorAt(1, QColor(0, 180, 70));
    p.setBrush(headGrad);
    p.setPen(QPen(QColor(20, 100, 50), 2));
    p.drawRoundedRect(headRect, bs / 4, bs / 4);

    // ---- 眼睛 + 瞳孔 ----
    int eyeSize = bs / 4;
    int eyeOffset = bs / 5;
    int pupilSize = eyeSize / 2;
    int hx = (m_game->head().x() + 1) * bs;
    int hy = (m_game->head().y() + 1) * bs;

    // 根据方向决定眼睛位置
    int eye1X, eye1Y, eye2X, eye2Y;
    Direction dir = m_game->direction();

    if (dir == DIR_RIGHT) {
        eye1X = hx + eyeOffset * 2; eye1Y = hy + eyeOffset;
        eye2X = hx + eyeOffset * 2; eye2Y = hy + eyeOffset * 2;
    } else if (dir == DIR_LEFT) {
        eye1X = hx + eyeOffset;     eye1Y = hy + eyeOffset;
        eye2X = hx + eyeOffset;     eye2Y = hy + eyeOffset * 2;
    } else if (dir == DIR_UP) {
        eye1X = hx + eyeOffset;     eye1Y = hy + eyeOffset;
        eye2X = hx + eyeOffset * 2; eye2Y = hy + eyeOffset;
    } else if (dir == DIR_DOWN) {
        eye1X = hx + eyeOffset;     eye1Y = hy + eyeOffset * 2;
        eye2X = hx + eyeOffset * 2; eye2Y = hy + eyeOffset * 2;
    } else {
        eye1X = hx + eyeOffset;     eye1Y = hy + eyeOffset;
        eye2X = hx + eyeOffset * 2; eye2Y = hy + eyeOffset;
    }

    // 眼白
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawEllipse(eye1X, eye1Y, eyeSize, eyeSize);
    p.drawEllipse(eye2X, eye2Y, eyeSize, eyeSize);

    // 瞳孔（黑色，朝方向偏移）
    int pdx = 0, pdy = 0;
    if (dir == DIR_RIGHT) pdx = pupilSize / 2;
    else if (dir == DIR_LEFT) pdx = -pupilSize / 2;
    else if (dir == DIR_UP) pdy = -pupilSize / 2;
    else if (dir == DIR_DOWN) pdy = pupilSize / 2;

    p.setBrush(QColor(20, 20, 30));
    p.drawEllipse(eye1X + (eyeSize - pupilSize) / 2 + pdx,
                  eye1Y + (eyeSize - pupilSize) / 2 + pdy,
                  pupilSize, pupilSize);
    p.drawEllipse(eye2X + (eyeSize - pupilSize) / 2 + pdx,
                  eye2Y + (eyeSize - pupilSize) / 2 + pdy,
                  pupilSize, pupilSize);
}

// ============================================================
// drawFood() — 绘制食物（苹果造型）
// 主体: 红色径向渐变球 + 高光
// 顶部: 棕色梗 + 绿色叶子
// ============================================================
void MainWindow::drawFood(QPainter &p)
{
    int bs = m_blockSize;
    QPoint food = m_game->food();
    int cx = (food.x() + 1) * bs + bs / 2;
    int cy = (food.y() + 1) * bs + bs / 2;
    int radius = bs / 2 - 2;

    // 裁剪到游戏区域内，防止梗和叶子溢出到墙上方
    int gameW = SnakeGame::GRID_WIDTH * bs;
    int gameH = SnakeGame::GRID_HEIGHT * bs;
    p.save();
    p.setClipRect(bs, bs, gameW, gameH);

    // ---- 苹果主体: 径向渐变 ----
    QRadialGradient grad(cx - radius / 3, cy - radius / 3, radius * 2);
    grad.setColorAt(0, QColor(255, 120, 100));
    grad.setColorAt(0.6, QColor(231, 76, 60));
    grad.setColorAt(1, QColor(160, 30, 20));
    p.setBrush(grad);
    p.setPen(QPen(QColor(120, 20, 10), 1));
    p.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2);

    // ---- 高光点 ----
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 80));
    p.drawEllipse(cx - radius / 2, cy - radius / 2, radius / 2, radius / 3);

    // ---- 棕色梗 ----
    p.setBrush(QColor(101, 67, 33));
    QRect stem(cx - 1, cy - radius - 2, 3, bs / 4);
    p.drawRect(stem);

    // ---- 绿色叶子 ----
    QPainterPath leaf;
    int leafX = cx + 2;
    int leafY = cy - radius;
    leaf.moveTo(leafX, leafY);
    leaf.quadTo(leafX + bs / 4, leafY - bs / 6, leafX + bs / 3, leafY + 2);
    leaf.quadTo(leafX + bs / 6, leafY + 3, leafX, leafY);
    p.setBrush(QColor(46, 174, 96));
    p.setPen(QPen(QColor(30, 130, 70), 1));
    p.drawPath(leaf);

    p.restore();
}

// ============================================================
// drawScoreBar() — 绘制底部分数信息
// ============================================================
void MainWindow::drawScoreBar(QPainter &p)
{
    int baseY = (SnakeGame::GRID_HEIGHT + 2) * m_blockSize + 25;
    QFont scoreFont("Consolas", 9, QFont::Bold);
    p.setFont(scoreFont);
    QFontMetrics fm(scoreFont); // 获取当前字体度量，用来算每个字符串宽度

    int x = 10;
    int gap = 20; //各个项目之间的间隔像素

    // 1.Score
    QString scoreText = "Score:" + QString::number(m_game->score());
    p.setPen(QColor(60, 60, 80));
    p.drawText(x, baseY, scoreText);
    x += fm.horizontalAdvance(scoreText) + gap;

    // 2.High
    QString highText = "High:" + QString::number(m_game->highScore());
    p.drawText(x, baseY, highText);
    x += fm.horizontalAdvance(highText) + gap;

    //3.Len
    QString lenText = "Len:" + QString::number(m_game->snakeLength());
    p.drawText(x, baseY, lenText);
    x += fm.horizontalAdvance(lenText) + gap;

    //4 AI状态
    QString aiStr = m_aiMode ? "AI:ON" : "AI:OFF";
    QString aiText = "[" + aiStr + "]";
    QColor aiColor = m_aiMode ? QColor(0, 150, 0) : QColor(150, 150, 150);
    p.setPen(aiColor);
    p.drawText(x, baseY, aiText);
    x += fm.horizontalAdvance(aiText) + gap;

    //5难度
    QString diffStr;
    QColor diffColor;
    switch (m_difficulty) {
    case DIFF_EASY:   diffStr = "简单"; diffColor = QColor(46, 204, 113); break;
    case DIFF_NORMAL: diffStr = "普通"; diffColor = QColor(241, 196, 15); break;
    case DIFF_HARD:   diffStr = "困难"; diffColor = QColor(231, 76, 60); break;
    }
    QString diffText = "[" + diffStr + "]";
    p.setPen(diffColor);
    p.drawText(x, baseY, diffText);

    //最右侧：缩放提示，单独设置小字体，靠右绘制，保持不动
    QFont hintFont("Consolas", 8);
    p.setFont(hintFont);
    QFontMetrics fmHint(hintFont);
    QString zoom = "+/- zoom:" + QString::number(m_blockSize);
    int zw = fmHint.horizontalAdvance(zoom);
    p.setPen(QColor(120, 120, 130));
    p.drawText(width() - zw - 10, baseY, zoom);
}


// ============================================================
// generateEatSound() — 生成吃食物音效 WAV 文件
// 600→1200Hz 上扫，80ms，带衰减包络
// ============================================================
void MainWindow::generateEatSound()
{
    QString path = QDir::tempPath() + "/snake_eat.wav";
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return;

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    const quint32 sampleRate = 44100;
    const quint16 numChannels = 1;
    const quint16 bitsPerSample = 16;
    const quint32 durationMs = 80;
    const quint32 numSamples = sampleRate * durationMs / 1000;
    const quint32 dataSize = numSamples * 2;
    const quint32 riffSize = 36 + dataSize;

    stream.writeRawData("RIFF", 4);
    stream << riffSize;
    stream.writeRawData("WAVE", 4);
    stream.writeRawData("fmt ", 4);
    stream << (quint32)16;
    stream << (quint16)1;
    stream << numChannels;
    stream << sampleRate;
    stream << sampleRate * numChannels * 2;
    stream << (quint16)(numChannels * 2);
    stream << bitsPerSample;
    stream.writeRawData("data", 4);
    stream << dataSize;

    double durSec = durationMs / 1000.0;
    for (quint32 i = 0; i < numSamples; i++) {
        double t = (double)i / sampleRate;
        double progress = t / durSec;
        double freq = 600.0 + 600.0 * progress;
        double env = exp(-3.0 * progress);
        if (progress < 0.05) env = progress / 0.05;
        qint16 sample = (qint16)(0.3 * 32767.0 * env * qSin(2.0 * M_PI * freq * t));
        stream << sample;
    }
    file.close();
}

// ============================================================
// drawMenuBox() — 绘制菜单选项卡片
// ============================================================
void MainWindow::drawMenuBox(QPainter &p, int x, int y, int w, int h,
                              const QString &num, const QString &title,
                              const QString &desc, const QColor &color)
{
    // 卡片背景
    p.setBrush(QColor(35, 35, 55, 200));
    p.setPen(QPen(color, 2));
    p.drawRoundedRect(x, y, w, h, 12, 12);

    // 数字圆圈（垂直居中）
    int circleSize = 28;
    int circleY = y + (h - circleSize) / 2;
    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawEllipse(x + 12, circleY, circleSize, circleSize);

    QRect numRect(x + 12, circleY, circleSize, circleSize);
    QFont numFont("Consolas", 12, QFont::Bold);
    p.setFont(numFont);
    p.setPen(QColor(255, 255, 255));
    p.drawText(numRect, Qt::AlignCenter, num);

    // 文字区域: 从圆圈右侧到卡片右边
    int textX = x + 52;
    int textW = w - 60;

    // 标题（框内上半部分，垂直居中）
    QRect titleRect(textX, y + 4, textW, h / 2 - 4);
    QFont titleFont("Microsoft YaHei", 12, QFont::Bold);
    p.setFont(titleFont);
    p.setPen(color);
    p.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, title);

    // 描述（框内下半部分，垂直居中）
    QRect descRect(textX, y + h / 2, textW, h / 2 - 4);
    QFont descFont("Microsoft YaHei", 9);
    p.setFont(descFont);
    p.setPen(QColor(150, 150, 170));
    p.drawText(descRect, Qt::AlignLeft | Qt::AlignVCenter, desc);
}

// ============================================================
// drawChoiceScreen() — 模式选择 + 难度选择界面
// ============================================================
void MainWindow::drawChoiceScreen(QPainter &p)
{
    // 渐变背景
    QLinearGradient bg(0, 0, 0, height());
    bg.setColorAt(0, QColor(26, 26, 46));
    bg.setColorAt(1, QColor(15, 15, 30));
    p.fillRect(rect(), bg);

    int cx = width() / 2;

    // ---- 标题: 贪吃蛇 ----
    QFont titleFont("Microsoft YaHei", 42, QFont::Bold);
    p.setFont(titleFont);
    p.setPen(QColor(0, 0, 0, 100));
    QString title = "贪吃蛇";
    int tw = p.fontMetrics().horizontalAdvance(title);
    p.drawText(cx - tw / 2 + 2, 182, title);
    p.setPen(QColor(46, 204, 113));
    p.drawText(cx - tw / 2, 180, title);

    // 英文副标题
    QFont subFont("Consolas", 12);
    p.setFont(subFont);
    p.setPen(QColor(80, 80, 100));
    QString sub = "S N A K E   G A M E";
    int sw = p.fontMetrics().horizontalAdvance(sub);
    p.drawText(cx - sw / 2, 200, sub);

    if (m_menuStep == 0) {
        // ---- 步骤 1: 选模式 ----
        QFont stepFont("Microsoft YaHei", 14, QFont::Bold);
        p.setFont(stepFont);
        p.setPen(QColor(180, 180, 200));
        QString step = "请选择游戏模式";
        int stw = p.fontMetrics().horizontalAdvance(step);
        p.drawText(cx - stw / 2, 240, step);

        int boxW = 340;
        int boxH = 55;
        int boxX = cx - boxW / 2;

        drawMenuBox(p, boxX, 270, boxW, boxH, "1",
                    "AI 自动模式", "蛇自己跑，看 AI 表演", QColor(52, 152, 219));
        drawMenuBox(p, boxX, 355, boxW, boxH, "2",
                    "手动操作模式", "自己控制，挑战极限", QColor(231, 76, 60));

        QFont hintFont("Microsoft YaHei", 10);
        p.setFont(hintFont);
        p.setPen(QColor(100, 100, 120));
        QString hint = "按数字键选择    X 退出";
        int hw = p.fontMetrics().horizontalAdvance(hint);
        p.drawText(cx - hw / 2, height() - 50, hint);

    } else if (m_menuStep == 1) {
        // ---- 步骤 2: 选难度 ----
        QFont stepFont("Microsoft YaHei", 14, QFont::Bold);
        p.setFont(stepFont);
        p.setPen(QColor(180, 180, 200));
        QString step = "请选择难度";
        int stw = p.fontMetrics().horizontalAdvance(step);
        p.drawText(cx - stw / 2, 230, step);

        int boxW = 340;
        int boxH = 55;
        int boxX = cx - boxW / 2;

        drawMenuBox(p, boxX, 260, boxW, boxH, "1",
                    "简单", "慢速 150ms · 适合新手", QColor(46, 204, 113));
        drawMenuBox(p, boxX, 335, boxW, boxH, "2",
                    "普通", "中速 100ms · 平衡挑战", QColor(241, 196, 15));
        drawMenuBox(p, boxX, 410, boxW, boxH, "3",
                    "困难", "快速 70ms · 极限挑战", QColor(231, 76, 60));

        QFont hintFont("Microsoft YaHei", 10);
        p.setFont(hintFont);
        p.setPen(QColor(100, 100, 120));
        QString hint = "按数字键选择    ESC 返回    X 退出";
        int hw = p.fontMetrics().horizontalAdvance(hint);
        p.drawText(cx - hw / 2, height() - 50, hint);
    }
}

// ============================================================
// drawStateOverlay() — 根据游戏状态绘制遮罩和文字
// ============================================================
void MainWindow::drawStateOverlay(QPainter &p)
{
    GameState state = m_game->state();

    if (state == STATE_READY) {
        drawCenterText(p, "贪吃蛇", -10, 36);
        drawCenterText(p, "按 方向键 或 WASD 开始", 35, 18);
        drawCenterText(p, "P 暂停  |  R 重新开始  |  X 退出  |  F 开启AI", 65, 14);
    } else if (state == STATE_PAUSED) {
        drawCenterText(p, "已暂停", -10, 30);
        drawCenterText(p, "按 P 继续", 30, 16);
    } else if (state == STATE_GAMEOVER) {
        drawCenterText(p, "游戏结束", -25, 32);
        drawCenterText(p, "得分: " + QString::number(m_game->score()), 15, 20);
        drawCenterText(p, "最高分: " + QString::number(m_game->highScore()), 45, 18);
        drawCenterText(p, "按 R 重新开始", 80, 16);
    }
}

// ============================================================
// drawCenterText() — 在游戏区域居中绘制文字
// ============================================================
void MainWindow::drawCenterText(QPainter &p, const QString &text, int yOffset, int fontSize)
{
    QFont font("Microsoft YaHei", fontSize, QFont::Bold);
    p.setFont(font);
    p.setPen(QColor(50, 50, 70));
    int textWidth = p.fontMetrics().horizontalAdvance(text);
    int cx = (SnakeGame::GRID_WIDTH + 2) * m_blockSize / 2;
    int cy = (SnakeGame::GRID_HEIGHT + 2) * m_blockSize / 2 + yOffset;
    p.drawText(cx - textWidth / 2, cy, text);
}

// ============================================================
// gridToPixel() — 格子坐标转像素矩形
// ============================================================
QRect MainWindow::gridToPixel(int gridX, int gridY, int margin) const
{
    return QRect(
        (gridX + 1) * m_blockSize + margin,
        (gridY + 1) * m_blockSize + margin,
        m_blockSize - margin * 2,
        m_blockSize - margin * 2
    );
}

// ============================================================
// wheelEvent() — 鼠标滚轮缩放
// ============================================================
void MainWindow::wheelEvent(QWheelEvent *event)
{
    int delta = event->angleDelta().y();
    if (delta > 0 && m_blockSize < MAX_BLOCK) {
        m_blockSize += 5;
        applyWindowSize();
        update();
    } else if (delta < 0 && m_blockSize > MIN_BLOCK) {
        m_blockSize -= 5;
        applyWindowSize();
        update();
    }
}

// ============================================================
// keyPressEvent() — 键盘输入处理
// ============================================================
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();

    // ---- X: 退出 ----
    if (key == Qt::Key_X) {
        close();
        return;
    }

    // ---- 选择界面 ----
    if (m_showChoice) {
        if (m_menuStep == 0) {
            // 步骤 1: 选模式
            if (key == Qt::Key_1) {
                m_aiMode = true;
                m_ai->setActive(true);
                m_menuStep = 1;
                update();
            } else if (key == Qt::Key_2) {
                m_aiMode = false;
                m_ai->setActive(false);
                m_menuStep = 1;
                update();
            }
        } else if (m_menuStep == 1) {
            // 步骤 2: 选难度
            if (key == Qt::Key_1) {
                m_difficulty = DIFF_EASY;
                m_game->setDifficulty(DIFF_EASY);
                m_showChoice = false;
                m_game->start();
                m_timer->start(m_game->currentInterval());
                update();
            } else if (key == Qt::Key_2) {
                m_difficulty = DIFF_NORMAL;
                m_game->setDifficulty(DIFF_NORMAL);
                m_showChoice = false;
                m_game->start();
                m_timer->start(m_game->currentInterval());
                update();
            } else if (key == Qt::Key_3) {
                m_difficulty = DIFF_HARD;
                m_game->setDifficulty(DIFF_HARD);
                m_showChoice = false;
                m_game->start();
                m_timer->start(m_game->currentInterval());
                update();
            } else if (key == Qt::Key_Escape) {
                m_menuStep = 0;
                update();
            }
        }
        return;
    }

    // ---- R: 重新开始 ----
    if (key == Qt::Key_R) {
        m_game->reset();
        m_timer->stop();
        if (m_aiMode) {
            m_game->start();
            m_timer->start(m_game->currentInterval());
        }
        update();
        return;
    }

    // ---- F: 切换 AI 自动模式 ----
    if (key == Qt::Key_F) {
        m_aiMode = !m_aiMode;
        m_ai->setActive(m_aiMode);
        if (m_aiMode) {
            if (m_game->state() == STATE_READY || m_game->state() == STATE_GAMEOVER) {
                if (m_game->state() == STATE_GAMEOVER) m_game->reset();
                m_game->start();
                m_timer->start(m_game->currentInterval());
            }
        }
        update();
        return;
    }

    // ---- P: 暂停/继续 ----
    if (key == Qt::Key_P) {
        if (m_game->state() == STATE_PLAYING) {
            m_game->pause();
        } else if (m_game->state() == STATE_PAUSED) {
            m_game->resume();
        }
        update();
        return;
    }

    // ---- + / = 放大, - 缩小 ----
    if (key == Qt::Key_Plus || key == Qt::Key_Equal) {
        if (m_blockSize < MAX_BLOCK) {
            m_blockSize += 5;
            applyWindowSize();
            update();
        }
        return;
    }
    if (key == Qt::Key_Minus) {
        if (m_blockSize > MIN_BLOCK) {
            m_blockSize -= 5;
            applyWindowSize();
            update();
        }
        return;
    }

    // ---- 方向键 ----
    Direction dir = DIR_NONE;
    switch (key) {
    case Qt::Key_W:
    case Qt::Key_Up:    dir = DIR_UP;    break;
    case Qt::Key_S:
    case Qt::Key_Down:  dir = DIR_DOWN;  break;
    case Qt::Key_A:
    case Qt::Key_Left:  dir = DIR_LEFT;  break;
    case Qt::Key_D:
    case Qt::Key_Right: dir = DIR_RIGHT; break;
    default:
        return;
    }

    if (!m_game->setDirection(dir)) return;

    if (m_game->state() == STATE_READY) {
        m_game->start();
        m_timer->start(m_game->currentInterval());
    }
}

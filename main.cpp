// ============================================================
// main.cpp — 程序入口
// 作用: 创建 QApplication 和 MainWindow，启动事件循环
// ============================================================

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // 创建 Qt 应用程序对象，处理命令行参数
    QApplication a(argc, argv);

    // 创建主窗口（贪吃蛇游戏窗口）
    MainWindow w;

    // 显示窗口
    w.show();

    // 进入 Qt 事件循环，等待用户操作（键盘、鼠标等）
    // 直到窗口关闭才返回
    return a.exec();
}

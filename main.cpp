#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect  screenGeometry = screen->geometry();
    w.move(screenGeometry.center() - w.rect().center());

    w.show();
    //w.showMaximized();
    return a.exec();
}

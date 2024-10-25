#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QString addr = "";
    bool debug_console = false;

    //if (argc == 2)
    //    addr = QString(argv[1]);

    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "debug"))
            debug_console = true;
        else
            addr = QString(argv[i]);
    }

#ifdef _WIN32
    if (!debug_console)
    {
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
        }
    }
    else
    {
        if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
        }
    }
#endif

    MainWindow w(addr, nullptr);

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect  screenGeometry = screen->geometry();
    w.move(screenGeometry.center() - w.rect().center());

    w.show();
    //w.showMaximized();
    return a.exec();
}

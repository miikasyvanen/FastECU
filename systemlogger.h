#ifndef SYSTEMLOGGER_H
#define SYSTEMLOGGER_H

#include <QApplication>
#include <QDebug>
#include <QTime>

class SystemLogger : public QObject
{
    Q_OBJECT

public:
    SystemLogger();
    ~SystemLogger();

    void run();
    void delay(int timeout);

private:
    enum {
        _LOG_E = 0,   // error
        _LOG_W,   // warning
        _LOG_I,   // info
        _LOG_D,   // debug
    } ;

signals:
    void logger();

private slots:
    void logMessages(int logType, QString message, bool timestamp, bool linefeed);
};

#endif // SYSTEMLOGGER_H

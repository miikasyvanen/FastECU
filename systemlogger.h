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
        LOGE = 0,   // error
        LOGW,   // warning
        LOGI,   // info
        LOGD,   // debug
    };

signals:
    void logger();

private slots:
    void logMessages(int logType, QString message, bool timestamp, bool linefeed);
};

#endif // SYSTEMLOGGER_H

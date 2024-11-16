#ifndef SYSTEMLOGGER_H
#define SYSTEMLOGGER_H

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QMetaMethod>
#include <QTime>

class SystemLogger : public QObject
{
    Q_OBJECT

public:
    SystemLogger(QString file_path, QString sw_name, QString sw_ver);
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

    QString file_path;
    QString software_name;
    QString software_version;

    bool write_syslog_to_file = false;
    bool syslog_file_open = false;
    bool syslog_file_init_ready = false;

    QFile syslog_file;
    QTextStream syslog_file_outstream;

    bool write_syslog(QString msg);

signals:
    void sendMsgToLogWindow(QString msg);
    void finished();
    void error(QString err);

private slots:
    void enable_log_write_to_file(bool enable);
    void logMessages(QString message, bool timestamp, bool linefeed);
};

#endif // SYSTEMLOGGER_H

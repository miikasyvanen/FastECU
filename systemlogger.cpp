#include "systemlogger.h"

SystemLogger::SystemLogger()
{

}

SystemLogger::~SystemLogger()
{

}

void SystemLogger::run()
{
    qDebug() << "SystemLogger started...";
    delay(1000);
}

void SystemLogger::logMessages(int logType, QString message, bool timestamp, bool linefeed)
{
    QString msg;

    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz'] ");

    // Check if timestamp added
    if (timestamp)
        msg += dateTimeString;

    // Check log type

    if (logType == 0)
        msg += "(EE) ";
    else if (logType == 1)
        msg += "(WW) ";
    else if (logType == 2)
        msg += "(II) ";
    else if (logType == 3)
        msg += "(DD) ";
    msg += message;

    // Check if linefeed added
    if (linefeed)
        msg += "\n";

    qDebug() << msg;

}





void SystemLogger::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

#include "systemlogger.h"

SystemLogger::SystemLogger(QString file_path, QString sw_name, QString sw_ver)
{
    this->file_path = file_path;
    this->software_name = sw_name;
    this->software_version = sw_ver;


}

SystemLogger::~SystemLogger()
{

}

void SystemLogger::run()
{
    qDebug() << "SystemLogger started...";
    delay(1000);
}

void SystemLogger::logMessages(int logType, bool write_syslog_to_file, QString message, bool timestamp, bool linefeed)
{
    QString msg;

    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz'] ");

    // Check if timestamp added
    if (timestamp)
        msg += dateTimeString;

    // Check log type
    if (logType == _LOG_E)
        msg += "(EE) ";
    else if (logType == _LOG_W)
        msg += "(WW) ";
    else if (logType == _LOG_I)
        msg += "(II) ";
    else if (logType == _LOG_D)
        msg += "(DD) ";
    else
        qDebug() << "Wrong log type!";

    msg += message;

    qDebug() << msg;

    // Check if linefeed added
    if (linefeed)
        msg += "\n";

    if(write_syslog_to_file)
        write_syslog(msg);
}

bool SystemLogger::write_syslog(QString msg)
{
    if (!syslog_file_open)
    {
        QDateTime dateTime = dateTime.currentDateTime();
        QString dateTimeString = dateTime.toString("yyyy-MM-dd hh':'mm':'ss'.'zzz'");

        QString syslog_file_name = file_path;
        if (file_path.at(file_path.length() - 1) != '/')
            syslog_file_name.append("/");
        syslog_file_name.append("log_fastecu_" + dateTimeString + ".txt");

        syslog_file.setFileName(syslog_file_name);
        if (!syslog_file.open(QIODevice::WriteOnly)) {
            //QMessageBox::information(this, tr("Unable to open file"), syslog_file.errorString() + ":\n" + syslog_file_name);
            return false;
        }
        else
            syslog_file_open = true;

        syslog_file_outstream.setDevice(&syslog_file);
        syslog_file_outstream << software_name + " v" + software_version + ", system log file, start time: " + dateTimeString;
        syslog_file_outstream << "\n";

    }
    if (syslog_file_open)
    {
        syslog_file_outstream << msg;
        syslog_file_outstream.flush();
    }
    return true;
}




void SystemLogger::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

#include "systemlogger.h"

SystemLogger::SystemLogger(QString file_path, QString software_name, QString software_version, QObject *parent)
    : QObject(parent),
    file_path(file_path),
    software_name(software_name),
    software_version(software_version)
{
}

SystemLogger::~SystemLogger()
{
    if (syslog_file_open)
        syslog_file.close();
}

void SystemLogger::run()
{
    qDebug() << "SystemLogger started...";
    delay(1000);
}

void SystemLogger::enable_log_write_to_file(bool enable)
{
    write_syslog_to_file = enable;
}

void SystemLogger::log_messages(QString message, bool timestamp, bool linefeed)
{
    QString msg;

    QDateTime dateTime = dateTime.currentDateTime();

    QString dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz'] ");
    QMetaMethod metaMethod;

    if (sender())
        metaMethod = sender()->metaObject()->method(senderSignalIndex());
    else
        return;

    //qDebug() << "metaMethod.name:" << metaMethod.name();

    // Check if timestamp added
    if (timestamp)
        msg += dateTimeString;

    // Check log type
    if (metaMethod.name() == "LOG_E")
        msg += "(EE) ";
    else if (metaMethod.name() == "LOG_W")
        msg += "(WW) ";
    else if (metaMethod.name() == "LOG_I")
        msg += "(II) ";
    else if (metaMethod.name() == "LOG_D")
        msg += "(DD) ";

    msg += message;

    qDebug() << msg;

    // Check if linefeed added
    if (linefeed)
        msg += "\n";

    if (metaMethod.name() != "LOG_D")
        emit send_message_to_log_window(msg);

    if(write_syslog_to_file)
        write_syslog(msg);
}

bool SystemLogger::write_syslog(QString msg)
{
    //Open file for writing if needed
    if (!syslog_file_open)
    {
        QDateTime dateTime = dateTime.currentDateTime();
        QString dateTimeString = dateTime.toString("yyyy-MM-dd_hh'h'mm'm'ss's'");

        QString syslog_file_name = file_path;
        if (file_path.at(file_path.length() - 1) != '/')
            syslog_file_name.append("/");
        syslog_file_name.append("log_fastecu_" + dateTimeString + ".txt");

        syslog_file.setFileName(syslog_file_name);

        qDebug() << "Create logfile: " << syslog_file_name;
        if (!syslog_file.open(QIODevice::WriteOnly))
        {
            qDebug() << "Cannot open log file for writing";
            qDebug() << syslog_file.errorString() + ": " + syslog_file.fileName();
            return false;
        }

        syslog_file_open = true;
        syslog_file_init_ready = true;
        syslog_file_outstream.setDevice(&syslog_file);
        syslog_file_outstream << software_name + " v" + software_version + ", system log file, start time: " + dateTimeString;
        syslog_file_outstream << "\n";
    }

    syslog_file_outstream << msg;
    syslog_file_outstream.flush();

    return true;
}

void SystemLogger::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

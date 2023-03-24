#include "ecu_operations_mercedes.h"
#include <ui_ecu_operations.h>

EcuOperationsMercedes::EcuOperationsMercedes(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::EcuOperationsWindow)
{
    ui->setupUi(this);
    this->setParent(parent);

    if (cmd_type == "test_write")
        this->setWindowTitle("Test write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "write")
        this->setWindowTitle("Write ROM " + ecuCalDef->FileName + " to ECU");
    else
        this->setWindowTitle("Read ROM from ECU");
    this->show();
    this->serial = serial;

    int result = 0;

    ui->progressbar->setValue(0);

    result = ecu_functions(ecuCalDef, cmd_type);

    if (result == STATUS_SUCCESS)
    {
        QMessageBox::information(this, tr("ECU Operation"), "ECU operation was succesful, press OK to exit");
        this->close();
    }
    else
    {
        QMessageBox::warning(this, tr("ECU Operation"), "ECU operation failed, press OK to exit and try again");
        //delete ecuCalDef;
    }
}

EcuOperationsMercedes::~EcuOperationsMercedes()
{
    //delete ui;
}

void EcuOperationsMercedes::closeEvent(QCloseEvent *bar)
{
    kill_process = true;
    ecuOperationsIso14230->kill_process = true;
}

int EcuOperationsMercedes::ecu_functions(FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type)
{



    return STATUS_SUCCESS;
}






QString EcuOperationsMercedes::parse_message_to_hex(QByteArray received)
{
    QByteArray msg;

    for (unsigned long i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

int EcuOperationsMercedes::send_log_window_message(QString message, bool timestamp, bool linefeed)
{
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']  ");

    if (timestamp)
        message = dateTimeString + message;
    if (linefeed)
        message = message + "\n";

    QTextEdit* textedit = this->findChild<QTextEdit*>("text_edit");
    if (textedit)
    {
        ui->text_edit->insertPlainText(message);
        ui->text_edit->ensureCursorVisible();

        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}

void EcuOperationsMercedes::delay(int n)
{
    QTime dieTime = QTime::currentTime().addMSecs(n);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

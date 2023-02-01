#include "biu_operations_subaru.h"
#include <ui_biu_operations_subaru.h>

BiuOperationsSubaru::BiuOperationsSubaru(SerialPortActions *serial, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::BiuOperationsSubaruWindow)
{
    ui->setupUi(this);
    this->setParent(parent);

    ui->progressbar->hide();

    this->serial = serial;

    for (int i = 0; i < biu_messages.length(); i+=2)
    {
        ui->msg_combo_box->addItem(biu_messages.at(i));
    }

    connect(ui->send_msg, SIGNAL(clicked(bool)), this, SLOT(send_biu_msg()));


}

BiuOperationsSubaru::~BiuOperationsSubaru()
{
    //delete ui;
}

void BiuOperationsSubaru::send_biu_msg()
{
    QByteArray output;
    QByteArray received;

    QString selected_item_text = ui->msg_combo_box->currentText();
    QStringList selected_item_msg;

    bool ok = false;

    if (selected_item_text != "Custom")
    {
        for (int i = 0; i < biu_messages.length(); i+=2)
        {
            if (selected_item_text == biu_messages.at(i))
                selected_item_msg = biu_messages.at(i + 1).split(",");
        }
    }
    else
    {
        if (ui->msg_line_edit->text() != "")
            selected_item_msg = ui->msg_line_edit->text().split(",");
    }

    output.clear();
    output.append((uint8_t)0x80);
    output.append((uint8_t)0x40);
    output.append((uint8_t)0xf0);
    for (int i = 0; i < selected_item_msg.length(); i++)
    {
        output.append(selected_item_msg.at(i).toUInt(&ok, 16));
    }
    output[0] = output[0] | (output.length() - 3);

    send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);

    if (selected_item_text == "Connect")
    {
        serial->fast_init(output);
        delay(100);
    }
    else
        received = serial->write_serial_data_echo_check(output);

    received = serial->read_serial_data(100, 100);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
/*
    received.clear();
    received.append((uint8_t)0x80);
    received.append((uint8_t)0xf0);
    received.append((uint8_t)0x40);
    received.append((uint8_t)0x58);
    received.append((uint8_t)0x82);
    received.append((uint8_t)0x01);
    received.append((uint8_t)0x00);
    received.append((uint8_t)0x83);
    received.append((uint8_t)0x27);
    received.append((uint8_t)0xfe);
*/
    parse_biu_message(received);
}


void BiuOperationsSubaru::parse_biu_message(QByteArray message)
{
    if ((uint8_t)message.at(3) == (DTC_READ + 0x40))
    {
        QStringList dtc_code_list;
        QString dtc_code;
        QString byte;

        int index = 4;

        if (message.length() >= (index + 3))
        {
            send_log_window_message("BIU DTC list:", true, true);

            while (index < message.length())
            {
                byte = QString("%1").arg(message.at(index) & 0x0f,2,16,QLatin1Char('0'));
                dtc_code = "B" + byte;
                index++;
                byte = QString("%1").arg(message.at(index),2,16,QLatin1Char('0'));
                dtc_code.append(byte);
                index++;
                for (int i = 0; i < biu_dtc_list.length(); i+=2)
                {
                    if (dtc_code == biu_dtc_list.at(i))
                        dtc_code.append(" - " + biu_dtc_list.at(i + 1));
                }
                if (message.at(index) == 0)
                    dtc_code.append(" (pending)");
                else
                    dtc_code.append(" (stored)");
                index++;

                if (dtc_code != "")
                {
                    send_log_window_message(dtc_code, true, true);
                    dtc_code_list.append(dtc_code);
                }
            }
        }
        else
            send_log_window_message("No BIU DTC found", true, true);

    }
}

QString BiuOperationsSubaru::parse_message_to_hex(QByteArray received)
{
    QByteArray msg;

    for (unsigned long i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

int BiuOperationsSubaru::send_log_window_message(QString message, bool timestamp, bool linefeed)
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

void BiuOperationsSubaru::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

#include "hexcommander.h"

HexCommander::HexCommander(SerialPortActions *serial, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DataTerminal)
{
    ui->setupUi(this);

    this->serial = serial;

    // Set initial values
    ui->klineProtocol->addItem("SSM");
    ui->klineProtocol->addItem("iso14230");

    ui->klineBaudRate->setText("4800");

    ui->klineDataBits->addItem("7");
    ui->klineDataBits->addItem("8");
    ui->klineDataBits->addItem("9");
    ui->klineDataBits->setCurrentIndex(1);

    ui->klineStopBits->addItem("1");
    ui->klineStopBits->addItem("2");

    ui->klineParity->addItem("None");
    ui->klineParity->addItem("Odd");
    ui->klineParity->addItem("Even");

    ui->klineTesterId->setText("F0");
    ui->klineTargetId->setText("10");

    ui->canProtocol->addItem("CAN");
    ui->canProtocol->addItem("iso15765");
    ui->canProtocol->setCurrentIndex(1);

    ui->canBaudRate->setText("500000");

    ui->canIdLength->addItem("11bit");
    ui->canIdLength->addItem("29bit");

    ui->canTesterId->setText("7E0");
    ui->canTargetId->setText("7E8");

    connect(ui->klineProtocol, SIGNAL(currentIndexChanged(int)), this, SLOT(protocolTypeChanged(int)));
    connect(ui->canProtocol, SIGNAL(currentIndexChanged(int)), this, SLOT(protocolTypeChanged(int)));
    connect(ui->klineListen, SIGNAL(clicked(bool)), this, SLOT(listenInterface()));
    connect(ui->canListen, SIGNAL(clicked(bool)), this, SLOT(listenInterface()));
    connect(ui->sendMessage, SIGNAL(clicked(bool)), this, SLOT(sendToInterface()));

    this->show();
}

HexCommander::~HexCommander()
{


}

void HexCommander::protocolTypeChanged(int)
{
    qDebug() << "Change protocol type";
    QObject *obj = sender();
    QString interfaceTypeName = obj->objectName();

    QComboBox *protocolType = (QComboBox*)obj;

    if (protocolType)
    {
        if (interfaceTypeName == "klineProtocol")
            qDebug() << "K-Line protocol type changed to:" << protocolType->currentText();
        else if (interfaceTypeName == "canProtocol")
            qDebug() << "Can protocol type changed to:" << protocolType->currentText();
    }
}

void HexCommander::listenInterface()
{
    QObject *obj = sender();
    QString interfaceTypeName = obj->objectName();

    QPushButton *btn = (QPushButton*)obj;
    if (btn->isChecked())
        if (interfaceTypeName == "klineProtocol")
            qDebug() << "Start listening K-Line interface";
        else
            qDebug() << "Start listening CANbus interface";
    else
        if (interfaceTypeName == "klineProtocol")
            qDebug() << "Stop listening K-Line interface";
        else
            qDebug() << "Stop listening CANbus interface";
}

void HexCommander::sendToInterface()
{
    bool serialOk = true;
    QObject *obj = sender();
    QString interfaceTypeName = obj->objectName();
    qDebug() << "Send data to interface:" << interfaceTypeName;

    //if (interfaceTypeName.startsWith("kline"))
    //{
        qDebug() << "Send message via K-Line";

        qDebug() << "Checking protocol:" << ui->klineProtocol->currentText();
        if (ui->klineProtocol->currentText() == "SSM")
            serial->set_is_iso14230_connection(false);
        else if (ui->klineProtocol->currentText() == "iso14230")
            serial->set_is_iso14230_connection(true);
        else
            serialOk = false;

        qDebug() << "Checking baudrate:" << ui->klineBaudRate->text();
        if (ui->klineBaudRate->text().toDouble() >=300 && ui->klineBaudRate->text().toDouble() <=2000000)
            serial->set_serial_port_baudrate(ui->klineBaudRate->text());
        else
            serialOk = false;

        qDebug() << "Checking tester id:" << ui->klineTesterId->text();
        serial->set_iso14230_tester_id(ui->klineTesterId->text().toUInt());

        qDebug() << "Checking target id:" << ui->klineTargetId->text();
        serial->set_iso14230_target_id(ui->klineTargetId->text().toUInt());

        if (serialOk)
        {
            qDebug() << "All good, setting interface...";
            serial->set_iso14230_startbyte(0x80);
            serial->set_is_can_connection(false);
            serial->set_is_iso15765_connection(false);
            serial->set_is_29_bit_id(false);
            qDebug() << "Opening interface...";
            serial->open_serial_port();
        }

        QStringList msg = ui->msgToSend->text().split(" ");
        QByteArray output;
        QByteArray received;
        bool ok = false;
        qDebug() << "Append message to serial output" << msg;
        for (int i = 0; i < msg.length(); i++)
        {
            output.append(msg.at(i).toUInt(&ok, 16));
        }
        qDebug() << "Message to send:" << parse_message_to_hex(output);
        if (ui->klineProtocol->currentText() == "SSM")
            output = add_ssm_header(output, ui->klineTesterId->text().toUInt(&ok, 16), ui->klineTargetId->text().toUInt(&ok, 16), false);

        serial->write_serial_data_echo_check(output);

        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        delay(200);
        received = serial->read_serial_data(100, serial_read_short_timeout);
        send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
        qDebug() << "Response:" << parse_message_to_hex(received);

        serial->reset_connection();
    //}
}




/*
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray HexCommander::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
{
    uint8_t length = output.length();

    qDebug() << "Append SSM header for message:" << parse_message_to_hex(output) << "length:" << QString::number(length);
    output.insert(0, (uint8_t)0x80);
    output.insert(1, target_id & 0xFF);
    output.insert(2, tester_id & 0xFF);
    output.insert(3, length);

    output.append(calculate_checksum(output, dec_0x100));

    qDebug() << "Constructed SSM message:" << parse_message_to_hex(output);
    return output;
}

/*
 * Calculate SSM checksum to message
 *
 * @return 8-bit checksum
 */
uint8_t HexCommander::calculate_checksum(QByteArray output, bool dec_0x100)
{
    uint8_t checksum = 0;

    for (uint16_t i = 0; i < output.length(); i++)
        checksum += (uint8_t)output.at(i);

    if (dec_0x100)
        checksum = (uint8_t) (0x100 - checksum);

    return checksum;
}

/*
 * Parse QByteArray to readable form
 *
 * @return parsed message
 */
QString HexCommander::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

/*
 * Output text to log window
 *
 * @return
 */
int HexCommander::send_log_window_message(QString message, bool timestamp, bool linefeed)
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

void HexCommander::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

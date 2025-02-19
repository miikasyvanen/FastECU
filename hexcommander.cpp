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
    connect(ui->klineListen, SIGNAL(clicked(bool)), this, SLOT(listenInterface()));
    connect(ui->sendKlineMessage, SIGNAL(clicked(bool)), this, SLOT(sendToInterface()));

    connect(ui->canProtocol, SIGNAL(currentIndexChanged(int)), this, SLOT(protocolTypeChanged(int)));
    connect(ui->canListen, SIGNAL(clicked(bool)), this, SLOT(listenInterface()));
    connect(ui->sendCanMessage, SIGNAL(clicked(bool)), this, SLOT(sendToInterface()));

    this->show();
}

HexCommander::~HexCommander()
{
    delete ui;
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
    bool ok = false;
    bool readFile = false;
    QObject *obj = sender();
    QString interfaceTypeName = obj->objectName();
    qDebug() << "Send data to interface";

    QFile file;
    QString msg;
    QStringList msgList;

    if (interfaceTypeName.startsWith("sendKlineMessage"))
        msg = ui->klineMsgToSend->text();
    else
        msg = ui->canMsgToSend->text();

    if (msg == "")
    {
        QMessageBox::warning(this, tr("Data terminal"), "Add message bytes or file to send");
        return;
    }

    if (msg.at(0) != '.' && msg.at(0) != '/')
    {
        qDebug() << "Read message from lineedit";
        msgList.append(msg);
    }
    else
    {
        qDebug() << "Read message from file";
        readFile = true;
        QFile file(msg);
        if (!file.open(QIODevice::ReadOnly ))
        {
            QMessageBox::warning(this, tr("Data terminal"), "Unable to open datastream file '" + file.fileName() + "' for reading");
            emit LOG_I("Unable to open datastream file '" + file.fileName() + "' for reading", true, true);
            qDebug() << "Unable to open datastream file '" + file.fileName() + "' for reading";
            return;
        }
        QTextStream in(&file);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            msgList.append(line);
        }
        file.close();

    }

    if (interfaceTypeName.startsWith("sendKlineMessage"))
    {
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
        serial->set_iso14230_tester_id(ui->klineTesterId->text().toUInt(&ok, 16));

        qDebug() << "Checking target id:" << ui->klineTargetId->text();
        serial->set_iso14230_target_id(ui->klineTargetId->text().toUInt(&ok, 16));

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

        QStringList msg;// = ui->klineMsgToSend->text().split(" ");
        QByteArray output;
        QByteArray received;
        int rspDelay = 10;
        qDebug() << "Append message to serial output" << msg;
        for (int j = 0; j < msgList.length(); j++)
        {
            output.clear();
            received.clear();
            rspDelay = 10;
            if (!msgList.at(j).startsWith("delay"))
            {
                msg = msgList.at(j).split(" ");
                for (int i = 0; i < msg.length(); i++)
                {
                    output.append(msg.at(i).toUInt(&ok, 16));
                }
                if (ui->klineProtocol->currentText() == "SSM")
                    output = add_ssm_header(output, ui->klineTesterId->text().toUInt(&ok, 16), ui->klineTargetId->text().toUInt(&ok, 16), false);

                qDebug() << "Message to send:" << parse_message_to_hex(output);
            }
            if (msgList.length() > (j+1))
            {
                if (msgList.at(j+1).startsWith("delay"))
                {
                    qDebug() << "Set delay";
                    delay(msgList.at(j+1).split(")").at(1).split("(").at(0).toUInt());
                    j++;
                }
            }
            serial->write_serial_data_echo_check(output);

            emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(rspDelay);
            received = serial->read_serial_data(serial_read_short_timeout);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);
        }
        serial->reset_connection();
    }
    else if (interfaceTypeName.startsWith("sendCanMessage"))
    {
        qDebug() << "Send message via CAN / iso15765";

        qDebug() << "Checking protocol:" << ui->canProtocol->currentText();
        serial->set_is_can_connection(false);
        serial->set_is_iso15765_connection(false);
        if (ui->canProtocol->currentText() == "CAN")
            serial->set_is_can_connection(true);
        else if (ui->canProtocol->currentText() == "iso15765")
            serial->set_is_iso15765_connection(true);
        else
            serialOk = false;
        qDebug() << "Checking baudrate:" << ui->canBaudRate->text();
        if (ui->canBaudRate->text().toDouble() >=300 && ui->canBaudRate->text().toDouble() <=2000000)
            serial->set_can_speed(ui->canBaudRate->text());
        else
            serialOk = false;

        qDebug() << "Checking CAN ID length:" << ui->canIdLength->currentText();
        serial->set_is_29_bit_id(ui->canIdLength->currentIndex());

        qDebug() << "Checking tester id:" << ui->canTesterId->text();
        serial->set_iso15765_source_address(ui->canTesterId->text().toUInt(&ok, 16));

        qDebug() << "Checking target id:" << ui->canTargetId->text();
        serial->set_iso15765_destination_address(ui->canTargetId->text().toUInt(&ok, 16));

        if (serialOk)
        {
            qDebug() << "All good, setting interface...";
            qDebug() << "Opening interface...";
            serial->open_serial_port();
        }

        QStringList msg;// = ui->canMsgToSend->text().split(" ");
        QByteArray output;
        QByteArray received;
        int rspDelay = 100;
        for (int j = 0; j < msgList.length(); j++)
        {
            output.clear();
            received.clear();
            rspDelay = 10;
            if (!msgList.at(j).startsWith("delay"))
            {
                msg = msgList.at(j).split(" ");
                if (ui->canProtocol->currentText() == "CAN")
                {
                    if (msg.length() > 8)
                    {
                        qDebug() << "ERROR: CAN message too long (8 message bytes)";
                        QMessageBox::warning(this, tr("CAN message"), "ERROR: CAN message too long (use 4 ID bytes + 8 message bytes)");
                    }
                    qDebug() << "Append message to CAN output" << msg;
                }
                for (int i = 3; i >= 0; i--)
                {
                    output.append(((ui->canTesterId->text().toUInt(&ok, 16) >> (i * 8)) & 0xff));
                }
                for (int i = 0; i < msg.length(); i++)
                {
                    output.append(msg.at(i).toUInt(&ok, 16));
                }
                qDebug() << "Message to send:" << parse_message_to_hex(output);
            }
            if (msgList.length() > (j+1))
            {
                if (msgList.at(j+1).startsWith("delay"))
                {
                    qDebug() << "Set delay";
                    rspDelay = msgList.at(j+1).split(")").at(1).split("(").at(0).toUInt();
                    j++;
                }
            }
            serial->write_serial_data_echo_check(output);

            emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(rspDelay);
            received = serial->read_serial_data(serial_read_short_timeout);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);
        }
        serial->reset_connection();
    }
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

void HexCommander::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

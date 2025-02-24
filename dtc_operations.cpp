#include "dtc_operations.h"

DtcOperations::DtcOperations(SerialPortActions *serial, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DtcOperationsWindow)
    , serial(serial)
{
    ui->setupUi(this);
    //this->show();

    emit LOG_I("DtcOperations Started", true, true);

    //run();
    //init_obd();
}

DtcOperations::~DtcOperations()
{
    delete ui;
}

void DtcOperations::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

void DtcOperations::run()
{
    this->show();
    emit LOG_I("Start RUN", true, true);

    int result = STATUS_ERROR;

    // Set serial port
    serial->set_is_iso14230_connection(false);
    serial->set_add_iso14230_header(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    serial->set_can_speed("500000");
    serial->set_iso14230_startbyte(0xc0);
    serial->set_iso14230_tester_id(0xf1);
    serial->set_iso14230_target_id(0x33);
    serial->set_can_source_address(0x7e0);
    serial->set_can_destination_address(0x7e8);
    serial->set_iso15765_source_address(0x7e0);
    serial->set_iso15765_destination_address(0x7e8);
    // Open serial port
    serial->open_serial_port();

    init_obd();
}

int DtcOperations::init_obd()
{
    emit LOG_I("Start INIT_OBD", true, true);

    QByteArray output;
    QByteArray received;
    int result = STATUS_SUCCESS;

    // FIVE_BAUD_INIT
    emit LOG_I("Request FIVE BAUD INIT", true, true);

    output.clear();
    output.append((uint8_t)0x33);
    result = serial->five_baud_init(output);
    received = serial->read_serial_data(serial_read_timeout);
    emit LOG_I("Result: " + QString::number(result), true, true);
/*
    if (result == STATUS_SUCCESS)
    {
        delay(1000);
        emit LOG_I("Send init response", true, true);
        output.clear();
        output.append((uint8_t)~0x08);
        serial->write_serial_data_echo_check(output);
        emit LOG_I("Request response", true, true);
        received = serial->read_serial_data(serial_read_timeout);

    }
*/
    return STATUS_SUCCESS;
}






/*
 * Parse QByteArray to readable form
 *
 * @return parsed message
 */
QString DtcOperations::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

void DtcOperations::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

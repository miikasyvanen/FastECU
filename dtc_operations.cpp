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
    serial->set_serial_port_baudrate("10400");
    // Open serial port
    serial->open_serial_port();

    init_obd();
}

int DtcOperations::init_obd()
{
    emit LOG_I("Start INIT_OBD", true, true);

    QByteArray output;
    QByteArray received;
    QByteArray VIN;
    int result = STATUS_SUCCESS;

    // FIVE_BAUD_INIT
    emit LOG_I("Request FIVE BAUD INIT", true, true);

    output.clear();
    output.append((uint8_t)0x33);
    received = serial->five_baud_init(output);
    //received = serial->read_serial_data(serial_read_extra_short_timeout);
    serial->set_comm_busy(true);
    emit LOG_I("5 baud init response: " + parse_message_to_hex(received), true, true);
    //serial->set_j2534_ioctl(W3, (uint8_t)received.at(5));
    serial->set_j2534_ioctl(P1_MAX, 35);
    //serial->set_j2534_ioctl(P4_MIN, 5);

    delay(500);
    output.clear();
    output.append((uint8_t)0x68);
    output.append((uint8_t)0x6A);
    output.append((uint8_t)0xF1);
    output.append((uint8_t)0x09);
    output.append((uint8_t)0x02);
    //output.append((uint8_t)0x0D);
    output.append(calculate_checksum(output, false));
    serial->write_serial_data_echo_check(output);
    while (1)
    {
        serial->set_comm_busy(true);
        received = serial->read_serial_data(serial_read_timeout);
        if (received.at(3) != 0x49)
            break;
        received.remove(0, 6);
        received.remove(received.length()-1, 1);
        VIN.append(received);
    }
    emit LOG_I("VIN: " + parse_message_to_hex(VIN), true, true);

    return STATUS_SUCCESS;
}






/*
 * Calculate SSM checksum to message
 *
 * @return 8-bit checksum
 */
uint8_t DtcOperations::calculate_checksum(QByteArray output, bool dec_0x100)
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

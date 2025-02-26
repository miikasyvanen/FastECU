#include "dtc_operations.h"

DtcOperations::DtcOperations(SerialPortActions *serial, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DtcOperationsWindow)
    , serial(serial)
{
    ui->setupUi(this);

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
    QByteArray output;
    QByteArray received;
    QByteArray response;
    int result = STATUS_SUCCESS;

    if (!serial->is_serial_port_open())
    {
        emit LOG_E("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Initializing iso9141 K-Line communications, please wait...", true, true);

    output.clear();
    output.append((uint8_t)0x33);

    if (serial->get_use_openport2_adapter())
        serial->set_j2534_ioctl(P1_MAX, 35);
    else
        serial->set_kline_timings(SERIAL_P1_MAX, 35);

    received = serial->five_baud_init(output);
    emit LOG_I("Five baud init response: " + parse_message_to_hex(received), true, true);


    if (serial->get_use_openport2_adapter())
    {
        if ((uint8_t)received.at(5) == '8' && (uint8_t)received.at(7) == '8')
            five_baud_init_ok = true;
    }
    else
    {
        if ((uint8_t)received.at(1) == 0x08 && (uint8_t)received.at(2) == 0x08)
                five_baud_init_ok = true;
        serial->set_kline_timings(SERIAL_P1_MAX, 25);
    }
    if (five_baud_init_ok)
    {
        emit LOG_I("ISO9141 five baud init mode succesfully completed.", true, true);

        emit LOG_I("Requesting vehicle info, please wait...", true, true);

        delay(500);
        response = request_data(vehicle_info, request_VIN);
        emit LOG_I("VIN: " + parse_message_to_hex(response), true, true);
        emit LOG_I("VIN: " + QString(response), true, true);
        delay(250);
        response = request_data(vehicle_info, request_CAL_ID_Length);
        emit LOG_I("CAL ID length: " + parse_message_to_hex(response), true, true);
        delay(250);
        response = request_data(vehicle_info, request_CAL_ID);
        emit LOG_I("CAL ID: " + parse_message_to_hex(response), true, true);
        emit LOG_I("CAL ID: " + QString(response), true, true);
        delay(250);
        response = request_data(vehicle_info, request_CAL_ID_Num_Length);
        emit LOG_I("CAL ID num length: " + parse_message_to_hex(response), true, true);
        delay(250);
        response = request_data(vehicle_info, request_CAL_ID_Num);
        emit LOG_I("CAL ID num: " + parse_message_to_hex(response), true, true);
        //emit LOG_I("CAL ID num: " + QString(response), true, true);
        delay(250);

        response = request_dtc_list();
        emit LOG_I("DTCs: " + parse_message_to_hex(response), true, true);

        for (int i = 0; i < response.length(); i+=2)
        {
            emit LOG_I("DTC: " + FileActions::parse_dtc_message(((uint8_t)response.at(i) << 8) + (uint8_t)response.at(i+1)), true, true);
            //emit LOG_I("DTC: " + QString::number(((uint8_t)response.at(i) << 8) + (uint8_t)response.at(i+1), 16), true, true);
        }
        delay(250);

    }

    return STATUS_SUCCESS;
}

QByteArray DtcOperations::request_data(const uint8_t cmd, const uint8_t sub_cmd)
{
    QByteArray output;
    QByteArray received;
    QByteArray response;

    output.clear();
    for (unsigned long i = 0; i < ARRAYSIZE(live_data_start_bytes_9141); i++)
        output.append((uint8_t)live_data_start_bytes_9141[i]);
    output.append((uint8_t)cmd);
    output.append((uint8_t)sub_cmd);
    output.append(calculate_checksum(output, false));
    serial->write_serial_data_echo_check(output);
    while (1)
    {
        if (serial->get_use_openport2_adapter())
            received = serial->read_serial_data(serial_read_short_timeout);
        else
            received = serial->read_serial_obd_data(serial_read_short_timeout);

        if (received.at(3) != (cmd | 0x40) || received.at(4) != sub_cmd)
            break;

        received.remove(received.length()-1, 1);
        if (received.length() < 7)
            received.remove(0, received.length()-1);
        else
            received.remove(0, 6);
        response.append(received);
    }

    return response;
}

QByteArray DtcOperations::request_dtc_list()
{
    QByteArray output;
    QByteArray received;
    QByteArray response;

    output.clear();
    for (unsigned long i = 0; i < ARRAYSIZE(live_data_start_bytes_9141); i++)
        output.append((uint8_t)live_data_start_bytes_9141[i]);
    output.append((uint8_t)0x03);
    output.append(calculate_checksum(output, false));
    serial->write_serial_data_echo_check(output);
    while (1)
    {
        if (serial->get_use_openport2_adapter())
            received = serial->read_serial_data(serial_read_short_timeout);
        else
            received = serial->read_serial_obd_data(serial_read_short_timeout);

        if (received.at(3) != (0x03 | 0x40))
            break;

        received.remove(received.length()-1, 1);
        if (received.length() < 7)
            received.remove(0, received.length()-1);
        else
            received.remove(0, 4);
        response.append(received);
    }

    return response;

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

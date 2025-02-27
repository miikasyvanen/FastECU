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
    int result = STATUS_SUCCESS;

    if (!serial->is_serial_port_open())
    {
        emit LOG_E("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    result = five_baud_init();
    delay(1000);
    if (result)
        result = fast_init();
    if (result)
        return STATUS_ERROR;

    return STATUS_SUCCESS;
}

int DtcOperations::five_baud_init()
{
    QByteArray output;
    QByteArray received;
    QByteArray response;
    int result = STATUS_SUCCESS;

    emit LOG_I("Initializing iso9141 K-Line communications, please wait...", true, true);

    output.clear();
    output.append((uint8_t)0x33);
    received = serial->five_baud_init(output);

    serial->set_comm_busy(true);
    //emit LOG_I("5 baud init response: " + parse_message_to_hex(received), true, true);
    serial->set_j2534_ioctl(P1_MAX, 35);

    if ((uint8_t)received.at(5) == '8' && (uint8_t)received.at(7) == '8')
        emit LOG_I("iso9141 five baud init mode succesfully completed.", true, true);

    emit LOG_I("Requesting vehicle info, please wait...", true, true);

<<<<<<< Updated upstream
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
=======
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
        emit LOG_I("iso9141 five baud init mode succesfully completed.", true, true);

        emit LOG_I("Requesting vehicle info, please wait...", true, true);

        delay(500);
        response = request_data(vehicle_info, request_VIN);
        if (!response.length())
            return STATUS_ERROR;
        emit LOG_I("VIN: " + parse_message_to_hex(response), true, true);
        emit LOG_I("VIN: " + QString(response), true, true);
        delay(250);
        response = request_data(vehicle_info, request_CAL_ID_Length);
        if (!response.length())
            return STATUS_ERROR;
        emit LOG_I("CAL ID length: " + parse_message_to_hex(response), true, true);
        delay(250);
        response = request_data(vehicle_info, request_CAL_ID);
        if (!response.length())
            return STATUS_ERROR;
        emit LOG_I("CAL ID: " + parse_message_to_hex(response), true, true);
        emit LOG_I("CAL ID: " + QString(response), true, true);
        delay(250);
        response = request_data(vehicle_info, request_CAL_ID_Num_Length);
        if (!response.length())
            return STATUS_ERROR;
        emit LOG_I("CAL ID num length: " + parse_message_to_hex(response), true, true);
        delay(250);
        response = request_data(vehicle_info, request_CAL_ID_Num);
        if (!response.length())
            return STATUS_ERROR;
        emit LOG_I("CAL ID num: " + parse_message_to_hex(response), true, true);
        //emit LOG_I("CAL ID num: " + QString(response), true, true);
        delay(250);

        result = read_dtc();
        if (result)
            return STATUS_ERROR;
    }
    else
        return STATUS_ERROR;

    return STATUS_SUCCESS;
}

int DtcOperations::fast_init()
{
    QByteArray output;
    QByteArray received;
    QByteArray response;
    int result = STATUS_SUCCESS;

    serial->reset_connection();
    serial->set_is_iso14230_connection(true);
    serial->open_serial_port();

    emit LOG_I("Initializing iso14230 K-Line communications, please wait...", true, true);

    output.clear();
    output.append((uint8_t)0xC1);
    output.append((uint8_t)0x33);
    output.append((uint8_t)0xF1);
    output.append((uint8_t)init_OBD);
    output.append(calculate_checksum(output, false));
    result = serial->fast_init(output);
    //if (!result)
    //    return STATUS_ERROR;

    if (serial->get_use_openport2_adapter())
        received = serial->read_serial_data(serial_read_short_timeout);
    else
        received = serial->read_serial_obd_data(serial_read_short_timeout);
    //emit LOG_I("Response: " + parse_message_to_hex(response), true, true);

    if ((uint8_t)received.at(0) == 0x83 && (uint8_t)received.at(1) == 0xf1 && (uint8_t)received.at(2) == 0x10 && (uint8_t)received.at(3) == 0xc1 && (uint8_t)received.at(4) == 0xe9 && (uint8_t)received.at(5) == 0x8f)
        fast_init_ok = true;

    if (fast_init_ok)
    {
        emit LOG_I("iso14230 fast init mode succesfully completed.", true, true);

        emit LOG_I("Requesting vehicle info, please wait...", true, true);

        delay(500);
        response = request_data(vehicle_info, request_VIN);
        if (response.length())
        {
            emit LOG_I("VIN: " + parse_message_to_hex(response), true, true);
            emit LOG_I("VIN: " + QString(response), true, true);
        }
        delay(250);
        response = request_data(vehicle_info, request_CAL_ID_Length);
        if (response.length())
        {
            emit LOG_I("CAL ID length: " + parse_message_to_hex(response), true, true);
        }
        delay(250);
        response = request_data(vehicle_info, request_CAL_ID);
        if (response.length())
        {
            emit LOG_I("CAL ID: " + parse_message_to_hex(response), true, true);
            emit LOG_I("CAL ID: " + QString(response), true, true);
        }
        delay(250);
        response = request_data(vehicle_info, request_CAL_ID_Num_Length);
        if (response.length())
        {
            emit LOG_I("CAL ID num length: " + parse_message_to_hex(response), true, true);
        }
        delay(250);
        response = request_data(vehicle_info, request_CAL_ID_Num);
        if (response.length())
        {
            emit LOG_I("CAL ID num: " + parse_message_to_hex(response), true, true);
        }
        delay(250);

        result = read_dtc();
        if (result)
            return STATUS_ERROR;
    }
    else
        return STATUS_ERROR;
>>>>>>> Stashed changes

    return STATUS_SUCCESS;
}

QByteArray DtcOperations::request_data(const uint8_t cmd, const uint8_t sub_cmd)
{
    QByteArray output;
    QByteArray received;
    QByteArray response;

    output.clear();
    if (!serial->get_is_iso14230_connection())
    {
        for (unsigned long i = 0; i < ARRAYSIZE(live_data_start_bytes_9141); i++)
            output.append((uint8_t)live_data_start_bytes_9141[i]);
    }
    else if (serial->get_is_iso14230_connection())
    {
        for (unsigned long i = 0; i < ARRAYSIZE(live_data_start_bytes); i++)
            output.append((uint8_t)live_data_start_bytes[i]);
    }
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

        if (received.at(3) == 0x7f)
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(3, received.length()-1)), true, true);
            break;
        }

        if (received.at(3) != (cmd | 0x40) || received.at(4) != sub_cmd)
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(3, received.length()-1)), true, true);
            break;
        }

        received.remove(received.length()-1, 1);
        if (received.length() < 7)
            received.remove(0, received.length()-1);
        else
            received.remove(0, 6);
        response.append(received);
    }

    return response;
}


<<<<<<< Updated upstream
=======
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

int DtcOperations::read_dtc()
{
    QByteArray response;
    QStringList dtc_list;
    bool ok = false;

    response = request_dtc_list();
    if (!response.length())
        return STATUS_ERROR;
    emit LOG_I("DTCs: " + parse_message_to_hex(response), true, true);

    for (int i = 0; i < response.length(); i+=2)
    {
        uint16_t dtc = ((uint8_t)response.at(i) << 8) + (uint8_t)response.at(i+1);
        if (dtc != 0)
        {
            dtc_list.append(QString("%1").arg(dtc,4,16,QLatin1Char('0')).toUpper());
            //emit LOG_I("DTC: " + FileActions::parse_dtc_message(dtc), true, true);
        }
    }
    sort(dtc_list.begin(), dtc_list.end(), less<QString>());
    for (int i = 0; i < dtc_list.length(); i++)
        emit LOG_I("DTC: " + FileActions::parse_dtc_message(dtc_list.at(i).toUInt(&ok, 16)), true, true);

    delay(250);

    return STATUS_SUCCESS;
}

int DtcOperations::clear_dtc()
{

    return STATUS_SUCCESS;
}
>>>>>>> Stashed changes



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

#include "dtc_operations.h"

DtcOperations::DtcOperations(SerialPortActions *serial, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DtcOperationsWindow)
    , serial(serial)
{
    ui->setupUi(this);

    ui->protocolComboBox->addItem("SSM (K-Line)");
    ui->protocolComboBox->addItem("SSM (CAN)");
    ui->protocolComboBox->addItem("iso9141");
    ui->protocolComboBox->addItem("iso14230");
    ui->protocolComboBox->addItem("iso15765");
    ui->protocolComboBox->setCurrentIndex(2);

    auto * model = qobject_cast<QStandardItemModel*>(ui->protocolComboBox->model());
    assert(model);
    if(!model) return;

    for (int i = 0; i < ui->protocolComboBox->count(); i++)
    {
        auto * item = model->item(i);
        assert(item);
        if(!item)
            return;
        if (item->text().startsWith("SSM "))
            item->setEnabled(false);
    }

    connect(ui->readDtcButton, &QPushButton::clicked, this, &DtcOperations::select_operation);
    connect(ui->clearDtcButton, &QPushButton::clicked, this, &DtcOperations::select_operation);
    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::close);

    this->show();
}

DtcOperations::~DtcOperations()
{
    delete ui;
}

void DtcOperations::closeEvent(QCloseEvent *event)
{
    serial->reset_connection();
    kill_process = true;
}

void DtcOperations::run()
{
    int result = STATUS_ERROR;

    // Set serial port
    serial->set_is_iso14230_connection(false);
    serial->set_add_iso14230_header(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    serial->set_can_speed("500000");
    serial->set_serial_port_baudrate("10400");
    // Open serial port
    serial->open_serial_port();

    //init_obd();
}

/*************************************
 *
 *
 *
 ************************************/
int DtcOperations::select_operation()
{
    QObject *obj = QObject::sender();
    int result = STATUS_SUCCESS;

    if (ui->protocolComboBox->currentText().startsWith("SSM"))
    {

    }
    else if (ui->protocolComboBox->currentText().startsWith("iso9141"))
    {
        serial->set_kline_startbyte(0x68);
        serial->set_kline_tester_id(0xF1);
        serial->set_kline_target_id(0x6A);

        result = five_baud_init("iso9141");
    }
    else if (ui->protocolComboBox->currentText().startsWith("iso14230"))
    {
        serial->set_kline_startbyte(0xC0);
        serial->set_kline_tester_id(0xF1);
        serial->set_kline_target_id(0x33);

        result = fast_init();
        if (result)
            result = five_baud_init("iso14230");
    }
    else if (ui->protocolComboBox->currentText().startsWith("iso15765"))
    {
        source_id = 0x7e0;
        destination_id = 0x7e8;
        serial->set_iso15765_source_address(source_id);
        serial->set_iso15765_destination_address(destination_id);
        result = iso15765_init();
    }

    if (result == STATUS_SUCCESS)
    {
        if (obj->objectName() == "readDtcButton")
        {
            result = read_dtc();
        }
        else if (obj->objectName() == "clearDtcButton")
        {
            result = clear_dtc();
        }
    }

    serial->set_add_ssm_header(false);
    serial->set_add_iso9141_header(false);
    serial->set_add_iso14230_header(false);
    serial->reset_connection();

    if (result)
        return result;

    return STATUS_SUCCESS;
}

int DtcOperations::five_baud_init(QString protocol)
{
    QByteArray output;
    QByteArray received;
    QByteArray response;
    int result = STATUS_SUCCESS;

    five_baud_init_iso9141_ok = false;
    five_baud_init_iso14230_ok = false;

    serial->reset_connection();
    serial->set_is_iso14230_connection(false);
    serial->set_add_ssm_header(false);
    serial->set_add_iso9141_header(false);
    serial->set_add_iso14230_header(false);
    serial->set_serial_port_baudrate("10400");
    serial->open_serial_port();

    emit LOG_I("Testing " + protocol + " five baud init, please wait...", true, true);

    if (serial->get_use_openport2_adapter())
        serial->set_j2534_ioctl(P1_MAX, 35);
    else
        serial->set_kline_timings(SERIAL_P1_MAX, 35);

    output.clear();
    output.append((uint8_t)five_baud_init_OBD);
    received = serial->five_baud_init(output);
    emit LOG_I("Init response: " + parse_message_to_hex(received), true, true);

    if (serial->get_use_openport2_adapter())
    {
        if ((uint8_t)received.at(5) == '8' && (uint8_t)received.at(7) == '8' && protocol == "iso9141")
            five_baud_init_iso9141_ok = true;
        if ((uint8_t)received.at(8) == '8' && (uint8_t)received.at(9) == 'f' && protocol == "iso14230")
            five_baud_init_iso14230_ok = true;
    }
    else
    {
        if ((uint8_t)received.at(1) == 0x08 && (uint8_t)received.at(2) == 0x08)
            five_baud_init_iso9141_ok = true;
        if ((uint8_t)received.at(2) == 0x8f)
            five_baud_init_iso14230_ok = true;
        serial->set_kline_timings(SERIAL_P1_MAX, 25);
    }
    if (five_baud_init_iso9141_ok)
    {
        serial->set_add_iso9141_header(true);
        emit LOG_I(protocol + " five baud init succesfully completed.", true, true);

        request_vehicle_info();

    }
    else if (five_baud_init_iso14230_ok)
    {
        serial->set_add_iso14230_header(true);
        emit LOG_I(protocol + " five baud init succesfully completed.", true, true);

        request_vehicle_info();

    }
    else
    {
        emit LOG_E(protocol + " five baud init failed.", true, true);
        return STATUS_ERROR;
    }

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
    serial->set_add_ssm_header(false);
    serial->set_add_iso9141_header(false);
    serial->set_add_iso14230_header(true);
    serial->set_serial_port_baudrate("10400");
    serial->open_serial_port();

    emit LOG_I("Initialising iso14230 fast init K-Line communications, please wait...", true, true);

    output.clear();
    output.append((uint8_t)fast_init_OBD);
    result = serial->fast_init(output);
    if (result)
        return STATUS_ERROR;

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

        request_vehicle_info();

    }
    else
    {
        emit LOG_E("iso14230 fast init mode failed.", true, true);
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
}

int DtcOperations::iso15765_init()
{
    QByteArray output;
    QByteArray received;
    QByteArray response;
    int result = STATUS_SUCCESS;

    serial->reset_connection();
    serial->set_is_iso14230_connection(false);
    serial->set_add_ssm_header(false);
    serial->set_add_iso9141_header(false);
    serial->set_add_iso14230_header(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(true);
    serial->set_is_29_bit_id(false);
    serial->set_can_speed("500000");
    serial->open_serial_port();

    emit LOG_I("Initialising iso15765 CAN communications, please wait...", true, true);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)(source_id >> 8) & 0xff);
    output.append((uint8_t)source_id & 0xff);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(output);
    received = serial->read_serial_data(serial_read_timeout);
    if (received.length() > 4)
    {
        if (received.at(4) == 0x7f)
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(3, received.length()-1)), true, true);
            return STATUS_ERROR;
        }
        else if (received.at(4) != 0x41)
        {
            emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
        return STATUS_ERROR;

    can_init_ok = true;

    if (can_init_ok)
    {
        emit LOG_I("iso15765 init mode succesfully completed.", true, true);

        request_vehicle_info();

    }
    else
    {
        emit LOG_E("iso15765 init mode failed.", true, true);
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
}

QByteArray DtcOperations::request_data(const uint8_t cmd, const uint8_t sub_cmd)
{
    QByteArray output;
    QByteArray received;
    QByteArray response;

    uint8_t cmd_index = 3;

    output.clear();
    if (ui->protocolComboBox->currentText().startsWith("iso15765"))
    {
        cmd_index = 4;
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)(source_id >> 8));
        output.append((uint8_t)source_id);
    }
    output.append((uint8_t)cmd);
    output.append((uint8_t)sub_cmd);
    serial->write_serial_data_echo_check(output);
    while (1)
    {
        if (serial->get_use_openport2_adapter())
            received = serial->read_serial_data(serial_read_short_timeout);
        else
            received = serial->read_serial_obd_data(serial_read_short_timeout);

        if (!received.length())
            break;
        else if (received.length() > cmd_index)
        {
            if (received.at(cmd_index) == 0x7f)
            {
                emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(cmd_index, received.length()-1)), true, true);
                break;
            }
            else if (received.at(cmd_index) != (cmd | 0x40) || received.at(cmd_index+1) != sub_cmd)
            {
                emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
                break;
            }
        }
        if (ui->protocolComboBox->currentText().startsWith("iso15765"))
        {
            received.remove(0, cmd_index+3);
            response.append(received);
        }
        else
        {
            received.remove(received.length()-1, 1);
            if (received.length() < 7)
                received.remove(0, received.length()-1);
            else if (received.length() < 10)
                received.remove(0, 5);
            else
                received.remove(0, 6);
            response.append(received);
        }
    }

    return response;
}

void DtcOperations::request_vehicle_info()
{
    QByteArray output;
    QByteArray received;
    QByteArray response;

    emit LOG_I("Requesting vehicle info, please wait...", true, true);

    delay(500);
    for (unsigned int info = 0; info < 7; info++)
    {
        response = request_data(live_data, request_support_info[info]);
        if (response.length())
        {
            QString PIDs;
            int PIDstart = info * 0x20+1;
            int PIDend = PIDstart + 0x1f;
            emit LOG_I("Supported PIDs 0x" + QString::number(PIDstart, 16) + "-0x" + QString::number(PIDend, 16) + ": " + parse_message_to_hex(response), true, true);
            //emit LOG_I("Supported PIDs: " + QString(response), true, true);
            emit LOG_I("Supported PIDs: ", true, false);
            for (int i = 0; i < response.length(); i++)
            {
                for (int j = 7; j >= 0; j--)
                {
                    int PIDen = (((uint8_t)response.at(i) >> j) & 0x01);
                    PIDs.append(QString("0x%1 ").arg(PIDen * ((i * 8 + 7 - j) + PIDstart),2,16,QLatin1Char('0')));
                }
            }
            emit LOG_I(PIDs, false, true);
        }
        delay(250);
    }
    response = request_data(live_data, MONITOR_STATUS_SINCE_DTC_CLEARED);
    if (response.length())
    {
        emit LOG_I("Status since DTCs cleared: " + parse_message_to_hex(response), true, true);
    }
    delay(250);
    response = request_data(vehicle_info, request_VIN_length);
    if (response.length())
    {
        emit LOG_I("VIN length: " + parse_message_to_hex(response), true, true);
    }
    delay(250);
    response = request_data(vehicle_info, request_VIN);
    if (response.length())
    {
        emit LOG_I("VIN: " + parse_message_to_hex(response), true, true);
        emit LOG_I("VIN: " + QString(response), true, true);
    }
    delay(250);
    response = request_data(vehicle_info, request_CAL_ID_length);
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
    response = request_data(vehicle_info, request_CVN_length);
    if (response.length())
    {
        emit LOG_I("CAL ID num length: " + parse_message_to_hex(response), true, true);
    }
    delay(250);
    response = request_data(vehicle_info, request_CVN);
    if (response.length())
    {
        emit LOG_I("CAL ID num: " + parse_message_to_hex(response), true, true);
    }
    delay(250);

}

QByteArray DtcOperations::request_dtc_list(uint8_t cmd)
{
    QByteArray output;
    QByteArray received;
    QByteArray response;
    uint8_t cmd_index = 3;

    output.clear();
    if (ui->protocolComboBox->currentText().startsWith("iso15765"))
    {
        cmd_index = 4;
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)(source_id >> 8));
        output.append((uint8_t)source_id);
    }
    output.append(cmd);
    serial->write_serial_data_echo_check(output);
    while (1)
    {
        if (serial->get_use_openport2_adapter())
            received = serial->read_serial_data(serial_read_short_timeout);
        else
            received = serial->read_serial_obd_data(serial_read_short_timeout);

        if (!received.length())
            break;
        if (received.length() > cmd_index)
        {
            if (received.at(cmd_index) == 0x7f)
            {
                emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(cmd_index, received.length()-1)), true, true);
                break;
            }
            else if (received.at(cmd_index) != (cmd | 0x40))
            {
                emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
                break;
            }
        }
        if (ui->protocolComboBox->currentText().startsWith("iso15765"))
        {
            received.remove(0, cmd_index+2);
            response.append(received);
        }
        else
        {
            received.remove(received.length()-1, 1);
            if (received.length() < 7)
                received.remove(0, received.length()-1);
            else
                received.remove(0, 4);
            response.append(received);
        }
    }

    return response;

}

int DtcOperations::read_dtc()
{
    QByteArray response;
    QStringList dtc_list;
    bool ok = false;

    response = request_dtc_list(read_stored_DTCs);
    if (!response.length())
        return STATUS_ERROR;

    emit LOG_I("Stored DTCs: " + parse_message_to_hex(response), true, true);

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

    response = request_dtc_list(read_pending_DTCs);
    if (!response.length())
        return STATUS_ERROR;

    emit LOG_I("Pending DTCs: " + parse_message_to_hex(response), true, true);

    dtc_list.clear();
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
    {
            emit LOG_I("DTC: " + FileActions::parse_dtc_message(dtc_list.at(i).toUInt(&ok, 16)), true, true);
    }

    delay(250);

    emit LOG_I("Diagnostic trouble codes succesfully read!", true, true);

    return STATUS_SUCCESS;
}

int DtcOperations::clear_dtc()
{
    QByteArray output;
    QByteArray received;
    uint8_t cmd_index = 3;
    bool clear_dtc_ok = false;

    result = read_dtc();
    if (result)
        return STATUS_ERROR;

    output.clear();
    if (ui->protocolComboBox->currentText().startsWith("iso15765"))
    {
        cmd_index = 4;
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)(source_id >> 8));
        output.append((uint8_t)source_id);
    }
    output.append((uint8_t)clear_DTCs);
    serial->write_serial_data_echo_check(output);
    while (1)
    {
        if (serial->get_use_openport2_adapter())
            received = serial->read_serial_data(serial_read_short_timeout);
        else
            received = serial->read_serial_obd_data(serial_read_short_timeout);

        if (!received.length())
            break;
        if (received.length() > cmd_index)
        {
            if (received.at(cmd_index) == 0x7f)
            {
                emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(cmd_index, received.length()-1)), true, true);
                break;
            }
            else if (received.at(cmd_index) != (clear_DTCs | 0x40))
            {
                emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
                break;
            }
            else
            {
                clear_dtc_ok = true;
                break;
            }
        }
    }

    if (!clear_dtc_ok)
        return STATUS_ERROR;

    emit LOG_I("Diagnostic trouble codes succesfully cleared!", true, true);

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

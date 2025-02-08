#include "flash_ecu_subaru_hitachi_m32r_can.h"

FlashEcuSubaruHitachiM32rCan::FlashEcuSubaruHitachiM32rCan(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EcuOperationsWindow)
    , ecuCalDef(ecuCalDef)
    , cmd_type(cmd_type)
{
    ui->setupUi(this);

    if (cmd_type == "test_write")
        this->setWindowTitle("Test write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "write")
        this->setWindowTitle("Write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "read")
        this->setWindowTitle("Read ROM from ECU");

    this->serial = serial;
}

void FlashEcuSubaruHitachiM32rCan::run()
{
    this->show();

    int result = STATUS_ERROR;

    //result = init_flash_subaru_hitachi_can();

    mcu_type_string = ecuCalDef->McuType;
    mcu_type_index = 0;

    while (flashdevices[mcu_type_index].name != 0)
    {
        if (flashdevices[mcu_type_index].name == mcu_type_string)
            break;
        mcu_type_index++;
    }
    QString mcu_name = flashdevices[mcu_type_index].name;
    //send_log_window_message("MCU type: " + mcu_name + " and index: " + mcu_type_index, true, true);
    qDebug() << "MCU type:" << mcu_name << mcu_type_string << "and index:" << mcu_type_index;

    flash_method = ecuCalDef->FlashMethod;

    emit external_logger("Starting");

    if (cmd_type == "read")
    {
        send_log_window_message("Read memory with flashmethod '" + flash_method + "'", true, true);
        //qDebug() << "Read memory with flashmethod" << flash_method;
    }
    else if (cmd_type == "write")
    {
        send_log_window_message("Write memory with flashmethod '" + flash_method + "'", true, true);
        //qDebug() << "Write memory with flashmethod" << flash_method;
    }

    // Set serial port
    serial->set_is_iso14230_connection(false);
    serial->set_add_iso14230_header(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(true);
    serial->set_is_29_bit_id(false);
    serial->set_can_speed("500000");
    serial->set_iso15765_source_address(0x7E0);
    serial->set_iso15765_destination_address(0x7E8);
    // Open serial port
    serial->open_serial_port();

    int ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                   tr("Turn ignition ON and press OK to start initializing connection to ECU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);

    switch (ret)
    {
        case QMessageBox::Ok:
            send_log_window_message("Connecting to Subaru ECU 512Kb Hitachi CAN bootloader, please wait...", true, true);
            result = connect_bootloader();

            if (result == STATUS_SUCCESS)
            {
                if (cmd_type == "read")
                {
                    emit external_logger("Reading ROM, please wait...");
                    send_log_window_message("Reading ROM from ECU Subaru using CAN", true, true);
                    result = read_mem(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                }
                else if (cmd_type == "test_write" || cmd_type == "write")
                {
                    emit external_logger("Writing ROM, please wait...");
                    send_log_window_message("Writing ROM to Subaru Hitachi WA12212970WWW using CAN", true, true);
                    result = write_mem(test_write);
                }
            }
            emit external_logger("Finished");

            if (result == STATUS_SUCCESS)
            {
                QMessageBox::information(this, tr("ECU Operation"), "ECU operation was succesful, press OK to exit");
                this->close();
            }
            else
            {
                QMessageBox::warning(this, tr("ECU Operation"), "ECU operation failed, press OK to exit and try again");
            }
            break;
        case QMessageBox::Cancel:
            qDebug() << "Operation canceled";
            this->close();
            break;
        default:
            QMessageBox::warning(this, tr("Connecting to ECU"), "Unknown operation selected!");
            qDebug() << "Unknown operation selected!";
            this->close();
            break;
    }
}

FlashEcuSubaruHitachiM32rCan::~FlashEcuSubaruHitachiM32rCan()
{
    delete ui;
}

void FlashEcuSubaruHitachiM32rCan::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

/*
 * Connect to Subaru TCU Hitachi CAN bootloader
 *
 * @return success
 */
int FlashEcuSubaruHitachiM32rCan::connect_bootloader()
{
    QByteArray output;
    QByteArray received;
    QByteArray seed;
    QByteArray seed_key;
    QByteArray response;
    QString msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    send_log_window_message("Checking if OBK is still running...", true, true);
    qDebug() << "Checking if OBK is still running...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0xB7);
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(50);
    received = serial->read_serial_data(100, serial_read_short_timeout);
    send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
    qDebug() << "Response:" << parse_message_to_hex(received);
    if (received.length() > 6)
    {
        if ((uint8_t)received.at(4) == 0x7F && (uint8_t)received.at(5) == 0xB7 && (uint8_t)received.at(6) == 0x13)
        {
            send_log_window_message("OBK active, don't continue bootloader access...", true, true);
            qDebug() << "OBK active, don't continue bootloader access...";
            kernel_alive = true;
            return STATUS_SUCCESS;
        }
    }

    send_log_window_message("OBK not active, initialising ECU...", true, true);
    qDebug() << "OBK not active, initialising ECU...";

    send_log_window_message("Requesting ECU ID...", true, true);
    qDebug() << "Requesting ECU ID...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x09);
    output.append((uint8_t)0x04);
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(50);
    received = serial->read_serial_data(100, serial_read_short_timeout);
    send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
    qDebug() << "Response:" << parse_message_to_hex(received);
    if (received.length() > 14)
    {
        if ((uint8_t)received.at(4) != 0x49 || (uint8_t)received.at(5) != 0x04)
        {
            send_log_window_message("Bad response from ECU", true, true);
            qDebug() << "Bad response from ECU";
            return STATUS_ERROR;
        }
    }
    else
    {
        send_log_window_message("Bad response from ECU", true, true);
        qDebug() << "Bad response from ECU";
        return STATUS_ERROR;
    }
    send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
    qDebug() << "Response:" << parse_message_to_hex(received);

    response = received;
    response.remove(0, 7);
    response.remove(8, received.length() - 1);
    QString ecuid = QString::fromUtf8(response);
    send_log_window_message("Init Success: ECU ID = " + ecuid, true, true);

    send_log_window_message("Requesting CAL ID...", true, true);
    qDebug() << "Requesting CAL ID...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0xAA);
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(500);
    received = serial->read_serial_data(100, serial_read_short_timeout);
    send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
    qDebug() << "Response:" << parse_message_to_hex(received);
    if (received.length() > 12)
    {
        if ((uint8_t)received.at(4) != 0xEA)
        {
            send_log_window_message("Bad response from ECU", true, true);
            qDebug() << "Bad response from ECU";
            return STATUS_ERROR;
        }
    }
    else
    {
        send_log_window_message("Bad response from ECU", true, true);
        qDebug() << "Bad response from ECU";
        return STATUS_ERROR;
    }

    response.clear();
    response.append(received);
    response.remove(0, 8);
    response.remove(5, received.length() - 1);

    QString calid;// = QString::fromUtf8(response);
    for (int i = 0; i < response.length(); i++)
        calid.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
    send_log_window_message("Init Success: CAL ID = " + calid, true, true);
    qDebug() << "Init Success: CAL ID = " + calid;

    send_log_window_message("Initializing bootloader...", true, true);
    qDebug() << "Initializing bootloader...";

    //On car session hack
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0xA8);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0xD7);
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(50);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
    qDebug() << "Response:" << parse_message_to_hex(received);

    if (received.length() > 5)
    {
        if ((uint8_t)received.at(5) != 0xA0 && (uint8_t)received.at(6) != 0x20)
        {
            send_log_window_message("In Car Programming, Accessing...", true, true);
            delay(800);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0xA8);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x01);
            output.append((uint8_t)0x3B);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(50);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xDF);
            output.append((uint8_t)0x10);
            output.append((uint8_t)0x03);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(50);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE1);
            output.append((uint8_t)0x04);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(50);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xB0);
            output.append((uint8_t)0x10);
            output.append((uint8_t)0x03);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(50);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xB0);
            output.append((uint8_t)0x85);
            output.append((uint8_t)0x02);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(50);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xDF);
            output.append((uint8_t)0x85);
            output.append((uint8_t)0x02);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(50);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xB0);
            output.append((uint8_t)0x85);
            output.append((uint8_t)0x02);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(50);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xDF);
            output.append((uint8_t)0x85);
            output.append((uint8_t)0x02);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(50);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xDF);
            output.append((uint8_t)0x28);
            output.append((uint8_t)0x03);
            output.append((uint8_t)0x01);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(50);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);

            send_log_window_message("Sending seed request...", true, true);
            qDebug() << "Sending seed request...";
            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x27);
            output.append((uint8_t)0x01);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(200);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);
            if (received.length() > 5)
            {
                if ((uint8_t)received.at(4) != 0x67 || (uint8_t)received.at(5) != 0x01)
                {
                    send_log_window_message("Bad response to seed request", true, true);
                    qDebug() << "Bad response to seed request";
                    return STATUS_ERROR;
                }
            }
            else
            {
                send_log_window_message("Bad response to seed request", true, true);
                qDebug() << "Bad response to seed request";
                return STATUS_ERROR;
            }
            send_log_window_message("Seed request ok", true, true);
            qDebug() << "Seed request ok";

            seed.clear();
            seed.append(received.at(6));
            seed.append(received.at(7));
            seed.append(received.at(8));
            seed.append(received.at(9));

            msg = parse_message_to_hex(seed);
            emit LOG_I("Received seed: " + msg, true, true);

            seed_key = generate_seed_key(seed);

            msg = parse_message_to_hex(seed_key);
            emit LOG_I("Calculated seed key: " + msg, true, true);

            send_log_window_message("Sending seed key to ECU", true, true);
            qDebug() << "Sending seed key...";
            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x27);
            output.append((uint8_t)0x02);
            output.append(seed_key);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(200);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);
            if (received.length() > 5)
            {
                if ((uint8_t)received.at(4) != 0x67 || (uint8_t)received.at(5) != 0x02)
                {
                    send_log_window_message("Failed! Possible wrong seed key!", true, true);
                    qDebug() << "Failed! Possible wrong seed key!";
                    return STATUS_ERROR;
                }
            }
            else
            {
                send_log_window_message("Failed! Possible wrong seed key!", true, true);
                qDebug() << "Failed! Possible wrong seed key!";
                return STATUS_ERROR;
            }
            send_log_window_message("Seed key ok", true, true);
            qDebug() << "Seed key ok";

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0xA8);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0xD5);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(50);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0xA8);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x01);
            output.append((uint8_t)0x3B);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(50);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0xA8);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x1C);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(50);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0xA8);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x0E);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x0F);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(50);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x10);
            output.append((uint8_t)0x02);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(50);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);

            send_log_window_message("Checking if jump successful and kernel alive...", true, true);
            qDebug() << "Checking if jump successful and kernel alive...";
            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x34);
            output.append((uint8_t)0x04);
            output.append((uint8_t)0x33);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x08);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(200);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);
            if (received.length() > 7)
            {
                if ((uint8_t)received.at(4) != 0x74 || (uint8_t)received.at(5) != 0x20 || (uint8_t)received.at(6) != 0x01 || (uint8_t)received.at(7) != 0x04)
                {
                    send_log_window_message("Kernel not verified to be running!", true, true);
                    qDebug() << "Kernel not verified to be running!";
                    return STATUS_ERROR;
                }
            }
            else
            {
                send_log_window_message("Kernel not verified to be running!", true, true);
                qDebug() << "Kernel not verified to be running!";
                return STATUS_ERROR;
            }
        }
        else
        {
            send_log_window_message("Bench Programming, Accessing...", true, true);
            delay(800);

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x10);
            output.append((uint8_t)0x43);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(200);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);
            if (received.length() > 5)
            {
                if ((uint8_t)received.at(4) != 0x50 || (uint8_t)received.at(5) != 0x43)
                {
                    send_log_window_message("Failed to initialise bootloader!", true, true);
                    qDebug() << "Failed to initialise bootloader!";
                    return STATUS_ERROR;
                }
            }
            else
            {
                send_log_window_message("Failed to initialise bootloader!", true, true);
                qDebug() << "Failed to initialise bootloader!";
                return STATUS_ERROR;
            }

            send_log_window_message("Starting seed request...", true, true);
            qDebug() << "Starting seed request...";

            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x27);
            output.append((uint8_t)0x01);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(200);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);
            if (received.length() > 5)
            {
                if ((uint8_t)received.at(4) != 0x67 || (uint8_t)received.at(5) != 0x01)
                {
                    send_log_window_message("Bad response to seed request!", true, true);
                    qDebug() << "Bad response to seed request!";
                    return STATUS_ERROR;
                }
            }
            else
            {
                send_log_window_message("Bad response to seed request!", true, true);
                qDebug() << "Bad response to seed request!";
                return STATUS_ERROR;
            }
            send_log_window_message("Seed request ok", true, true);
            qDebug() << "Seed request ok";

            seed.clear();
            seed.append(received.at(6));
            seed.append(received.at(7));
            seed.append(received.at(8));
            seed.append(received.at(9));

            msg = parse_message_to_hex(seed);
            emit LOG_I("Received seed: " + msg, true, true);

            seed_key = generate_seed_key(seed);

            msg = parse_message_to_hex(seed_key);
            emit LOG_I("Calculated seed key: " + msg, true, true);

            emit LOG_I("Sending seed key to ECU...", true, true);
            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x27);
            output.append((uint8_t)0x02);
            output.append(seed_key);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(200);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);
            if (received.length() > 5)
            {
                if ((uint8_t)received.at(4) != 0x67 || (uint8_t)received.at(5) != 0x02)
                {
                    send_log_window_message("Failed! Possible wrong seed key!", true, true);
                    qDebug() << "Failed! Possible wrong seed key!";
                    return STATUS_ERROR;
                }
            }
            else
            {
                send_log_window_message("Failed! Possible wrong seed key!", true, true);
                qDebug() << "Failed! Possible wrong seed key!";
                return STATUS_ERROR;
            }
            send_log_window_message("Seed key ok", true, true);
            qDebug() << "Seed key ok";

            send_log_window_message("Jumping to onboad kernel...", true, true);
            qDebug() << "Jumping to onboad kernel...";
            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x10);
            output.append((uint8_t)0x42);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(200);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);
            if (received.length() > 5)
            {
                if ((uint8_t)received.at(4) != 0x50 || (uint8_t)received.at(5) != 0x42)
                {
                    send_log_window_message("Bad response to jumping to onboard kernel", true, true);
                    qDebug() << "Bad response to jumping to onboard kernel";
                    return STATUS_ERROR;
                }
            }
            else
            {
                send_log_window_message("Bad response to jumping to onboard kernel", true, true);
                qDebug() << "Bad response to jumping to onboard kernel";
                return STATUS_ERROR;
            }
            send_log_window_message("Jump to kernel ok", true, true);
            qDebug() << "Jump to kernel ok";

            send_log_window_message("Checking if jump successful and kernel alive...", true, true);
            qDebug() << "Checking if jump successful and kernel alive...";
            output.clear();
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x07);
            output.append((uint8_t)0xE0);
            output.append((uint8_t)0x34);
            output.append((uint8_t)0x04);
            output.append((uint8_t)0x33);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x08);
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x00);
            serial->write_serial_data_echo_check(output);
            send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
            qDebug() << "Sent:" << parse_message_to_hex(output);
            delay(200);
            received = serial->read_serial_data(20, 200);
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
            qDebug() << "Response:" << parse_message_to_hex(received);
            if (received.length() > 7)
            {
                if ((uint8_t)received.at(4) != 0x74 || (uint8_t)received.at(5) != 0x20 || (uint8_t)received.at(6) != 0x01 || (uint8_t)received.at(7) != 0x04)
                {
                    send_log_window_message("Kernel not verified to be running!", true, true);
                    qDebug() << "Kernel not verified to be running!";
                    return STATUS_ERROR;
                }
            }
            else
            {
                send_log_window_message("Kernel not verified to be running!", true, true);
                qDebug() << "Kernel not verified to be running!";
                return STATUS_ERROR;
            }
        }
        send_log_window_message("ECU inittialised, continue...", true, true);
        qDebug() << "ECU inittialised, continue...";

        kernel_alive = true;
        return STATUS_SUCCESS;
    }
    return STATUS_ERROR;
}

/*
 * Read memory from Subaru TCU Hitachi CAN bootloader
 *
 * @return success
 */
int FlashEcuSubaruHitachiM32rCan::read_mem(uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray pagedata;
    QByteArray mapdata;
    uint32_t cplen = 0;
    uint32_t timeout = 0;

    uint32_t pagesize = 0x100;

    start_addr = 0x0;
    length = 0x80000;    // hack for testing

    uint32_t skip_start = start_addr & (pagesize - 1); //if unaligned, we'll be receiving this many extra bytes
    uint32_t addr = start_addr - skip_start;
    uint32_t willget = (skip_start + length + pagesize - 1) & ~(pagesize - 1);
    uint32_t len_done = 0;  //total data written to file

    send_log_window_message("Settting dump start & length...", true, true);
    qDebug() << "Settting dump start & length...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x35);
    output.append((uint8_t)0x04);
    output.append((uint8_t)0x33);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x08);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
    qDebug() << "Response:" << parse_message_to_hex(received);
    if (received.length() > 7)
    {
        if ((uint8_t)received.at(4) != 0x75 || (uint8_t)received.at(5) != 0x20 || (uint8_t)received.at(6) != 0x01 || (uint8_t)received.at(7) != 0x01)
        {
            send_log_window_message("Bad response to setting dump start & length", true, true);

            //return STATUS_ERROR;
        }
    }

    send_log_window_message("Start reading ROM, please wait...", true, true);
    qDebug() << "Start reading ROM, please wait...";

    // send 0xB7 command to kernel to dump from ROM
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0xB7);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    set_progressbar_value(0);

    mapdata.clear();

    while (willget)
    {
        if (kill_process)
            return STATUS_ERROR;

        uint32_t numblocks = 1;
        unsigned curspeed = 0, tleft;
        float pleft = 0;
        unsigned long chrono;

        pleft = (float)(addr - start_addr) / (float)length * 100.0f;
        set_progressbar_value(pleft);

        //length = 7FFF0;

        output[5] = (uint8_t)((addr >> 16) & 0xFF);
        output[6] = (uint8_t)((addr >> 8) & 0xFF);
        output[7] = (uint8_t)(addr & 0xFF);
        serial->write_serial_data_echo_check(output);
        emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
        received = serial->read_serial_data(20, serial_read_short_timeout);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        if (received.length() > 4)
        {
            if ((uint8_t)received.at(4) != 0xF7)
            {
                send_log_window_message("Page data request failed!" + parse_message_to_hex(received), true, true);
                qDebug() << "Page data request failed!" + parse_message_to_hex(received);
                return STATUS_ERROR;
            }
        }
        else
        {
            send_log_window_message("Page data request failed!" + parse_message_to_hex(received), true, true);
            qDebug() << "Page data request failed!" + parse_message_to_hex(received);
            return STATUS_ERROR;
        }
        pagedata.clear();
        received.remove(0, 5);
        received = decrypt_payload(received, received.length());
        //emit LOG_D("Decrypted response: " + parse_message_to_hex(received), true, true);
        mapdata.append(received);

        // don't count skipped first bytes //
        cplen = (numblocks * pagesize) - skip_start; //this is the actual # of valid bytes in buf[]
        skip_start = 0;

        chrono = timer.elapsed();
        timer.start();

        if (cplen > 0 && chrono > 0)
            curspeed = cplen * (1000.0f / chrono);

        if (!curspeed) {
            curspeed += 1;
        }

        tleft = (willget / curspeed) % 9999;
        tleft++;

        QString start_address = QString("%1").arg(addr,8,16,QLatin1Char('0')).toUpper();
        QString block_len = QString("%1").arg(pagesize,8,16,QLatin1Char('0')).toUpper();
        msg = QString("Kernel read addr:  0x%1  length:  0x%2,  %3  B/s  %4 s remaining").arg(start_address).arg(block_len).arg(curspeed, 6, 10, QLatin1Char(' ')).arg(tleft, 6, 10, QLatin1Char(' ')).toUtf8();
        emit LOG_I(msg, true, true);
        //delay(1);

        // and drop extra bytes at the end //
        uint32_t extrabytes = (cplen + len_done);   //hypothetical new length
        if (extrabytes > length) {
            cplen -= (extrabytes - length);
            //thus, (len_done + cplen) will not exceed len
        }

        // increment addr, len, etc //
        len_done += cplen;
        addr += (numblocks * pagesize);
        willget -= (numblocks * pagesize);
    }

    send_log_window_message("ROM read complete", true, true);
    qDebug() << "ROM read complete";

    QString mapdata_size = QString("0x%1").arg(mapdata.length(),8,16,QLatin1Char('0')).toUpper();
    send_log_window_message("Mapdata size: " + mapdata_size, true, true);
    qDebug() << "Mapdata size: " + mapdata_size;

    send_log_window_message("Sending stop command...", true, true);
    qDebug() << "Sending stop command...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x37);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(200);
    received = serial->read_serial_data(20, 200);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(4) != 0x77)
        {
            send_log_window_message("ROM read stop request failed!", true, true);
            qDebug() << "ROM read stop request failed!";
            //return STATUS_ERROR;
        }
    }

    //mapdata = decrypt_payload(mapdata, mapdata.length());

    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*
 * Write memory to Subaru Hitachi CAN 32bit TCUs, on board kernel
 *
 * @return success
 */

int FlashEcuSubaruHitachiM32rCan::write_mem(bool test_write)
{
    QByteArray filedata;

    filedata = ecuCalDef->FullRomData;

    QScopedArrayPointer<uint8_t> data_array(new uint8_t[filedata.length()]);
    //uint8_t* data_array = new uint8_t[filedata.length()];

    int block_modified[16] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};   // assume blocks after 0x8000 are modified

    unsigned bcnt;  // 13 blocks in M32R_512kB, but kernelmemorymodels.h has 11. Of these 11, the first 3 are not flashed by OBK
    unsigned blockno;

    //encrypt the data
    filedata = encrypt_payload(filedata, filedata.length());

    for (int i = 0; i < filedata.length(); i++)
    {
        data_array[i] = filedata.at(i);
    }

    bcnt = 0;
    send_log_window_message("Blocks to flash: ", true, false);
    for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++) {
        if (block_modified[blockno]) {
            send_log_window_message(QString::number(blockno) + ", ", false, false);
            bcnt += 1;
        }
    }
    send_log_window_message(" (total: " + QString::number(bcnt) + ")", false, true);

    if (bcnt)
    {
        send_log_window_message("--- Erasing ECU flash memory ---", true, true);
        if (erase_memory())
        {
            send_log_window_message("--- Erasing did not complete successfully ---", true, true);
            return STATUS_ERROR;
        }

        flashbytescount = 0;
        flashbytesindex = 0;

        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++)
        {
            if (block_modified[blockno])
            {
                flashbytescount += flashdevices[mcu_type_index].fblocks[blockno].len;
            }
        }

        send_log_window_message("--- Start writing ROM file to ECU flash memory ---", true, true);
        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++)  // hack so that only 1 flash loop done for the entire ROM above 0x8000
        {
            if (block_modified[blockno])
            {
                if (reflash_block(&data_array[flashdevices[mcu_type_index].fblocks->start], &flashdevices[mcu_type_index], blockno, test_write))
                {
                    send_log_window_message("Block " + QString::number(blockno) + " reflash failed.", true, true);
                    return STATUS_ERROR;
                }
                else
                {
                    flashbytesindex += flashdevices[mcu_type_index].fblocks[blockno].len;
                    send_log_window_message("Block " + QString::number(blockno) + " reflash complete.", true, true);
                }
            }
        }

    }
    else
    {
        send_log_window_message("*** No blocks require flash! ***", true, true);
    }

    return STATUS_SUCCESS;
}

/*
 * Upload kernel to Subaru Denso CAN (iso15765) 32bit ECUs
 *
 * @return success
 */
int FlashEcuSubaruHitachiM32rCan::reflash_block(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write)
{

    int errval;

    uint32_t start_address, end_addr;
    uint32_t pl_len;
    uint16_t maxblocks;
    uint16_t blockctr;
    uint32_t blockaddr;

    uint32_t pagesize = 0x100;

    QByteArray output;
    QByteArray received;
    QByteArray msg;

    set_progressbar_value(0);

    if (blockno >= fdt->numblocks) {
        emit LOG_D("block " + QString::number(blockno) + " out of range !", true, true);
        return -1;
    }

    start_address = fdt->fblocks[blockno].start;
    pl_len = fdt->fblocks[blockno].len;
    maxblocks = pl_len / 256;
    end_addr = (start_address + (maxblocks * 256)) & 0xFFFFFFFF;
    uint32_t data_len = end_addr - start_address;

    QString start_addr = QString("%1").arg((uint32_t)start_address,8,16,QLatin1Char('0')).toUpper();
    QString length = QString("%1").arg((uint32_t)pl_len,8,16,QLatin1Char('0')).toUpper();
    msg = QString("Flash block addr: 0x" + start_addr + " len: 0x" + length).toUtf8();
    emit LOG_I(msg, true, true);

    emit LOG_I("Setting flash start & length...", true, true);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x34);
    output.append((uint8_t)0x04);
    output.append((uint8_t)0x33);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x08);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
    qDebug() << "Response:" << parse_message_to_hex(received);
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(4) != 0x74)
        {
            send_log_window_message("No or bad response received", true, true);
            return STATUS_ERROR;
        }
    }

    int data_bytes_sent = 0;
    for (blockctr = 0; blockctr < maxblocks; blockctr++)
    {
        if (kill_process)
            return 0;

        blockaddr = start_address + blockctr * 256;
        output.clear();
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x07);
        output.append((uint8_t)0xE0);
        output.append((uint8_t)0xB6);
        output.append((uint8_t)(blockaddr >> 16) & 0xFF);
        output.append((uint8_t)(blockaddr >> 8) & 0xFF);
        output.append((uint8_t)blockaddr & 0xFF);


        //        qDebug() << "Data header:" << parse_message_to_hex(output);
        for (int i = 0; i < 256; i++)
        {
            output.append((uint8_t)(newdata[i + blockaddr] & 0xFF));
            data_bytes_sent++;
        }
        data_len -= 256;

        serial->write_serial_data_echo_check(output);
        //send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        //delay(200);
        received = serial->read_serial_data(20, receive_timeout);
        //send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
        qDebug() << "Response:" << parse_message_to_hex(received);
        if (received.length() > 4)
        {
            if ((uint8_t)received.at(4) != 0xF6)
            {
                send_log_window_message("Write data failed!", true, true);
                return STATUS_ERROR;
            }
        }

        float pleft = (float)blockctr / (float)maxblocks * 100;
        set_progressbar_value(pleft);

    }

    set_progressbar_value(100);

    send_log_window_message("Closing out Flashing of this block...", true, true);
    qDebug() << "Closing out Flashing of this block...";
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x37);

    uint8_t try_count = 0;
    bool connected = false;
    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        delay(200);
        received = serial->read_serial_data(20, 200);
        if (received.length() > 4)
        {
            if ((uint8_t)received.at(4) != 0x77)
            {
                send_log_window_message("." + parse_message_to_hex(received), false, false);
            }
            else if ((uint8_t)received.at(4) == 0x77)
            {
                connected = true;
                send_log_window_message("", false, true);
                send_log_window_message("Closed succesfully: " + parse_message_to_hex(received), true, true);
                qDebug() << "Closed succesfully: " + parse_message_to_hex(received);
            }
        }
        try_count++;
    }

    delay(100);

    send_log_window_message("Verifying checksum...", true, true);
    qDebug() << "Verifying checksum...";
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x01);
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(1000);
    received = serial->read_serial_data(20, 500);
    send_log_window_message(QString::number(try_count) + ": 0x31 response: " + parse_message_to_hex(received), true, true);
    if (received.length() != 7)
    {
        send_log_window_message("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
        qDebug() << "Wrong response from ECU: " + parse_message_to_hex(received);
        send_log_window_message("Checksum not verified", true, true);
        qDebug() << "Checksum not verified";
        return STATUS_ERROR;
    }
    else
    {
        if ((uint8_t)received.at(4) != 0x7F || (uint8_t)received.at(5) != 0x31 || (uint8_t)received.at(6) != 0x78)
        {
            send_log_window_message("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
            qDebug() << "Wrong response from ECU: " + parse_message_to_hex(received);
            send_log_window_message("Checksum not verified", true, true);
            qDebug() << "Checksum not verified";
            return STATUS_ERROR;
        }
        else
        {
            delay(200);
            received = serial->read_serial_data(20, 500);
            send_log_window_message(QString::number(try_count) + ": 0x31 response: " + parse_message_to_hex(received), true, true);
            qDebug() << QString::number(try_count) + ": 0x31 response: " + parse_message_to_hex(received);

            if (received.length() > 6)
            {
                if ((uint8_t)received.at(4) != 0x71 || (uint8_t)received.at(5) != 0x01 || (uint8_t)received.at(6) != 0x02)
                {
                    send_log_window_message("ROM checksum error: " + parse_message_to_hex(received), true, true);
                    qDebug() << "ROM checksum error: " + parse_message_to_hex(received);
                    return STATUS_ERROR;
                }
            }
            else
            {
                send_log_window_message("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
                qDebug() << "Wrong response from ECU: " + parse_message_to_hex(received);
                return STATUS_ERROR;
            }
        }
        send_log_window_message("Checksum verified", true, true);
        qDebug() << "Checksum verified";
    }

    return STATUS_SUCCESS;
}

/*
 * Erase Subaru Hitachi TCU CAN (iso15765)
 *
 * @return success
 */
int FlashEcuSubaruHitachiM32rCan::erase_memory()
{
    QByteArray output;
    QByteArray received;

    bool connected = false;
    int try_count = 0;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    send_log_window_message("Erasing ECU ROM...", true, true);
    //    qDebug() << "Erasing ECU ROM...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x0f);
    output.append((uint8_t)0xff);
    output.append((uint8_t)0xff);
    output.append((uint8_t)0xff);
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
    qDebug() << "Response:" << parse_message_to_hex(received);
    while (try_count < 20 && connected == false)
    {
        received = serial->read_serial_data(20, 500);
        if (received.length() > 6)
        {
            if ((uint8_t)received.at(4) != 0x71 || (uint8_t)received.at(5) != 0x01 || (uint8_t)received.at(6) != 0x02)
            {
                send_log_window_message(".", false, false);
            }
            else if ((uint8_t)received.at(4) == 0x71 && (uint8_t)received.at(5) == 0x01 && (uint8_t)received.at(6) == 0x02)
            {
                connected = true;
                send_log_window_message("", false, true);
            }
        }
        else
        {
            send_log_window_message(".", false, false);
        }
        delay(500);
        try_count++;
    }
    if (!connected)
    {
        send_log_window_message("Flash area erase failed: " + parse_message_to_hex(received), true, true);
        qDebug() << "Flash area erase failed: " + parse_message_to_hex(received);
        return STATUS_ERROR;
    }

    send_log_window_message("Flash erased! Starting flash write, do not power off!", true, true);
    qDebug() << "Flash erased! Starting flash write, do not power off!";

    return STATUS_SUCCESS;
}


/*
 * Generate tcu hitachi can seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashEcuSubaruHitachiM32rCan::generate_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex[]={
        0x90A1, 0x2F92, 0xDE3C, 0xCDC0,
        0x1A99, 0x437C, 0xF91B, 0xDB57,
        0x96BA, 0xDE10, 0xFCAF, 0x3F31,
        0xF47F, 0x0BB6, 0x16E9, 0x4645
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = calculate_seed_key(requested_seed, keytogenerateindex, indextransformation);

    return key;
}

/*
 * Calculate subaru tcu hitachi seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashEcuSubaruHitachiM32rCan::calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
{
    QByteArray key;

    uint32_t seed, index;
    uint16_t wordtogenerateindex, wordtobeencrypted, encryptionkey;
    int ki, n;

    seed = (requested_seed.at(0) << 24) & 0xFF000000;
    seed += (requested_seed.at(1) << 16) & 0x00FF0000;
    seed += (requested_seed.at(2) << 8) & 0x0000FF00;
    seed += requested_seed.at(3) & 0x000000FF;
    //seed = reconst_32(seed8);

    for (ki = 15; ki >= 0; ki--) {

        wordtogenerateindex = seed;
        wordtobeencrypted = seed >> 16;
        index = wordtogenerateindex ^ keytogenerateindex[ki];
        index += index << 16;
        encryptionkey = 0;

        for (n = 0; n < 4; n++) {
            encryptionkey += indextransformation[(index >> (n * 4)) & 0x1F] << (n * 4);
        }

        encryptionkey = (encryptionkey >> 3) + (encryptionkey << 13);
        seed = (encryptionkey ^ wordtobeencrypted) + (wordtogenerateindex << 16);
    }

    seed = (seed >> 16) + (seed << 16);

    key.clear();
    key.append((uint8_t)(seed >> 24));
    key.append((uint8_t)(seed >> 16));
    key.append((uint8_t)(seed >> 8));
    key.append((uint8_t)seed);

    //write_32b(seed, key);

    return key;
}
/*
 * Encrypt upload data
 *
 * @return encrypted data
 */

QByteArray FlashEcuSubaruHitachiM32rCan::encrypt_payload(QByteArray buf, uint32_t len)
{
    QByteArray encrypted;

    const uint16_t keytogenerateindex[]={
        //        0xF50E, 0x973C, 0x77F4, 0x14CA
        0x14CA, 0x77F4, 0x973C, 0xF50E
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    encrypted = calculate_payload(buf, len, keytogenerateindex, indextransformation);

    return encrypted;
}

QByteArray FlashEcuSubaruHitachiM32rCan::decrypt_payload(QByteArray buf, uint32_t len)
{
    QByteArray decrypt;

    const uint16_t keytogenerateindex[]={
        //        0x14CA, 0x77F4, 0x973C, 0xF50E
        0xF50E, 0x973C, 0x77F4, 0x14CA

    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    decrypt = calculate_payload(buf, len, keytogenerateindex, indextransformation);

    return decrypt;
}

QByteArray FlashEcuSubaruHitachiM32rCan::calculate_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
{
    QByteArray encrypted;
    uint32_t datatoencrypt32, index;
    uint16_t wordtogenerateindex, wordtobeencrypted, encryptionkey;
    int ki, n;

    if (!buf.length() || !len) {
        return NULL;
    }

    encrypted.clear();

    len &= ~3;
    for (uint32_t i = 0; i < len; i += 4) {
        datatoencrypt32 = ((buf.at(i) << 24) & 0xFF000000) | ((buf.at(i + 1) << 16) & 0xFF0000) | ((buf.at(i + 2) << 8) & 0xFF00) | (buf.at(i + 3) & 0xFF);

        for (ki = 0; ki < 4; ki++) {

            wordtogenerateindex = datatoencrypt32;
            wordtobeencrypted = datatoencrypt32 >> 16;
            index = wordtogenerateindex ^ keytogenerateindex[ki];
            index += index << 16;
            encryptionkey = 0;

            for (n = 0; n < 4; n++) {
                encryptionkey += indextransformation[(index >> (n * 4)) & 0x1F] << (n * 4);
            }

            encryptionkey = (encryptionkey >> 3) + (encryptionkey << 13);
            datatoencrypt32 = (encryptionkey ^ wordtobeencrypted) + (wordtogenerateindex << 16);
        }

        datatoencrypt32 = (datatoencrypt32 >> 16) + (datatoencrypt32 << 16);

        encrypted.append((datatoencrypt32 >> 24) & 0xFF);
        encrypted.append((datatoencrypt32 >> 16) & 0xFF);
        encrypted.append((datatoencrypt32 >> 8) & 0xFF);
        encrypted.append(datatoencrypt32 & 0xFF);
        //encrypted.append(sub_encrypt(tempbuf));
    }
    return encrypted;
}

/*
 * Parse QByteArray to readable form
 *
 * @return parsed message
 */
QString FlashEcuSubaruHitachiM32rCan::parse_message_to_hex(QByteArray received)
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
int FlashEcuSubaruHitachiM32rCan::send_log_window_message(QString message, bool timestamp, bool linefeed)
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

void FlashEcuSubaruHitachiM32rCan::set_progressbar_value(int value)
{
    bool valueChanged = true;
    if (ui->progressbar)
    {
        valueChanged = ui->progressbar->value() != value;
        ui->progressbar->setValue(value);
    }
    if (valueChanged)
        emit external_logger(value);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void FlashEcuSubaruHitachiM32rCan::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

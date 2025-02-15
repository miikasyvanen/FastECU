#include "eeprom_ecu_subaru_denso_sh705x_can.h"

EepromEcuSubaruDensoSH705xCan::EepromEcuSubaruDensoSH705xCan(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
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

void EepromEcuSubaruDensoSH705xCan::run()
{
    this->show();

    int result = STATUS_ERROR;
    set_progressbar_value(0);

    //result = init_flash_denso_subarucan(ecuCalDef, cmd_type);

    EEPROM_MODE = 2;

    bool ok = false;

    mcu_type_string = ecuCalDef->McuType;
    mcu_type_index = 0;

    while (flashdevices[mcu_type_index].name != 0)
    {
        if (flashdevices[mcu_type_index].name == mcu_type_string)
            break;
        mcu_type_index++;
    }
    QString mcu_name = flashdevices[mcu_type_index].name;
    emit LOG_D("MCU type: " + mcu_name + " " + mcu_type_string + " and index: " + mcu_type_index, true, true);

    kernel = ecuCalDef->Kernel;
    flash_method = ecuCalDef->FlashMethod;

    emit external_logger("Starting");

    if (cmd_type == "read")
    {
        emit LOG_I("Read memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
    }
    else if (cmd_type == "test_write")
    {
        test_write = true;
        emit LOG_I("Test write memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
    }
    else if (cmd_type == "write")
    {
        test_write = false;
        emit LOG_I("Write memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
    }

    // Set serial port
    serial->set_is_iso14230_connection(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(true);
    serial->set_is_29_bit_id(false);
    serial->set_can_speed("500000");
    serial->set_can_source_address(0x7e0);
    serial->set_can_destination_address(0x7e8);
    serial->set_iso15765_source_address(0x7e0);
    serial->set_iso15765_destination_address(0x7e8);
    // Open serial port
    serial->open_serial_port();

    int ret = QMessageBox::information(this, tr("Connecting to ECU"),
                                       tr("Downloading EEPROM content. There is 3 different option depends on "
                                          "ECU. All 3 option shows content on screen and you can save it when "
                                          "it looks ok.\n\n"
                                          "Turn ignition ON and press OK to start initializing connection to ECU"),
                                       QMessageBox::Ok | QMessageBox::Cancel,
                                       QMessageBox::Ok);

    switch (ret)
    {
    case QMessageBox::Ok:
        for (int i = 2; i < 5; i++)
        {
            bool save_and_exit = false;

            emit external_logger("Preparing, please wait...");
            emit LOG_I("Connecting to Subaru 07+ 32-bit CAN bootloader, please wait...", true, true);
            result = connect_bootloader();

            if (result == STATUS_SUCCESS && !kernel_alive)
            {
                emit LOG_I("Initializing Subaru 07+ 32-bit CAN kernel upload, please wait...", true, true);
                result = upload_kernel(kernel, ecuCalDef->KernelStartAddr.toUInt(&ok, 16));
            }
            if (result == STATUS_SUCCESS)
            {
                if (cmd_type == "read")
                {
                    emit external_logger("Reading EEPROM, please wait...");
                    emit LOG_I("Reading EEPROM from Subaru 07+ 32-bit using CAN", true, true);
                    result = read_mem(flashdevices[mcu_type_index].eblocks[0].start, flashdevices[mcu_type_index].eblocks[0].len);
                }
                else if (cmd_type == "test_write" || cmd_type == "write")
                {
                    emit external_logger("Writing EEPROM, please wait...");
                    emit LOG_I("Writing ROM to Subaru 07+ 32-bit using CAN", true, true);
                    //result = write_mem(ecuCalDef, test_write);
                }
            }
            emit external_logger("Finished");
            if (result == STATUS_SUCCESS)
            {
                int ret = QMessageBox::information(this, tr("Downloaded EEPROM content"),
                                                   tr("If downloaded content looks ok, click 'Save' to save content and exit, otherwise click 'discard' and continue with next method."),
                                                   QMessageBox::Save | QMessageBox::Ignore,
                                                   QMessageBox::Save);

                switch (ret)
                {
                case QMessageBox::Save:
                    save_and_exit = true;
                    break;
                case QMessageBox::Ignore: {
                    result = STATUS_ERROR;
                    ecuCalDef->FullRomData.clear();
                    EEPROM_MODE++;
                    int ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                                   tr("Turn ignition OFF and back ON and press OK to start initializing connection to ECU"),
                                                   QMessageBox::Ok | QMessageBox::Cancel,
                                                   QMessageBox::Ok);

                    switch (ret)
                    {
                    case QMessageBox::Ok:

                        break;
                    case QMessageBox::Cancel:
                        save_and_exit = true;
                        break;
                    default:
                        // should never be reached
                        break;
                    }
                    break;
                }
                default:
                    // should never be reached
                    break;
                }
            }
            if (save_and_exit)
                break;
        }
    case QMessageBox::Cancel:
        emit LOG_D("Operation canceled", true, true);
        this->close();
        break;
    default:
        QMessageBox::warning(this, tr("Connecting to ECU"), "Unknown operation selected!");
        emit LOG_D("Unknown operation selected!", true, true);
        this->close();
        break;
    }
    if (result != STATUS_SUCCESS)
    {
        QMessageBox::warning(this, tr("ECU Operation"), "ECU operation failed, press OK to exit and try again");
    }
}

EepromEcuSubaruDensoSH705xCan::~EepromEcuSubaruDensoSH705xCan()
{
    delete ui;
}

void EepromEcuSubaruDensoSH705xCan::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

/*
 * Connect to Subaru Denso CAN (iso15765) bootloader 32bit ECUs
 *
 * @return success
 */
int EepromEcuSubaruDensoSH705xCan::connect_bootloader()
{
    QByteArray output;
    QByteArray received;
    QByteArray seed;
    QByteArray seed_key;
    QString msg;

    uint32_t ram_value = 0;

    if (!serial->is_serial_port_open())
    {
        emit LOG_E("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Checking if kernel is already running...", true, true);

    emit LOG_I("Requesting kernel ID", true, true);

    received.clear();
    received = request_kernel_id();
    if (received.length() > 8)
    {
        if ((uint8_t)received.at(4) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(5) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(8) != (SUB_KERNEL_ID | 0x40))
        {
            emit LOG_E("Wrong response from ECU: " + fileActions.parse_nrc_message(received.remove(0, 8)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
        else
        {
            received.remove(0, 9);
            emit LOG_I("Kernel ID: " + received, true, true);
            kernel_alive = true;
            return STATUS_SUCCESS;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    }

    emit LOG_I("No response from kernel, continue bootloader initialization...", true, true);

    if (flash_method.endsWith("_ecutek_racerom_alt"))
    {
        serial->reset_connection();
        serial->set_is_iso14230_connection(false);
        serial->set_is_can_connection(false);
        serial->set_is_iso15765_connection(false);
        serial->set_is_29_bit_id(false);
        serial->set_serial_port_baudrate("4800");
        serial->set_can_speed("500000");
        serial->set_can_source_address(0x7E0);
        serial->set_can_destination_address(0x7E8);
        serial->set_iso15765_source_address(0x7E0);
        serial->set_iso15765_destination_address(0x7E8);
        // Open serial port
        serial->open_serial_port();

        // AE5Z500V
        ram_value = read_ram_location(0xffff1ed8);
        emit LOG_D("Value at RAM loc 0xffff1ed8: 0x" + QString::number(ram_value, 16), true, true);
        seed_alter = ram_value;
        ram_value = read_ram_location(0xffff1e80);
        emit LOG_D("Value at RAM loc 0xffff1e80: 0x" + QString::number(ram_value, 16), true, true);
        xor_byte_1 = ((ram_value >> 8) & 0xff);
        xor_byte_2 = (ram_value & 0xff);
        /*
        // AE5I910V
        ram_value = read_ram_location(0xffffbba8);
        emit LOG_D("Value at RAM loc 0xffffbba8: 0x" + QString::number(ram_value, 16), true, true);
        seed_alter = ram_value;
        ram_value = read_ram_location(0xffffbbf0);
        emit LOG_D("Value at RAM loc 0xffffbbf0: 0x" + QString::number(ram_value, 16), true, true);
        xor_byte_1 = ((ram_value >> 8) & 0xff);
        xor_byte_2 = (ram_value & 0xff);
*/
        emit LOG_I("Seed alter is: 0x" + QString::number(seed_alter, 16), true, true);
        emit LOG_I("XOR values are: 0x" + QString::number(xor_byte_1, 16) + " and 0x" + QString::number(xor_byte_2, 16), true, true);
        serial->reset_connection();
        serial->set_is_iso15765_connection(true);
        serial->open_serial_port();
    }

    emit LOG_I("Initializing connection...", true, true);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x00);

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x41 && (uint8_t)received.at(5) == 0x00)
        {
            QByteArray response = received;
            response.remove(0, 6);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + fileActions.parse_nrc_message(received.remove(0, 8)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    }

    emit LOG_I("Requesting ECU ID", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0xAA);

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0xEA)
        {
            QByteArray response = received;
            response.remove(0, 8);
            response.remove(5, response.length()-5);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            QString ecuid;
            for (int i = 0; i < 5; i++)
                ecuid.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_I("ECU ID: " + ecuid, true, true);
            if (cmd_type == "read")
                ecuCalDef->RomId = ecuid + "_";
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + fileActions.parse_nrc_message(received.remove(0, 8)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    }

    emit LOG_I("Requesting VIN", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x09);
    output.append((uint8_t)0x02);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x49 && (uint8_t)received.at(5) == 0x02)
        {
            QByteArray response = received;
            response.remove(0, 7);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            emit LOG_I("VIN: " + response, true, true);
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + fileActions.parse_nrc_message(received.remove(0, 8)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    }

    emit LOG_I("Requesting CAL ID", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x09);
    output.append((uint8_t)0x04);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x49 && (uint8_t)received.at(5) == 0x04)
        {
            QByteArray response = received;
            response.remove(0, 7);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            emit LOG_I("CAL ID: " + response, true, true);
            if (cmd_type == "read")
                ecuCalDef->RomId.insert(0, QString(response) + "_");
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + fileActions.parse_nrc_message(received.remove(0, 8)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    }

    emit LOG_I("Requesting CVN", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x09);
    output.append((uint8_t)0x06);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x49 && (uint8_t)received.at(5) == 0x06)
        {
            QByteArray response = received;
            response.remove(0, 7);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            emit LOG_I("CVN: " + msg, true, true);
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + fileActions.parse_nrc_message(received.remove(0, 8)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    }

    bool req_10_03_connected = false;
    bool req_10_43_connected = false;

    emit LOG_I("Requesting session mode", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x10);
    output.append((uint8_t)0x03);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x50 && (uint8_t)received.at(5) == 0x03)
        {
            req_10_03_connected = true;
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + fileActions.parse_nrc_message(received.remove(0, 8)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    }

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x10);
    output.append((uint8_t)0x43);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x50 && (uint8_t)received.at(5) == 0x43)
        {
            req_10_43_connected = true;
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + fileActions.parse_nrc_message(received.remove(0, 8)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    }

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x27);
    output.append((uint8_t)0x01);

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x67 && (uint8_t)received.at(5) == 0x01)
        {
            QByteArray response = received;
            response.remove(0, 6);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + fileActions.parse_nrc_message(received.remove(0, 8)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Seed request ok", true, true);

    seed.clear();
    seed.append(received.at(6));
    seed.append(received.at(7));
    seed.append(received.at(8));
    seed.append(received.at(9));

    if (flash_method.endsWith("_ecutek_racerom_alt"))
        seed_key = generate_ecutek_seed_key(seed);
    else if (flash_method.endsWith("_ecutek_racerom"))
        seed_key = generate_ecutek_racerom_can_seed_key(seed);
    else if (flash_method.endsWith("_ecutek"))
        seed_key = generate_ecutek_seed_key(seed);
    else if (flash_method.endsWith("_cobb"))
        seed_key = generate_cobb_seed_key(seed);
    else
        seed_key = generate_seed_key(seed);

    emit LOG_I("Sending seed key", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x27);
    output.append((uint8_t)0x02);
    output.append(seed_key);

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x67 && (uint8_t)received.at(5) == 0x02)
        {
            QByteArray response = received;
            response.remove(0, 6);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + fileActions.parse_nrc_message(received.remove(0, 8)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Seed key ok", true, true);

    emit LOG_I("Set session mode", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x10);
    if (req_10_03_connected)
        output.append((uint8_t)0x02);
    if (req_10_43_connected)
        output.append((uint8_t)0x42);

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x50 && ((uint8_t)received.at(5) == 0x02 || (uint8_t)received.at(5) == 0x42))
        {
            QByteArray response = received;
            response.remove(0, 6);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + fileActions.parse_nrc_message(received.remove(0, 8)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }
    emit LOG_I("Succesfully set to programming session", true, true);

    return STATUS_SUCCESS;
}

uint32_t EepromEcuSubaruDensoSH705xCan::read_ram_location(uint32_t loc)
{
    QByteArray output;
    QByteArray received;
    QByteArray response;

    emit LOG_D("Reading RAM value at location: 0x" + QString::number(loc, 16), true, true);

    output.clear();
    if (serial->get_is_iso15765_connection())
    {
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x07);
        output.append((uint8_t)0xE0);
    }

    output.append((uint8_t)0xA8);
    output.append((uint8_t)0x00);
    for (int i = 0; i < 4; i++)
    {
        output.append((uint8_t)((loc >> 16) & 0xff));
        output.append((uint8_t)((loc >> 8) & 0xff));
        output.append((uint8_t)(loc & 0xff));
        loc++;
    }
    if (!serial->get_is_iso15765_connection())
        output = add_ssm_header(output, tester_id, target_id, false);

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    received = serial->read_serial_data(1, serial_read_timeout);
    response = received;
    /*
    if (!serial->get_is_iso15765_connection())
    {
        while(received.length()){
            received = serial->read_serial_data(1, serial_read_short_timeout);
            response.append(received);
        }
    }
    emit LOG_D("Response: " + parse_message_to_hex(response), true, false);
*/
    if (received.length() > 8)
    {
        if ((uint8_t)received.at(4) != 0xe8)
        {
            emit LOG_E("Wrong response from ECU: " + fileActions.parse_nrc_message(received.remove(0, 4)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    uint32_t result = 0;
    result = (response.at(5) << 24) & 0xFF000000;
    result += (response.at(6) << 16) & 0x00FF0000;
    result += (response.at(7) << 8) & 0x0000FF00;
    result += response.at(8) & 0x000000FF;

    return result;
}

/*
 * Upload kernel to Subaru Denso CAN (iso15765) 32bit ECUs
 *
 * @return success
 */
int EepromEcuSubaruDensoSH705xCan::upload_kernel(QString kernel, uint32_t kernel_start_addr)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray received;
    QByteArray pl_encr;
    uint32_t file_len = 0;
    uint32_t pl_len = 0;
    uint32_t start_address = 0;
    uint32_t end_addr = 0;
    uint32_t chk_sum = 0;
    uint32_t blockaddr = 0;
    uint16_t blockno = 0;
    uint16_t maxblocks = 0;

    start_address = kernel_start_addr;//flashdevices[mcu_type_index].kblocks->start;
    emit LOG_D("Start address to upload kernel: 0x" + QString::number(start_address, 16), true, true);

    if (!serial->is_serial_port_open())
    {
        emit LOG_E("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->set_add_iso14230_header(false);

    // Check kernel file
    if (!file.open(QIODevice::ReadOnly))
    {
        emit LOG_E("Unable to open kernel file for reading", true, true);
        return STATUS_ERROR;
    }
    file_len = file.size();
    pl_len = (file_len + 3) & ~3;
    pl_encr = file.readAll();
    maxblocks = pl_len / 128;
    if((pl_len % 128) != 0)
        maxblocks++;
    end_addr = (start_address + (maxblocks * 128)) & 0xFFFFFFFF;
    uint32_t data_len = end_addr - start_address;
    while ((uint32_t)pl_encr.length() < data_len)
        pl_encr.append((uint8_t)0x00);
    pl_encr.remove(pl_encr.length() - 4, 4);
    chk_sum = 0;
    for (int i = 0; i < pl_encr.length(); i+=4)
        chk_sum += ((pl_encr.at(i) << 24) & 0xFF000000) | ((pl_encr.at(i + 1) << 16) & 0xFF0000) | ((pl_encr.at(i + 2) << 8) & 0xFF00) | ((pl_encr.at(i + 3)) & 0xFF);
    chk_sum = 0x5aa5a55a - chk_sum;

    pl_encr.append((uint8_t)((chk_sum >> 24) & 0xFF));
    pl_encr.append((uint8_t)((chk_sum >> 16) & 0xFF));
    pl_encr.append((uint8_t)((chk_sum >> 8) & 0xFF));
    pl_encr.append((uint8_t)(chk_sum & 0xFF));
    pl_encr = encrypt_payload(pl_encr, pl_encr.length());
    //pl_encr = decrypt_payload(pl_encr, pl_encr.length());

    set_progressbar_value(0);

    emit LOG_I("Initialize kernel upload", true, true);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x34);
    output.append((uint8_t)0x04);
    output.append((uint8_t)0x33);
    output.append((uint8_t)((start_address >> 16) & 0xFF));
    output.append((uint8_t)((start_address >> 8) & 0xFF));
    output.append((uint8_t)(start_address & 0xFF));
    output.append((uint8_t)((data_len >> 16) & 0xFF));
    output.append((uint8_t)((data_len >> 8) & 0xFF));
    output.append((uint8_t)(data_len & 0xFF));

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, 10);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x74 && (uint8_t)received.at(5) == 0x20)
        {
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + fileActions.parse_nrc_message(received.remove(0, 8)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Uploading kernel, please wait...", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0xB6);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append(128, (uint8_t)0x00);

    int data_bytes_sent = 0;
    for (blockno = 0; blockno <= maxblocks; blockno++)
    {
        if (kill_process)
            return 0;

        blockaddr = start_address + blockno * 128;
        output.clear();
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x07);
        output.append((uint8_t)0xE0);
        output.append((uint8_t)0xB6);
        output.append((uint8_t)(blockaddr >> 16) & 0xFF);
        output.append((uint8_t)(blockaddr >> 8) & 0xFF);
        output.append((uint8_t)blockaddr & 0xFF);

        if (blockno == maxblocks)
        {
            for (uint32_t i = 0; i < data_len; i++)
            {
                output.append(pl_encr.at(i + blockno * 128));
                data_bytes_sent++;
            }
        }
        else
        {
            for (int i = 0; i < 128; i++)
            {
                output.append(pl_encr.at(i + blockno * 128));
                data_bytes_sent++;
            }
            data_len -= 128;
        }

        serial->write_serial_data_echo_check(output);
        received = serial->read_serial_data(5, receive_timeout);

        float pleft = (float)blockno / (float)maxblocks * 100;
        set_progressbar_value(pleft);
    }
    emit LOG_D("Data bytes sent: 0x" + QString::number(data_bytes_sent), true, true);

    emit LOG_I("Kernel uploaded, starting...", true, true);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x37);

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, 10);
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(4) == 0x77)
        {
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + fileActions.parse_nrc_message(received.remove(0, 8)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    delay(100);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x02);

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, 10);
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(4) == 0x71)
        {
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + fileActions.parse_nrc_message(received.remove(0, 8)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Kernel requesting kernel ID...", true, true);

    received.clear();
    received = request_kernel_id();
    if (received.length() > 8)
    {
        if ((uint8_t)received.at(4) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(5) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(8) != (SUB_KERNEL_ID | 0x40))
        {
            emit LOG_E("Wrong response from ECU: " + fileActions.parse_nrc_message(received.remove(0, 9)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
        else
        {
            received.remove(0, 9);
            emit LOG_I("Kernel ID: " + received, true, true);
            kernel_alive = true;
            return STATUS_SUCCESS;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    return STATUS_ERROR;
}

/*
 * Read memory from Subaru Denso CAN 32bit ECUs, nisprog kernel
 *
 * @return success
 */
int EepromEcuSubaruDensoSH705xCan::read_mem(uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray pagedata;
    QByteArray mapdata;
    uint32_t cplen = 0;
    uint32_t timeout = 0;
    uint32_t datalen = 6;
    uint32_t pagesize = 0x400;
    if (pagesize > length)
        pagesize = length;

    uint32_t skip_start = start_addr & (pagesize - 1); //if unaligned, we'll be receiving this many extra bytes
    uint32_t addr = start_addr - skip_start;
    uint32_t willget = (skip_start + length + pagesize - 1) & ~(pagesize - 1);
    uint32_t len_done = 0;  //total data written to file

    emit LOG_I("Start reading EEPROM, please wait..." + received, true, true);

    // send 0xD8 command to kernel to dump the chunk from ROM
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xe0);
    output.append((uint8_t)((SUB_KERNEL_START_COMM >> 8) & 0xFF));
    output.append((uint8_t)(SUB_KERNEL_START_COMM & 0xFF));
    output.append((uint8_t)((datalen + 1) >> 8) & 0xFF);
    output.append((uint8_t)(datalen + 1) & 0xFF);
    output.append((uint8_t)SUB_KERNEL_READ_EEPROM);
    output.append((uint8_t)EEPROM_MODE);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
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

        //uint32_t curblock = (addr / pagesize);


        pleft = (float)(addr - start_addr) / (float)length * 100.0f;
        set_progressbar_value(pleft);

        //length = 256;
        emit LOG_I("Read EEPROM start at: 0x" + QString::number(start_addr, 16) + " and size of 0x" + QString::number(pagesize, 16), true, true);

        output[10] = (uint8_t)((addr >> 16) & 0xFF);
        output[11] = (uint8_t)((addr >> 8) & 0xFF);
        output[12] = (uint8_t)((addr >> 0) & 0xFF);
        output[13] = (uint8_t)((pagesize >> 8) & 0xFF);
        output[14] = (uint8_t)((pagesize >> 0) & 0xFF);
        serial->write_serial_data_echo_check(output);
        emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
        //delay(100);
        received = serial->read_serial_data(1, serial_read_timeout);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

        if (received.length() > 8)
        {
            if ((uint8_t)received.at(4) == ((SUB_KERNEL_START_COMM >> 8) & 0xFF) && (uint8_t)received.at(5) == (SUB_KERNEL_START_COMM & 0xFF) && (uint8_t)received.at(8) == (SUB_KERNEL_READ_AREA | 0x40))
            {
                received.remove(0, 9);
                mapdata.append(received);
                //qDebug() << "DATA:" << addr << parse_message_to_hex(received);
            }
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }

        timeout = 0;
        pagedata.clear();
        while ((uint32_t)pagedata.length() < pagesize && timeout < 100)
        {
            if (kill_process)
                return STATUS_ERROR;
            received = serial->read_serial_data(1, serial_read_short_timeout);
            if (received.length())
                pagedata.append(received, received.length());
            timeout++;
        }
        if (timeout >= 1000)
        {
            emit LOG_E("Page data timeout!", true, true);
            return STATUS_ERROR;
        }

        if (pagedata.length() > 7)
            pagedata.remove(0, 8);

        QByteArray data;
        for (int i = 0; i < pagedata.length(); i+=16)
        {
            data.clear();
            for (int j = 0; j < 16; j++)
            {
                data.append(pagedata[i + j]);
            }
            emit LOG_I(parse_message_to_hex(data), true, true);
        }
        emit LOG_D(parse_message_to_hex(pagedata), true, true);
        mapdata.append(pagedata);

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
        delay(1);

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

    emit LOG_I("EEPROM read ready" + received, true, true);

    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*
 * 8bit checksum
 *
 * @return
 */
uint8_t EepromEcuSubaruDensoSH705xCan::cks_add8(QByteArray chksum_data, unsigned len)
{
    uint16_t sum = 0;
    uint8_t data[chksum_data.length()];
/*
    for (int i = 0; i < chksum_data.length(); i++)
    {
        data[i] = chksum_data.at(i);
    }
*/
    for (unsigned i = 0; i < len; i++) {
        sum += (uint8_t)chksum_data.at(i);//data[i];
        if (sum & 0x100) {
            sum += 1;
        }
        sum = (uint8_t) sum;
    }
    return sum;
}

/*
 * Generate denso can seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray EepromEcuSubaruDensoSH705xCan::generate_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex_1[]={
        0x78B1, 0x4625, 0x201C, 0x9EA5,
        0xAD6B, 0x35F4, 0xFD21, 0x5E71,
        0xB046, 0x7F4A, 0x4B75, 0x93F9,
        0x1895, 0x8961, 0x3ECC, 0x862B
    };

    const uint16_t keytogenerateindex_2[]={
        0x24B9, 0x9D91, 0xFF0C, 0xB8D5,
        0x15BB, 0xF998, 0x8723, 0x9E05,
        0x7092, 0xD683, 0xBA03, 0x59E1,
        0x6136, 0x9B9A, 0x9CFB, 0x9DDB
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    emit LOG_I("Using stock seed key algo", true, true);
    key = calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

// RSA: Function to compute base^expo mod m
unsigned long long EepromEcuSubaruDensoSH705xCan::decrypt_racerom_seed(unsigned long long base, unsigned long long exponent, unsigned long long modulus) {
    unsigned long long result = 1;
    base = base % modulus;
    while (exponent > 0) {
        if (exponent & 1)
            result = (result * 1LL * base) % modulus;
        base = (base * 1LL * base) % modulus;
        exponent = exponent / 2;
    }
    return result;
}

QByteArray EepromEcuSubaruDensoSH705xCan::generate_ecutek_racerom_can_seed_key(QByteArray requested_seed)
{
    emit LOG_I("Using EcuTek RaceRom RSA algo", true, true);
    // Function 000a86fc
    QByteArray key;
    uint32_t seed;

    seed = (requested_seed.at(0) << 24) & 0xFF000000;
    seed += (requested_seed.at(1) << 16) & 0x00FF0000;
    seed += (requested_seed.at(2) << 8) & 0x0000FF00;
    seed += requested_seed.at(3) & 0x000000FF;
    //seed = 0x00a80730;

    uint32_t d = 0x0A863281;
    uint32_t n = 0x0fda9293;
    seed = decrypt_racerom_seed(seed, d, n);
    QString msg = QString("Seed key: 0x%1").arg(seed, 8, 16, QLatin1Char('0')).toUtf8();
    emit LOG_I(msg, true, true);

    key.clear();
    key.append((uint8_t)(seed >> 24));
    key.append((uint8_t)(seed >> 16));
    key.append((uint8_t)(seed >> 8));
    key.append((uint8_t)seed);

    return key;
}

/*
 * Generate denso can ecutek seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray EepromEcuSubaruDensoSH705xCan::generate_ecutek_seed_key(QByteArray requested_seed)
{
    QByteArray seed_key;

    const uint16_t keytogenerateindex_1[]={
        0x78B1, 0x4625, 0x201C, 0x9EA5,
        0xAD6B, 0x35F4, 0xFD21, 0x5E71,
        0xB046, 0x7F4A, 0x4B75, 0x93F9,
        0x1895, 0x8961, 0x3ECC, 0x862B
    };

    const uint16_t keytogenerateindex_2[]={
        0x24B9, 0x9D91, 0xFF0C, 0xB8D5,
        0x15BB, 0xF998, 0x8723, 0x9E05,
        0x7092, 0xD683, 0xBA03, 0x59E1,
        0x6136, 0x9B9A, 0x9CFB, 0x9DDB
    };

    const uint8_t indextransformation[]={
        0x4, 0x2, 0x5, 0x1, 0x8, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    emit LOG_I("Using EcuTek seed key algo", true, true);
    seed_key = calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    if (flash_method.endsWith("_ecutek_racerom_alt"))
    {
        uint32_t et_rr_seed = 0;
        et_rr_seed = ((uint8_t)seed_key.at(0) << 24) & 0xFF000000;
        et_rr_seed += ((uint8_t)seed_key.at(1) << 16) & 0x00FF0000;
        et_rr_seed += ((uint8_t)seed_key.at(2) << 8) & 0x0000FF00;
        et_rr_seed += (uint8_t)seed_key.at(3) & 0x000000FF;

        et_rr_seed = ((seed_alter^et_rr_seed)^xor_byte_1)*xor_multi;
        et_rr_seed = (et_rr_seed^xor_byte_2)*xor_multi;

        seed_key.clear();
        seed_key.append((uint8_t)(et_rr_seed >> 24) & 0xff);
        seed_key.append((uint8_t)(et_rr_seed >> 16) & 0xff);
        seed_key.append((uint8_t)(et_rr_seed >> 8) & 0xff);
        seed_key.append((uint8_t)(et_rr_seed & 0xff));
        emit LOG_D("Altered seed key: " + parse_message_to_hex(seed_key), true, true);
    }

    return seed_key;
}

/************************************
 * COBB'd Denso CAN ECUs seed key
 ***********************************/
QByteArray EepromEcuSubaruDensoSH705xCan::generate_cobb_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    // 2017 VA model
    const uint16_t keytogenerateindex_1[]={
        0x9DDB, 0x9CFB, 0x9B9A, 0x6136,
        0x59E1, 0xBA03, 0xD683, 0x7092,
        0x9E05, 0x8723, 0xF998, 0x15BB,
        0xB8D5, 0xFF0C, 0x9D91, 0x24B9
    };

    // 2012 STi
    const uint16_t keytogenerateindex_2[]={
        0x77C1, 0x80BB, 0xD5C1, 0x7A94,
        0x11F0, 0x25FF, 0xC365, 0x10B4,
        0x48DA, 0x6720, 0x3255, 0xFA17,
        0x60BF, 0x780E, 0x9C1D, 0x5A28
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    emit LOG_I("Using COBB seed key algo", true, true);
    key = calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

/*
 * Calculate denso seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray EepromEcuSubaruDensoSH705xCan::calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
{
    QByteArray key;

    uint32_t seed, index;
    uint16_t wordtogenerateindex, wordtobeencrypted, encryptionkey;
    int ki, n;

    seed = (requested_seed.at(0) << 24) & 0xFF000000;
    seed += (requested_seed.at(1) << 16) & 0x00FF0000;
    seed += (requested_seed.at(2) << 8) & 0x0000FF00;
    seed += requested_seed.at(3) & 0x000000FF;

    QString msg = QString("Seed: 0x%1").arg(seed, 8, 16, QLatin1Char('0')).toUtf8();
    emit LOG_I(msg, true, true);

    //for (ki = 0; ki < 16; ki++) // Calculate seed from key
    for (ki = 15; ki >= 0; ki--) // Calculate key from seed
    {
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
    msg = QString("Seed key: 0x%1").arg(seed, 8, 16, QLatin1Char('0')).toUtf8();
    emit LOG_I(msg, true, true);

    key.clear();
    key.append((uint8_t)(seed >> 24));
    key.append((uint8_t)(seed >> 16));
    key.append((uint8_t)(seed >> 8));
    key.append((uint8_t)seed);

    return key;
}

/*
 * Encrypt upload data
 *
 * @return encrypted data
 */
QByteArray EepromEcuSubaruDensoSH705xCan::encrypt_payload(QByteArray buf, uint32_t len)
{
    QByteArray encrypted;

    const uint16_t keytogenerateindex[]={
        0xC85B, 0x32C0, 0xE282, 0x92A0
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

QByteArray EepromEcuSubaruDensoSH705xCan::decrypt_payload(QByteArray buf, uint32_t len)
{
    QByteArray decrypt;

    const uint16_t keytogenerateindex[]={
        0x92A0, 0xE282, 0x32C0, 0xC85B
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

QByteArray EepromEcuSubaruDensoSH705xCan::calculate_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
 * Request kernel id
 *
 * @return kernel id
 */
QByteArray EepromEcuSubaruDensoSH705xCan::request_kernel_id()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray kernelid;

    request_denso_kernel_id = true;

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)((SUB_KERNEL_START_COMM >> 8) & 0xFF));
    output.append((uint8_t)(SUB_KERNEL_START_COMM & 0xFF));
    output.append((uint8_t)0x00 & 0xFF);
    output.append((uint8_t)0x01 & 0xFF);
    output.append((uint8_t)(SUB_KERNEL_ID & 0xFF));
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Requesting kernel id sent: " + parse_message_to_hex(output), true, true);
    delay(100);
    received = serial->read_serial_data(100, serial_read_short_timeout);
    emit LOG_D("Requesting kernel id received: " + parse_message_to_hex(received), true, true);

    if (received.length() > 7)
        received.remove(0, 9);
    emit LOG_D("Initial request kernel id received and length:" + parse_message_to_hex(received) + " " + received.length(), true, true);
    kernelid = received;

    while (received.length())
    {
        received = serial->read_serial_data(10, serial_read_short_timeout);
        emit LOG_D("Requesting kernel id received:" + parse_message_to_hex(received), true, true);
        if (received.length() > 7)
            received.remove(0, 9);
        kernelid.append(received);
        delay(100);
    }

    request_denso_kernel_id = false;

    return kernelid;
}

/*
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray EepromEcuSubaruDensoSH705xCan::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
{
    uint8_t length = output.length();

    output.insert(0, (uint8_t)0x80);
    output.insert(1, target_id & 0xFF);
    output.insert(2, tester_id & 0xFF);
    output.insert(3, length);

    output.append(calculate_checksum(output, dec_0x100));

    //emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    //qDebug () << "Send:" << parse_message_to_hex(output);
    return output;
}

/*
 * Calculate SSM checksum to message
 *
 * @return 8-bit checksum
 */
uint8_t EepromEcuSubaruDensoSH705xCan::calculate_checksum(QByteArray output, bool dec_0x100)
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
QString EepromEcuSubaruDensoSH705xCan::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

void EepromEcuSubaruDensoSH705xCan::set_progressbar_value(int value)
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

void EepromEcuSubaruDensoSH705xCan::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}



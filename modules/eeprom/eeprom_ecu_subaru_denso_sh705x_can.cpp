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
    //send_log_window_message("MCU type: " + mcu_name + " and index: " + mcu_type_index, true, true);
    qDebug() << "MCU type: " + mcu_name + " (" + mcu_type_string + ") and index: " + QString::number(mcu_type_index);

    kernel = ecuCalDef->Kernel;
    flash_method = ecuCalDef->FlashMethod;

    emit external_logger("Starting");

    if (cmd_type == "read")
    {
        send_log_window_message("Read memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
        //qDebug() << "Read memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
    }
    else if (cmd_type == "test_write")
    {
        test_write = true;
        send_log_window_message("Test write memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
        //qDebug() << "Test write memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
    }
    else if (cmd_type == "write")
    {
        test_write = false;
        send_log_window_message("Write memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
        //qDebug() << "Write memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
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
            send_log_window_message("Connecting to Subaru 07+ 32-bit CAN bootloader, please wait...", true, true);
            result = connect_bootloader_subaru_denso_subarucan();

            if (result == STATUS_SUCCESS && !kernel_alive)
            {
                send_log_window_message("Initializing Subaru 07+ 32-bit CAN kernel upload, please wait...", true, true);
                result = upload_kernel_subaru_denso_subarucan(kernel, ecuCalDef->KernelStartAddr.toUInt(&ok, 16));
            }
            if (result == STATUS_SUCCESS)
            {
                if (cmd_type == "read")
                {
                    emit external_logger("Reading EEPROM, please wait...");
                    send_log_window_message("Reading EEPROM from Subaru 07+ 32-bit using CAN", true, true);
                    qDebug() << "Reading EEPROM start at:" << flashdevices[mcu_type_index].eblocks[0].start << "and size of" << flashdevices[mcu_type_index].eblocks[0].len;
                    result = read_mem_subaru_denso_subarucan(flashdevices[mcu_type_index].eblocks[0].start, flashdevices[mcu_type_index].eblocks[0].len);
                }
                else if (cmd_type == "test_write" || cmd_type == "write")
                {
                    emit external_logger("Writing EEPROM, please wait...");
                    send_log_window_message("Writing ROM to Subaru 07+ 32-bit using CAN", true, true);
                    //result = write_mem_subaru_denso_subarucan(ecuCalDef, test_write);
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
        qDebug() << "Operation canceled";
        this->close();
        break;
    default:
        QMessageBox::warning(this, tr("Connecting to ECU"), "Unknown operation selected!");
        qDebug() << "Unknown operation selected!";
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
int EepromEcuSubaruDensoSH705xCan::connect_bootloader_subaru_denso_subarucan()
{
    QByteArray output;
    QByteArray received;
    QByteArray seed;
    QByteArray seed_key;
    QByteArray msg;

    if (!serial->is_serial_port_open())
    {
        emit LOG_E("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Checking if kernel is already running...", true, true);
    emit LOG_I("Requesting kernel ID", true, true);

    received.clear();
    received = request_kernel_id();
    emit LOG_I("Kernel ID: " + received, true, true);
    if (received != "")
    {
        kernel_alive = true;
        return STATUS_SUCCESS;
    }
    emit LOG_I("No response from kernel, continue bootloader initialization...", true, true);

    emit LOG_I("Initializing bootloader", true, true);

    bool connected = false;
    //serial->set_j2534_stmin_tx();

    connected = false;
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x00);

    serial->write_serial_data_echo_check(output);
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length())
    {
        if ((uint8_t)received.at(0+4) == 0x41 || (uint8_t)received.at(1+4) == 0x00)
        {
            connected = true;
            QByteArray response = received;
            response.remove(0, 2+4);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_I("Response: " + parse_message_to_hex(received), true, true);
        }
        else
            emit LOG_E("Wrong response from ECU... (" + parse_message_to_hex(received) + ")", true, true);
    }

    if (!connected)
        return STATUS_ERROR;

    connected = false;
    output[4] = ((uint8_t)0x09);
    output[5] = ((uint8_t)0x02);

    serial->write_serial_data_echo_check(output);
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length())
    {
        if ((uint8_t)received.at(0+4) == 0x49 || (uint8_t)received.at(1+4) == 0x02)
        {
            connected = true;
            QByteArray response = received;
            response.remove(0, 2+4);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_I("Response: " + parse_message_to_hex(received), true, true);
            emit LOG_I("VIN: " + msg, true, true);
        }
        else
            emit LOG_E("Wrong response from ECU... (" + parse_message_to_hex(received) + ")", true, true);
    }

    if (!connected)
        return STATUS_ERROR;

    connected = false;
    output[4] = ((uint8_t)0x09);
    output[5] = ((uint8_t)0x06);

    serial->write_serial_data_echo_check(output);
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length())
    {
        if ((uint8_t)received.at(0+4) == 0x49 || (uint8_t)received.at(1+4) == 0x06)
        {
            connected = true;
            QByteArray response = received;
            response.remove(0, 2+4);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_I("Response: " + parse_message_to_hex(received), true, true);
            emit LOG_I("CVN: " + msg, true, true);
        }
        else
            emit LOG_E("Wrong response from ECU... (" + parse_message_to_hex(received) + ")", true, true);
    }

    if (!connected)
        return STATUS_ERROR;

    bool req_10_03_connected = false;
    bool req_10_43_connected = false;

    connected = false;
    output[4] = ((uint8_t)0x10);
    output[5] = ((uint8_t)0x03);

    serial->write_serial_data_echo_check(output);
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length())
    {
        if ((uint8_t)received.at(0+4) == 0x50 || (uint8_t)received.at(1+4) == 0x03)
        {
            req_10_03_connected = true;
            connected = true;
            QByteArray response = received;
            response.remove(0, 2+4);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_I("Response: " + parse_message_to_hex(received), true, true);
        }
        else
            emit LOG_E("Wrong response from ECU... (" + parse_message_to_hex(received) + ")", true, true);
    }

    connected = false;
    output[4] = ((uint8_t)0x10);
    output[5] = ((uint8_t)0x43);

    serial->write_serial_data_echo_check(output);
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length())
    {
        if ((uint8_t)received.at(0+4) == 0x50 || (uint8_t)received.at(1+4) == 0x43)
        {
            req_10_43_connected = true;
            connected = true;
            QByteArray response = received;
            response.remove(0, 2+4);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_I("Response: " + parse_message_to_hex(received), true, true);
        }
        else
            emit LOG_E("Wrong response from ECU... (" + parse_message_to_hex(received) + ")", true, true);
    }

    //if (!connected)
        //return STATUS_ERROR;

    connected = false;
    output[4] = ((uint8_t)0x27);
    output[5] = ((uint8_t)0x01);

    serial->write_serial_data_echo_check(output);
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length())
    {
        if ((uint8_t)received.at(0+4) == 0x67 || (uint8_t)received.at(1+4) == 0x01)
        {
            connected = true;
            QByteArray response = received;
            response.remove(0, 2+4);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_I("Response: " + parse_message_to_hex(received), true, true);
        }
        else
            emit LOG_E("Wrong response from ECU... (" + parse_message_to_hex(received) + ")", true, true);
    }

    if (!connected)
        return STATUS_ERROR;

    emit LOG_I("Seed request ok", true, true);

    seed.clear();
    seed.append(received.at(2+4));
    seed.append(received.at(3+4));
    seed.append(received.at(4+4));
    seed.append(received.at(5+4));

    seed.append((uint8_t)0xD3);
    seed.append((uint8_t)0x80);
    seed.append((uint8_t)0xE8);
    seed.append((uint8_t)0x94);

    if (flash_method.endsWith("_ecutek"))
        seed_key = subaru_denso_generate_ecutek_can_seed_key(seed);
    if (flash_method.endsWith("_cobb"))
        seed_key = subaru_denso_generate_cobb_can_seed_key(seed);
    else
        seed_key = subaru_denso_generate_can_seed_key(seed);

    emit LOG_I("Calculated seed key: " + parse_message_to_hex(seed_key), true, true);
    emit LOG_I("Sending seed key", true, true);

    connected = false;
    output[4] = ((uint8_t)0x27);
    output[5] = ((uint8_t)0x02);
    output.append(seed_key);

    serial->write_serial_data_echo_check(output);
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length())
    {
        if ((uint8_t)received.at(0+4) == 0x67 || (uint8_t)received.at(1+4) == 0x02)
        {
            connected = true;
            QByteArray response = received;
            response.remove(0, 2+4);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_I("Response: " + parse_message_to_hex(received), true, true);
        }
        else
            emit LOG_E("Wrong response from ECU... (" + parse_message_to_hex(received) + ")", true, true);
    }

    if (!connected)
        return STATUS_ERROR;

    emit LOG_I("Seed key ok", true, true);

    connected = false;
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
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length())
    {
        if ((uint8_t)received.at(0+4) == 0x50 || (uint8_t)received.at(1+4) == 0x02)
        {
            connected = true;
            QByteArray response = received;
            response.remove(0, 2+4);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_I("Response: " + parse_message_to_hex(received), true, true);
        }
        else
            emit LOG_E("Wrong response from ECU... (" + parse_message_to_hex(received) + ")", true, true);
    }

    if (!connected)
        return STATUS_ERROR;

    return STATUS_SUCCESS;
}

/*
 * Upload kernel to Subaru Denso CAN (iso15765) 32bit ECUs
 *
 * @return success
 */
int EepromEcuSubaruDensoSH705xCan::upload_kernel_subaru_denso_subarucan(QString kernel, uint32_t kernel_start_addr)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray payload;
    QByteArray received;
    QByteArray msg;
    QByteArray pl_encr;
    uint32_t file_len = 0;
    uint32_t pl_len = 0;
    uint32_t start_address = 0;
    uint32_t end_addr = 0;
    QByteArray cks_bypass;
    uint32_t chk_sum = 0;
    uint32_t blockaddr = 0;
    uint16_t blockno = 0;
    uint16_t maxblocks = 0;
    int byte_counter = 0;

    QString mcu_name;

    start_address = kernel_start_addr;//flashdevices[mcu_type_index].kblocks->start;
    emit LOG_D("Start address to upload kernel: 0x" + QString::number(start_address, 16), true, true);

    if (!serial->is_serial_port_open())
    {
        emit LOG_E("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->set_add_iso14230_header(false);

    // Check kernel file
    if (!file.open(QIODevice::ReadOnly ))
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
    pl_encr = subaru_denso_encrypt_32bit_payload(pl_encr, pl_encr.length());
    //pl_encr = subaru_denso_decrypt_32bit_payload(pl_encr, pl_encr.length());
    //qDebug() << "\nEncrypted kernel orig: " << parse_message_to_hex(pl_encr);
    //qDebug() << "Kernel checksum" << hex << chk_sum;

    set_progressbar_value(0);

    bool connected = false;

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
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(50);
    received = serial->read_serial_data(20, 10);
    if (received.length())
    {
        if ((uint8_t)received.at(0+4) == 0x34 || (uint8_t)received.at(1+4) == 0x20)
        {
            connected = true;
            QByteArray response = received;
            response.remove(0, 2+4);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_I("Response: " + parse_message_to_hex(received), true, true);
        }
    }

    if (!connected)
    {
        emit LOG_E("ERROR: No response / wrong response from ECU!", true, true);
        return STATUS_ERROR;
    }

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
        //qDebug() << "Data header:" << parse_message_to_hex(output);

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
        //qDebug() << "Kernel data:" << parse_message_to_hex(output);
        //delay(20);
        received = serial->read_serial_data(5, receive_timeout);

        float pleft = (float)blockno / (float)maxblocks * 100;
        set_progressbar_value(pleft);
    }
    emit LOG_D("Data bytes sent: 0x" + QString::number(data_bytes_sent, 16), true, true);

    connected = false;
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x37);

    serial->write_serial_data_echo_check(output);
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, 10);
    if (received.length())
    {
        if ((uint8_t)received.at(0+4) == 0x77)
        {
            connected = true;
            QByteArray response = received;
            response.remove(0, 2+4);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_I("Response: " + parse_message_to_hex(received), true, true);
        }
    }
    if (!connected)
        return STATUS_ERROR;

    delay(100);

    connected = false;
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
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    delay(50);
    received = serial->read_serial_data(20, 10);
    if (received.length())
    {
        if ((uint8_t)received.at(0+4) == 0x71)
        {
            connected = true;
            QByteArray response = received;
            response.remove(0, 2+4);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            emit LOG_I("Response: " + parse_message_to_hex(received), true, true);
        }
    }

    if (!connected)
        return STATUS_ERROR;

    set_progressbar_value(100);

    delay(100);

    emit LOG_I("Kernel started, initializing...", true, true);
    emit LOG_I("Requesting kernel ID", true, true);

    received.clear();
    received = request_kernel_id();
    //received.remove(0, 6);
    emit LOG_I("Kernel ID: " + received, true, true);
    if (received == "")
        return STATUS_ERROR;

    return STATUS_SUCCESS;
}














/*
 * Read memory from Subaru Denso CAN 32bit ECUs, nisprog kernel
 *
 * @return success
 */
int EepromEcuSubaruDensoSH705xCan::read_mem_subaru_denso_subarucan(uint32_t start_addr, uint32_t length)
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
        emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
        //delay(100);
        received = serial->read_serial_data(1, serial_read_timeout);
        emit LOG_I("Response: " + parse_message_to_hex(received), true, true);

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
 * CRC16 implementation adapted from Lammert Bies
 *
 * @return
 */
#define NPK_CRC16   0xBAAD  //koopman, 2048bits (256B)
static bool crc_tab16_init = 0;
static uint16_t crc_tab16[256];
void EepromEcuSubaruDensoSH705xCan::init_crc16_tab(void)
{

    uint32_t i, j;
    uint16_t crc, c;

    for (i=0; i<256; i++) {
        crc = 0;
        c   = (uint16_t) i;

        for (j=0; j<8; j++) {
            if ( (crc ^ c) & 0x0001 ) {
                crc = ( crc >> 1 ) ^ NPK_CRC16;
            } else {
                crc =   crc >> 1;
            }
            c = c >> 1;
        }
        crc_tab16[i] = crc;
    }

    crc_tab16_init = 1;

}

uint16_t EepromEcuSubaruDensoSH705xCan::crc16(const uint8_t *data, uint32_t siz)
{
    uint16_t crc;

    if (!crc_tab16_init) {
        init_crc16_tab();
    }

    crc = 0;

    while (siz > 0) {
        uint16_t tmp;
        uint8_t nextval;

        nextval = *data++;
        tmp =  crc ^ nextval;
        crc = (crc >> 8) ^ crc_tab16[ tmp & 0xff ];
        siz -= 1;
    }
    return crc;
}




















/*
 * Generate denso can seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray EepromEcuSubaruDensoSH705xCan::subaru_denso_generate_can_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    qDebug() << "Calculate Denso seed key";

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

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

/*
 * Generate denso can ecutek seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray EepromEcuSubaruDensoSH705xCan::subaru_denso_generate_ecutek_can_seed_key(QByteArray requested_seed)
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
        0x4, 0x2, 0x5, 0x1, 0x8, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

/************************************
 * COBB'd Denso CAN ECUs seed key
 ***********************************/
QByteArray EepromEcuSubaruDensoSH705xCan::subaru_denso_generate_cobb_can_seed_key(QByteArray requested_seed)
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

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

/*
 * Calculate denso seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray EepromEcuSubaruDensoSH705xCan::subaru_denso_calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
QByteArray EepromEcuSubaruDensoSH705xCan::subaru_denso_encrypt_32bit_payload(QByteArray buf, uint32_t len)
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

    encrypted = subaru_denso_calculate_32bit_payload(buf, len, keytogenerateindex, indextransformation);

    return encrypted;
}

QByteArray EepromEcuSubaruDensoSH705xCan::subaru_denso_decrypt_32bit_payload(QByteArray buf, uint32_t len)
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

    decrypt = subaru_denso_calculate_32bit_payload(buf, len, keytogenerateindex, indextransformation);

    return decrypt;
}

QByteArray EepromEcuSubaruDensoSH705xCan::subaru_denso_calculate_32bit_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
 * Request kernel init
 *
 * @return
 */
QByteArray EepromEcuSubaruDensoSH705xCan::request_kernel_init()
{
    QByteArray output;
    QByteArray received;

    request_denso_kernel_init = true;

    output.clear();

    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);

    output.append((uint8_t)SID_KERNEL_INIT);

    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    received = serial->write_serial_data_echo_check(output);
    delay(500);
    received = serial->read_serial_data(100, serial_read_short_timeout);

    request_denso_kernel_init = false;

    return received;
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
    emit LOG_D("Request kernel id sent: " + parse_message_to_hex(output), true, true);
    delay(100);
    received = serial->read_serial_data(100, serial_read_short_timeout);
    emit LOG_D("Request kernel id received: " + parse_message_to_hex(received), true, true);

    if (received.length() > 7)
        received.remove(0, 9);
    emit LOG_D("Initial request kernel id received and length:" + parse_message_to_hex(received) + " " + received.length(), true, true);
    kernelid = received;

    while (received.length())
    {
        received = serial->read_serial_data(10, serial_read_short_timeout);
        emit LOG_D("Request kernel id received:" + parse_message_to_hex(received), true, true);
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

    //send_log_window_message("Send: " + parse_message_to_hex(output), true, true);
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
 * Countdown prior power on
 *
 * @return
 */
int EepromEcuSubaruDensoSH705xCan::connect_bootloader_start_countdown(int timeout)
{
    for (int i = timeout; i > 0; i--)
    {
        if (kill_process)
            break;
        send_log_window_message("Starting in " + QString::number(i), true, true);
        //qDebug() << "Countdown:" << i;
        delay(1000);
    }
    if (!kill_process)
    {
        send_log_window_message("Initializing connection, please wait...", true, true);
        delay(1500);
        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
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

/*
 * Output text to log window
 *
 * @return
 */
int EepromEcuSubaruDensoSH705xCan::send_log_window_message(QString message, bool timestamp, bool linefeed)
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

        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);

        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
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



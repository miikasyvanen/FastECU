#include "flash_ecu_subaru_unisia_jecs_m32r_bootmode.h"

FlashEcuSubaruUnisiaJecsM32rBootMode::FlashEcuSubaruUnisiaJecsM32rBootMode(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
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

void FlashEcuSubaruUnisiaJecsM32rBootMode::run()
{
    this->show();

    int result = STATUS_ERROR;

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
    else if (cmd_type == "write")
    {
        emit LOG_I("Write memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
    }

    // Set serial port
    serial->reset_connection();
    serial->set_add_iso14230_header(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    serial->set_serial_port_parity(QSerialPort::EvenParity);
    tester_id = 0xF0;
    target_id = 0x10;
    serial->open_serial_port();
    serial->change_port_speed("39063");

    int ret = 0;

    if (cmd_type == "write")
    {
        emit LOG_I("Set programming voltage +12v to Line End Check 1 and MOD1 to Line End Check 2", true, true);
        serial->set_lec_lines(serial->get_requestToSendEnabled(), serial->get_dataTerminalEnabled());

        ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                   tr("Make sure VPP and MOD1 is connected and turn ignition ON and press OK to start initializing connection to ECU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);
    }
    else
    {
        ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                       tr("Turn ignition ON and press OK to start initializing connection to ECU"),
                                       QMessageBox::Ok | QMessageBox::Cancel,
                                       QMessageBox::Ok);
    }
    switch (ret)
    {
    case QMessageBox::Ok:

        if (cmd_type == "read")
        {
            emit external_logger("Reading ROM, please wait...");
            emit LOG_I("Reading ROM from Subaru Unisia Jecs UJ20/30WWW using K-Line", true, true);
            result = read_mem(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
        }
        else if (cmd_type == "write")
        {
            emit LOG_I("Uploading kernel to Subaru Unisia Jecs UJ20/30WWW using K-Line", true, true);
            result = upload_kernel(kernel);
            if (result == STATUS_SUCCESS)
            {
                emit external_logger("Writing ROM, please wait...");
                emit LOG_I("Writing ROM to Subaru Unisia Jecs UJ20/30WWW using K-Line", true, true);
                result = write_mem();
                emit LOG_I("Unset programming voltage +12v to Line End Check 1", true, true);
                serial->set_lec_lines(serial->get_requestToSendDisabled(), serial->get_dataTerminalDisabled());
            }
        }
        emit external_logger("Finished");
        serial->set_lec_lines(serial->get_requestToSendDisabled(), serial->get_dataTerminalDisabled());

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
        emit LOG_D("Operation canceled", true, true);
        this->close();
        break;
    default:
        QMessageBox::warning(this, tr("Connecting to ECU"), "Unknown operation selected!");
        emit LOG_D("Unknown operation selected!", true, true);
        this->close();
        break;
    }
}

FlashEcuSubaruUnisiaJecsM32rBootMode::~FlashEcuSubaruUnisiaJecsM32rBootMode()
{
    delete ui;
}

void FlashEcuSubaruUnisiaJecsM32rBootMode::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

/*
 * Read memory from Subaru Unisia Jecs UJ20/30/40/70 K-Line 32bit ECUs
 *
 * @return success
 */
int FlashEcuSubaruUnisiaJecsM32rBootMode::read_mem(uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;

    QByteArray output;
    QByteArray received;
    QString msg;
    QByteArray mapdata;
    QString ecuid;

    uint32_t pagesize = 0;
    uint32_t cplen = 0;

    serial->reset_connection();
    serial->set_serial_port_parity(QSerialPort::NoParity);
    serial->open_serial_port();

    if (!serial->is_serial_port_open())
    {
        emit LOG_E("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Checking if ECU in read mode", true, true);
    serial->change_port_speed("38400");
    received = send_subaru_sid_bf_ssm_init();
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    if (received != "" && received.length() > 12)
    {
        kernel_alive = true;
        received.remove(0, 8);
        received.remove(5, received.length() - 5);

        for (int i = 0; i < received.length(); i++)
        {
            msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
        }
        emit LOG_I("Connected, ECU ID: " + msg, true, true);
        ecuid = msg;
    }

    if(!kernel_alive)
    {
        // SSM init
        serial->change_port_speed("4800");
        received = send_subaru_sid_bf_ssm_init();
        if (received == "" && (uint8_t)received.at(4) != 0xff)
            return STATUS_ERROR;

        if (received.length() < 13)
            return STATUS_ERROR;

        received.remove(0, 8);
        received.remove(5, received.length() - 5);

        for (int i = 0; i < received.length(); i++)
        {
            msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
        }
        ecuid = msg;
        emit LOG_I("ECU ID = " + ecuid, true, true);
        //send_log_window_message("ECU ID = " + ecuid, true, true);

        received = send_subaru_sid_b8_change_baudrate_38400();
        //send_log_window_message("0xB8 response: " + parse_message_to_hex(received), true, true);
        //qDebug() << "0xB8 response:" << parse_message_to_hex(received);
        if (received == "" || (uint8_t)received.at(4) != 0xf8)
            return STATUS_ERROR;

        serial->change_port_speed("38400");

        // Checking connection after baudrate change with SSM Init
        received = send_subaru_sid_bf_ssm_init();
        emit LOG_D("Init response: " + parse_message_to_hex(received), true, true);
        if (received == "" || (uint8_t)received.at(4) != 0xff)
            return STATUS_ERROR;
    }

    ecuCalDef->RomId = ecuid;

    start_addr += 0x00100000;
    pagesize = 0x80;

    uint32_t skip_start = start_addr & (pagesize - 1); //if unaligned, we'll be receiving this many extra bytes
    uint32_t addr = start_addr - skip_start;
    uint32_t willget = (skip_start + length + pagesize - 1) & ~(pagesize - 1);

    timer.start();

    output.clear();
    output.append((uint8_t)0x80);
    output.append((uint8_t)0x10);
    output.append((uint8_t)0xF0);
    output.append((uint8_t)0x06);
    output.append((uint8_t)(SID_UNISIA_JECS_BLOCK_READ & 0xFF));
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    while (willget)
    {
        if (kill_process)
            return STATUS_ERROR;

        uint32_t numblocks = 1;
        unsigned curspeed = 0, tleft;
        uint32_t curblock = (addr / pagesize);
        float pleft = 0;
        unsigned long chrono;

        pleft = (float)(addr - start_addr) / (float)(length) * 100.0f;
        set_progressbar_value(pleft);

        output[6] = (uint8_t)(addr >> 16) & 0xFF;
        output[7] = (uint8_t)(addr >> 8) & 0xFF;
        output[8] = (uint8_t)addr & 0xFF;
        output[9] = (uint8_t)(pagesize - 1) & 0xFF;
        output.remove(10, 1);
        output.append(calculate_checksum(output, false));

        //chk_sum = calculate_checksum(output, false);
        //output.append((uint8_t) chk_sum);
        received = serial->write_serial_data_echo_check(output);
        received = serial->read_serial_data(pagesize + 6, serial_read_extra_long_timeout);

        //qDebug() << "Received map data:" << parse_message_to_hex(received);
        if (received.startsWith("\x80\xf0"))
        {
            received.remove(0, 5);
            received.remove(received.length() - 1, 1);
            mapdata.append(received);
        }
        else
        {
            emit LOG_E("ERROR IN DATA RECEIVE! Addr: 0x" +  QString::number(addr, 16) + " " + parse_message_to_hex(received), true, true);
        }

        cplen = (numblocks * pagesize);

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
        msg = QString("ROM read addr: 0x%1 length: 0x%2, %3 B/s %4 s remain").arg(start_address).arg(block_len).arg(curspeed, 6, 10, QLatin1Char(' ')).arg(tleft, 6, 10, QLatin1Char(' ')).toUtf8();
        emit LOG_I(msg, true, true);
        delay(1);

        addr += (numblocks * pagesize);
        willget -= pagesize;
    }

    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

int FlashEcuSubaruUnisiaJecsM32rBootMode::upload_kernel(QString kernel)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray received;
    float pleft = 0;

    if (!serial->is_serial_port_open())
    {
        LOG_E("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    // Check kernel file
    if (!file.open(QIODevice::ReadOnly ))
    {
        emit LOG_E("Unable to open kernel file for reading", true, true);
        return STATUS_ERROR;
    }
    //LOG_I("Kernel file " + file.fileName() + " opened.", true, true);

    QByteArray kerneldata = file.readAll();
    file.close();

    while (kerneldata.length() % 0x80)
        kerneldata.append((uint8_t)0x00);

    int filesize = kerneldata.length();

    emit LOG_I("Uploading kernel, please wait...", true, true);
    for (int i = 0; i < filesize; i+=0x80)
    {
        output.clear();
        for (int j = 0; j < 0x80; j++)
            output.append(kerneldata.at(i + j));
        serial->write_serial_data_echo_check(output);
        pleft = (float)i / (float)filesize * 100.0f;
        set_progressbar_value(pleft);
    }
    set_progressbar_value(100);

    delay(500);
    received = serial->read_serial_data(100, 200);
    received.clear();



    return STATUS_SUCCESS;
}

/*
 *
 * Error codes:
 * 0x48 - Missing VPP voltage
 * 0x5a -
 * 0x5c - Checksum error
 * 0x72 - Address error in 0x61 command
 * 0x8a - FENTRY bit not set
 *
 */
int FlashEcuSubaruUnisiaJecsM32rBootMode::write_mem()
{
    QByteArray output;
    QByteArray received;
    QString msg;

    serial->reset_connection();
    serial->set_serial_port_parity(QSerialPort::NoParity);
    serial->open_serial_port();
    serial->change_port_speed("19200");

    serial->set_lec_lines(serial->get_requestToSendEnabled(), serial->get_dataTerminalDisabled());
    QMessageBox::information(this, tr("Flash file"), "Remove MOD1 voltage and press ok to continue.");

    emit LOG_I("Initialising serial port, please wait...", true, true);
    if(!serial->is_serial_port_open())
    {
        emit LOG_E("Serial port error!", true, true);
        serial->reset_connection();
        return STATUS_ERROR;
    }
    else
    {
        emit LOG_I("Requesting flash erase, please wait...", true, true);

        output.clear();
        output.append((uint8_t)0x80);
        output.append((uint8_t)0x10);
        output.append((uint8_t)0xF0);
        output.append((uint8_t)0x02);
        output.append((uint8_t)0xAF);
        output.append((uint8_t)0x31);
        output.append(calculate_checksum(output, false));
        serial->write_serial_data_echo_check(output);
        emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
        delay(500);

        emit LOG_I("", true, false);
        received.clear();
        for (int i = 0; i < 20; i++)
        {
            received.append(serial->read_serial_data(10, 10));
            emit LOG_I(".", false, false);
            if (received.length() > 6)
            {
                if ((uint8_t)received.at(0) == 0x80 && (uint8_t)received.at(1) == 0xf0 && (uint8_t)received.at(2) == 0x10 && (uint8_t)received.at(3) == 0x02 && (uint8_t)received.at(4) == 0xEF && (uint8_t)received.at(5) == 0x42)
                {
                    emit LOG_I("", false, true);
                    emit LOG_I("Flash erase in progress, please wait...", true, true);
                    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
                    break;
                }
                else
                {
                    emit LOG_I("", false, true);
                    emit LOG_E("Flash erase cmd failed!", true, true);
                    emit LOG_E("Response: " + parse_message_to_hex(received), true, true);
                    return STATUS_ERROR;
                }
            }
            delay(500);
        }
        if (received == "")
        {
            emit LOG_I("", false, true);
            emit LOG_E("Flash erase cmd failed!", true, true);
            emit LOG_E("Response: " + parse_message_to_hex(received), true, true);
            serial->reset_connection();

            return STATUS_ERROR;
        }
    }
    emit LOG_I("", true, false);
    received.clear();
    for (int i = 0; i < 20; i++)
    {
        received.append(serial->read_serial_data(10, 10));
        emit LOG_I(".", false, false);
        if (received.length() > 6)
        {
            if ((uint8_t)received.at(0) == 0x80 && (uint8_t)received.at(1) == 0xf0 && (uint8_t)received.at(2) == 0x10 && (uint8_t)received.at(3) == 0x02 && (uint8_t)received.at(4) == 0xEF && (uint8_t)received.at(5) == 0x52)
            {
                emit LOG_I("", false, true);
                emit LOG_I("Flash erased!", true, true);
                emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
                break;
            }
            else
            {
                emit LOG_I("", false, true);
                emit LOG_E("Flash erase failed!", true, true);
                emit LOG_E("Response: " + parse_message_to_hex(received), true, true);
                return STATUS_ERROR;
            }
        }
        delay(1000);
    }

    delay(1000);

    QByteArray filedata;
    filedata = ecuCalDef->FullRomData;

    QString flashdata_filename = ecuCalDef->FileName;
    int flashdatasize = filedata.length();
    uint32_t dataaddr = 0;
    int blocksize = 0x80;
    int blocks = flashdatasize / blocksize;

    uint32_t start = 0;
    uint32_t byteindex = 0;
    unsigned long chrono;
    unsigned curspeed, tleft;
    QElapsedTimer timer;

    emit LOG_I("Uploading file " + flashdata_filename + " to flash, please wait...", true, true);

    for (int i = 0; i < blocks; i++)
    {
        output.clear();
        output.append((uint8_t)0x80);
        output.append((uint8_t)0x10);
        output.append((uint8_t)0xF0);
        output.append((uint8_t)0x85);
        output.append((uint8_t)0xAF);
        if (i < (blocks - 1))
            output.append((uint8_t)0x61);
        else
            output.append((uint8_t)0x69);
        output.append((uint8_t)(dataaddr >> 16) & 0xFF);
        output.append((uint8_t)(dataaddr >> 8) & 0xFF);
        output.append((uint8_t)dataaddr & 0xFF);
        for (int j = 0; j < blocksize; j++)
        {
            output.append((filedata.at(i * blocksize + j)));// ^ encrypt));
        }
        output.append(calculate_checksum(output, false));

        serial->write_serial_data_echo_check(output);
        emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
        delay(5);
        received.clear();
        if (output.at(5) == 0x61)
        {
            for (int i = 0; i < 500; i++)
            {
                received.append(serial->read_serial_data(10, 1));
                if (received.length() > 6)
                {
                    if ((uint8_t)received.at(0) == 0x80 && (uint8_t)received.at(1) == 0xf0 && (uint8_t)received.at(2) == 0x10 && (uint8_t)received.at(3) == 0x02 && (uint8_t)received.at(4) == 0xEF && (uint8_t)received.at(5) == 0x52)
                    {
                        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
                        break;
                    }
                    else
                    {
                        emit LOG_E("Block flash failed!", true, true);
                        emit LOG_E("Response: " + parse_message_to_hex(received), true, true);
                        return STATUS_ERROR;
                    }
                }
                delay(50);
            }
            if (received == "")
            {
                emit LOG_E("Flash failed!", true, true);
                emit LOG_E("Response: " + parse_message_to_hex(received), true, true);
                return STATUS_ERROR;
            }
        }

        QString start_address = QString("%1").arg(start,8,16,QLatin1Char('0')).toUpper();
        QString block_len = QString("%1").arg(blocksize,8,16,QLatin1Char('0')).toUpper();
        msg = QString("Kernel write addr: 0x%1 length: 0x%2, %3 B/s %4 s remain").arg(start_address).arg(block_len).arg(curspeed, 6, 10, QLatin1Char(' ')).arg(tleft, 6, 10, QLatin1Char(' ')).toUtf8();
        emit LOG_I(msg, true, true);

        start += blocksize;
        byteindex += blocksize;

        chrono = timer.elapsed();
        timer.start();

        if (!chrono) {
            chrono += 1;
        }
        curspeed = blocksize * (1000.0f / chrono);  //avg B/s
        if (!curspeed) {
            curspeed += 1;
        }

        tleft = ((float)flashdatasize - byteindex) / curspeed;  //s
        if (tleft > 9999) {
            tleft = 9999;
        }
        tleft++;

        dataaddr += blocksize;
        float pleft = (float)(i * blocksize) / (float)flashdatasize * 100;
        set_progressbar_value(pleft);
    }
    set_progressbar_value(100);
    emit LOG_I("File " + flashdata_filename + " written to flash.", true, true);
    emit LOG_I("Please remove VPP voltage, power cycle ECU and request SSM Init to confirm.", true, true);

    return STATUS_SUCCESS;
}





/*
 * ECU init
 *
 * @return ECU ID and capabilities
 */
QByteArray FlashEcuSubaruUnisiaJecsM32rBootMode::send_subaru_sid_bf_ssm_init()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    emit LOG_I("SSM init", true, true);
    output.append((uint8_t)0xBF);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    delay(250);
    received = serial->read_serial_data(100, receive_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    while (received == "" && loop_cnt < comm_try_count)
    {
        if (kill_process)
            break;

        serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
        emit LOG_I("SSM init", true, true);
        delay(comm_try_timeout);
        received = serial->read_serial_data(100, receive_timeout);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        loop_cnt++;
    }

    return received;
}

QByteArray FlashEcuSubaruUnisiaJecsM32rBootMode::send_subaru_sid_b8_change_baudrate_4800()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    emit LOG_I("Request baudrate change to 4800", true, true);
    output.clear();
    output.append((uint8_t)0xB8);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x15);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    delay(200);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

QByteArray FlashEcuSubaruUnisiaJecsM32rBootMode::send_subaru_sid_b8_change_baudrate_38400()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    emit LOG_I("Request baudrate change to 38400", true, true);
    output.clear();
    output.append((uint8_t)0xB8);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x75);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    delay(200);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray FlashEcuSubaruUnisiaJecsM32rBootMode::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
{
    uint8_t length = output.length();

    output.insert(0, (uint8_t)0x80);
    output.insert(1, target_id & 0xFF);
    output.insert(2, tester_id & 0xFF);
    output.insert(3, length);

    output.append(calculate_checksum(output, dec_0x100));

    return output;
}

/*
 * Calculate SSM checksum to message
 *
 * @return 8-bit checksum
 */
uint8_t FlashEcuSubaruUnisiaJecsM32rBootMode::calculate_checksum(QByteArray output, bool dec_0x100)
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
QString FlashEcuSubaruUnisiaJecsM32rBootMode::parse_message_to_hex(QByteArray received)
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
int FlashEcuSubaruUnisiaJecsM32rBootMode::send_log_window_message(QString message, bool timestamp, bool linefeed)
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

void FlashEcuSubaruUnisiaJecsM32rBootMode::set_progressbar_value(int value)
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

void FlashEcuSubaruUnisiaJecsM32rBootMode::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

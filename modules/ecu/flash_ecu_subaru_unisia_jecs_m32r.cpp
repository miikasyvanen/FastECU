#include "flash_ecu_subaru_unisia_jecs_m32r.h"

FlashEcuSubaruUnisiaJecsM32r::FlashEcuSubaruUnisiaJecsM32r(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
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

void FlashEcuSubaruUnisiaJecsM32r::run()
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
    qDebug() << "MCU type:" << mcu_name << mcu_type_string << "and index:" << mcu_type_index;

    flash_method = ecuCalDef->FlashMethod;

    emit external_logger("Starting");

    if (cmd_type == "read")
    {
        emit LOG_I("Read memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
        //qDebug() << "Read memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
    }
    else if (cmd_type == "write")
    {
        emit LOG_I("Write memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
        //qDebug() << "Write memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
    }

    // Set serial port
    serial->set_is_iso14230_connection(false);
    serial->set_add_iso14230_header(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    tester_id = 0xF0;
    target_id = 0x10;
    serial->change_port_speed("4800");
    serial->open_serial_port();

    int ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                   tr("Turn ignition ON and press OK to start initializing connection to ECU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);

    switch (ret)
    {
        case QMessageBox::Ok:

            if (cmd_type == "read")
            {
                emit external_logger("Reading ROM, please wait...");
                emit LOG_I("Reading ROM from Subaru Unisia Jecs UJ20/30/40/70WWW using K-Line", true, true);
                result = read_mem_subaru_unisia_jecs(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
            }
            else if (cmd_type == "write")
            {
                emit external_logger("Writing ROM, please wait...");
                emit LOG_I("Writing ROM to Subaru Unisia Jecs UJ20/30/40/70WWW using K-Line", true, true);
                result = write_mem_subaru_unisia_jecs();
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

FlashEcuSubaruUnisiaJecsM32r::~FlashEcuSubaruUnisiaJecsM32r()
{
    delete ui;
}

void FlashEcuSubaruUnisiaJecsM32r::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

/*
 * Read memory from Subaru Unisia Jecs UJ20/30/40/70 K-Line 32bit ECUs
 *
 * @return success
 */
int FlashEcuSubaruUnisiaJecsM32r::read_mem_subaru_unisia_jecs(uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;

    QByteArray output;
    QByteArray received;
    QString msg;
    QByteArray mapdata;
    QString ecuid;

    uint32_t pagesize = 0;
    uint32_t cplen = 0;

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
    if (start_addr == 0 && length == 0)
    {
        start_addr = 0;
        length = 0x040000;
    }

    uint32_t skip_start = start_addr & (pagesize - 1); //if unaligned, we'll be receiving this many extra bytes
    uint32_t addr = start_addr - skip_start;
    uint32_t willget = (skip_start + length + pagesize - 1) & ~(pagesize - 1);
    //uint32_t len_done = 0;  //total data written to file

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
        //uint32_t curblock = (addr / pagesize);
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

        received = serial->write_serial_data_echo_check(output);
        received = serial->read_serial_data(pagesize + 6, serial_read_extra_long_timeout);

        //emit LOG_I("Received map data:" << parse_message_to_hex(received), true, true);
        if (received.startsWith("\x80\xf0"))
        {
            received.remove(0, 5);
            received.remove(received.length() - 1, 1);
            mapdata.append(received);
        }
        else
        {
            emit LOG_I("ERROR IN DATA RECEIVE!" + QString::number(addr, 16) + " " + parse_message_to_hex(received), true, true);
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
        msg = QString("ROM read addr:  0x%1  length:  0x%2,  %3  B/s  %4 s remaining").arg(start_address).arg(block_len).arg(curspeed, 6, 10, QLatin1Char(' ')).arg(tleft, 6, 10, QLatin1Char(' ')).toUtf8();
        emit LOG_I(msg, true, true);
        delay(1);

        //len_done += cplen;
        addr += (numblocks * pagesize);
        willget -= pagesize;
    }

    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*
 * Write memory to Subaru Unisia Jecs UJ20/30/40/70 K-Line 32bit ECUs
 *
 * @return success
 */
int FlashEcuSubaruUnisiaJecsM32r::write_mem_subaru_unisia_jecs()
{
    QByteArray flashdata;
    QByteArray output;
    QByteArray received;
    QString msg;

    flashdata = ecuCalDef->FullRomData;
    set_progressbar_value(0);

    if (!serial->is_serial_port_open())
    {
        emit LOG_E("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->set_add_iso14230_header(false);

    // SSM init
    received = send_subaru_sid_bf_ssm_init();
    emit LOG_I("SSM init: " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0xFF)
        return STATUS_ERROR;

    received.remove(0, 8);
    received.remove(5, received.length() - 5);

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
    }
    QString ecuid = msg;
    emit LOG_I("ECU ID = " + ecuid, true, true);

    emit LOG_I("Sending request to change to flash mode", true, true);
    received = send_subaru_unisia_jecs_sid_af_enter_flash_mode(received);
    emit LOG_I("Response: " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0xef)
        return STATUS_ERROR;

    emit LOG_I("Changing baudrate to 19200", true, true);
    serial->change_port_speed("19200");

    emit LOG_I("Set programming voltage +12v to Line End Check 1", true, true);
    serial->set_lec_lines(0, 1);

    emit LOG_I("Sending request to erase flash", true, true);
    received = send_subaru_unisia_jecs_sid_af_erase_memory_block();

    emit LOG_I("", true, false);
    received.clear();
    for (int i = 0; i < 20; i++)
    {
        received.append(serial->read_serial_data(8, receive_timeout));
        emit LOG_I(".", false, false);
        if (received.length() > 6)
        {
            if ((uint8_t)received.at(0) == 0x80 && (uint8_t)received.at(1) == 0xf0 && (uint8_t)received.at(2) == 0x10 && (uint8_t)received.at(3) == 0x02 && (uint8_t)received.at(4) == 0xef && (uint8_t)received.at(5) == 0x42)
            {
                emit LOG_I("", false, true);
                emit LOG_I("Flash erase in progress, please wait...", true, true);
                break;
            }
            else
            {
                emit LOG_E("", false, true);
                emit LOG_E("Flash erase cmd failed!", true, true);
                emit LOG_E("Response: " + parse_message_to_hex(received), true, true);
                return STATUS_ERROR;
            }
        }
        delay(500);
    }
    if (received == "")
    {
        emit LOG_E("", false, true);
        emit LOG_E("Flash erase cmd failed, no answer from ECU!", true, true);
        emit LOG_E("Response: " + parse_message_to_hex(received), true, true);

        return STATUS_ERROR;
    }
    emit LOG_I("", true, false);
    received.clear();
    for (int i = 0; i < 40; i++)
    {
        received.append(serial->read_serial_data(10, receive_timeout));
        emit LOG_I(".", false, false);
        qDebug() << ".";
        if (received.length() > 6)
        {
            if ((uint8_t)received.at(0) == 0x80 && (uint8_t)received.at(1) == 0xf0 && (uint8_t)received.at(2) == 0x10 && (uint8_t)received.at(3) == 0x02 && (uint8_t)received.at(4) == 0xef && (uint8_t)received.at(5) == 0x52)
            {
                emit LOG_I("", false, true);
                emit LOG_I("Flash erased!", true, true);
                break;
            }
            else
            {
                emit LOG_E("", false, true);
                emit LOG_E("Flash erase failed!", true, true);
                emit LOG_E("Received: " + parse_message_to_hex(received), true, true);
                return STATUS_ERROR;
            }
        }
        delay(500);
    }
    received = serial->read_serial_data(100, receive_timeout);
    //delay(500);

    QString flashdata_filename = ecuCalDef->FileName;
    int flashdatasize = flashdata.length();
    uint32_t dataaddr = 0;
    int blocksize = 128;
    int blocks = flashdatasize / blocksize;
    int encrypt = 0x82;

    uint32_t start = 0;
    uint32_t remain = flashdata.length();
    uint32_t len = flashdata.length();
    uint32_t byteindex = 0;
    unsigned long chrono;
    unsigned curspeed, tleft;
    QElapsedTimer timer;

    emit LOG_I("Uploading file " + flashdata_filename + " to flash, please wait...", true, true);

    for (int i = 0; i < blocks; i++)
    {
        output.clear();
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
            output.append(flashdata.at(i * blocksize + j) ^ encrypt);
        }
        serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));

        //emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
        //delay(5);
        received.clear();
        if (output.at(1) == 0x61)
        {
            for (int i = 0; i < 500; i++)
            {
                received.append(serial->read_serial_data(8, receive_timeout));
                if (received.length() > 6)
                {
                    if ((uint8_t)received.at(0) == 0x80 && (uint8_t)received.at(1) == 0xf0 && (uint8_t)received.at(2) == 0x10 && (uint8_t)received.at(3) == 0x02 && (uint8_t)received.at(4) == 0xef && (uint8_t)received.at(5) == 0x52)
                    {
                        //emit LOG_I("Response: " + parse_message_to_hex(received), true, true);
                        break;
                    }
                    else
                    {
                        emit LOG_I("Block flash failed!", true, true);
                        emit LOG_I("Response: " + parse_message_to_hex(received), true, true);
                        return STATUS_ERROR;
                    }
                }
                //delay(50);
            }
        }
        else
        {
            LOG_I("File written to flash, please wait...", true, true);
            delay(2000);
        }
        if ((uint8_t)output.at(1) != 0x69 && received == "")
        {
            emit LOG_E("Flash failed!", true, true);
            return STATUS_ERROR;
        }

        QString start_address = QString("%1").arg(start,8,16,QLatin1Char('0'));
        msg = QString("Writing chunk @ 0x%1 (%2\% - %3 B/s, ~ %4 s remaining)").arg(start_address).arg((unsigned) 100 * (len - remain) / len,1,10,QLatin1Char('0')).arg((uint32_t)curspeed,1,10,QLatin1Char('0')).arg(tleft,1,10,QLatin1Char('0')).toUtf8();
        emit LOG_I(msg, true, true);

        remain -= blocksize;
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

    return STATUS_SUCCESS;
}






/*
 * ECU init
 *
 * @return ECU ID and capabilities
 */
QByteArray FlashEcuSubaruUnisiaJecsM32r::send_subaru_sid_bf_ssm_init()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0xBF);

    while (received == "" && loop_cnt < 1)
    {
        if (kill_process)
            break;

        emit LOG_I("SSM init", true, true);
        serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
        emit LOG_I("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);

        delay(200);
        received = serial->read_serial_data(100, receive_timeout);
        emit LOG_I("Response: " + parse_message_to_hex(received), true, true);
        loop_cnt++;
    }

    return received;
}

QByteArray FlashEcuSubaruUnisiaJecsM32r::send_subaru_sid_b8_change_baudrate_4800()
{
    QByteArray output;
    QByteArray received;

    //qDebug() << "Start B8";
    output.clear();
    output.append((uint8_t)0xB8);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x15);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_I("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(comm_try_timeout);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_I("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

QByteArray FlashEcuSubaruUnisiaJecsM32r::send_subaru_sid_b8_change_baudrate_38400()
{
    QByteArray output;
    QByteArray received;

    //qDebug() << "Start B8";
    output.clear();
    output.append((uint8_t)0xB8);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x75);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_I("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(comm_try_timeout);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_I("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

QByteArray FlashEcuSubaruUnisiaJecsM32r::send_subaru_unisia_jecs_sid_af_enter_flash_mode(QByteArray ecu_id)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    uint32_t rom_size = flashdevices[mcu_type_index].romsize;

    output.append((uint8_t)0xAF);
    output.append((uint8_t)0x11);
    output.append((uint8_t)ecu_id.at(0)); // ECU ID [0]
    output.append((uint8_t)ecu_id.at(1)); // ECU ID [1]
    output.append((uint8_t)ecu_id.at(2)); // ECU ID [2]
    output.append((uint8_t)ecu_id.at(3)); // ECU ID [3]
    output.append((uint8_t)ecu_id.at(4)); // ECU ID [4]
    output.append((uint8_t)(rom_size >> 16) & 0xFF); // ROM size >> 24
    output.append((uint8_t)(rom_size >> 8) & 0xFF); // ROM size >> 16
    output.append((uint8_t)rom_size & 0xFF); // ROM size
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_I("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(comm_try_timeout);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_I("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

QByteArray FlashEcuSubaruUnisiaJecsM32r::send_subaru_unisia_jecs_sid_af_erase_memory_block()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    output.append((uint8_t)0xAF);
    output.append((uint8_t)0x31);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));

    return received;
}

/*
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray FlashEcuSubaruUnisiaJecsM32r::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
{
    uint8_t length = output.length();

    output.insert(0, (uint8_t)0x80);
    output.insert(1, target_id & 0xFF);
    output.insert(2, tester_id & 0xFF);
    output.insert(3, length);

    output.append(calculate_checksum(output, dec_0x100));

    //LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    return output;
}

/*
 * Calculate SSM checksum to message
 *
 * @return 8-bit checksum
 */
uint8_t FlashEcuSubaruUnisiaJecsM32r::calculate_checksum(QByteArray output, bool dec_0x100)
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
QString FlashEcuSubaruUnisiaJecsM32r::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("0x%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

void FlashEcuSubaruUnisiaJecsM32r::set_progressbar_value(int value)
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

void FlashEcuSubaruUnisiaJecsM32r::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

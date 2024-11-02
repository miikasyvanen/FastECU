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
    qDebug() << "MCU type:" << mcu_name << mcu_type_string << "and index:" << mcu_type_index;

    flash_method = ecuCalDef->FlashMethod;

    emit external_logger("Starting");

    if (cmd_type == "read")
    {
        send_log_window_message("Read memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
        //qDebug() << "Read memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
    }
    else if (cmd_type == "write")
    {
        send_log_window_message("Write memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
        //qDebug() << "Write memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
    }

    // Set serial port
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    tester_id = 0xF0;
    target_id = 0x10;
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
            send_log_window_message("Reading ROM from Subaru Unisia Jecs UJ20/30/40/70WWW using K-Line", true, true);
            result = read_mem(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
        }
        else if (cmd_type == "write")
        {
            emit external_logger("Writing ROM, please wait...");
            send_log_window_message("Writing ROM to Subaru Unisia Jecs UJ20/30/40/70WWW using K-Line", true, true);
            result = write_mem();
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

FlashEcuSubaruUnisiaJecsM32rBootMode::~FlashEcuSubaruUnisiaJecsM32rBootMode()
{

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

    uint32_t pagesize = 0;
    uint32_t end_addr = 0;
    uint32_t datalen = 0;
    uint32_t cplen = 0;

    uint8_t chk_sum = 0;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->set_add_iso14230_header(false);

    // Start countdown
    //if (connect_bootloader_start_countdown(bootloader_start_countdown))
    //    return STATUS_ERROR;

    serial->change_port_speed("4800");
    // SSM init
    received = send_subaru_sid_bf_ssm_init();
    if (received == "" || (uint8_t)received.at(4) != 0xff)
        return STATUS_ERROR;

    received.remove(0, 8);
    received.remove(5, received.length() - 5);

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
    }
    QString ecuid = msg;
    send_log_window_message("ECU ID = " + ecuid, true, true);

    received = send_subaru_sid_b8_change_baudrate_38400();
    send_log_window_message("0xB8 response: " + parse_message_to_hex(received), true, true);
    qDebug() << "0xB8 response:" << parse_message_to_hex(received);
    if (received == "" || (uint8_t)received.at(4) != 0xf8)
        return STATUS_ERROR;

    serial->change_port_speed("38400");

    // Checking connection after baudrate change with SSM Init
    received = send_subaru_sid_bf_ssm_init();
    if (received == "" || (uint8_t)received.at(4) != 0xff)
        return STATUS_ERROR;

    datalen = 6;
    pagesize = 0x80;
    if (start_addr == 0 && length == 0)
    {
        start_addr = 0;
        length = 0x040000;
    }
    end_addr = start_addr + length;

    uint32_t skip_start = start_addr & (pagesize - 1); //if unaligned, we'll be receiving this many extra bytes
    uint32_t addr = start_addr - skip_start;
    uint32_t willget = (skip_start + length + pagesize - 1) & ~(pagesize - 1);
    uint32_t len_done = 0;  //total data written to file

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
            qDebug() << "ERROR IN DATA RECEIVE!" << hex << addr << parse_message_to_hex(received);
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
        send_log_window_message(msg, true, true);
        //qDebug() << msg;
        delay(1);

        len_done += cplen;
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

    serial->set_serial_port_baudrate("39063");
    serial->set_serial_port_parity(QSerialPort::EvenParity);

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }
    // Check kernel file
    if (!file.open(QIODevice::ReadOnly ))
    {
        send_log_window_message("Unable to open kernel file for reading", true, true);
        return STATUS_ERROR;
    }

    QByteArray kerneldata = file.readAll();
    file.close();

    int filesize = kerneldata.length();

    send_log_window_message("Initialising serial port, please wait...", true, true);
    qDebug() << "Initialising serial port, please wait...";
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("Serial port error!", true, true);
        qDebug() << "Serial port error!";
        return STATUS_ERROR;
    }
    else
    {
        send_log_window_message("Uploading UJ20 kernel, please wait...", true, true);
        qDebug() << "Uploading kernel, please wait...";
        for (int i = 0; i < filesize; i++)
        {
            output.clear();
            output.append(kerneldata.at(i));
            serial->write_serial_data_echo_check(output);
            pleft = (float)i / (float)filesize * 100.0f;
            set_progressbar_value(pleft);
        }
        delay(500);
        received = serial->read_serial_data(100, 200);
        received.clear();
    }

    set_progressbar_value(100);


    return STATUS_SUCCESS;
}

int FlashEcuSubaruUnisiaJecsM32rBootMode::write_mem()
{
    QByteArray output;
    QByteArray received;
    float pleft = 0;
    uint32_t dataaddr = 0;

    QMessageBox::information(this, tr("Flash file"), "Remove MOD1 voltage and press ok to continue.");

    serial->set_serial_port_baudrate("19200");
    serial->set_serial_port_parity(QSerialPort::NoParity);
    serial->reset_connection();

    send_log_window_message("Initialising serial port, please wait...", true, true);
    qDebug() << "Initialising serial port, please wait...";
    if(!serial->is_serial_port_open())
    {
        send_log_window_message("Serial port error!", true, true);
        qDebug() << "Serial port error!";
        serial->reset_connection();
        return STATUS_ERROR;
    }
    else
    {
        emit send_log_window_message("Requesting flash erase, please wait...", true, true);
        qDebug() << "Requesting flash erase, please wait...";

        output.clear();
        output.append((uint8_t)0x80);
        output.append((uint8_t)0x10);
        output.append((uint8_t)0xF0);
        output.append((uint8_t)0x02);
        output.append((uint8_t)0xAF);
        output.append((uint8_t)0x31);
        output.append(calculate_checksum(output, false));
        serial->write_serial_data_echo_check(output);
        delay(500);

        send_log_window_message("", true, false);
        received.clear();
        for (int i = 0; i < 20; i++)
        {
            received.append(serial->read_serial_data(10, 1));
            send_log_window_message(".", false, false);
            qDebug() << ".";
            if (received.length() > 6)
            {
                if ((uint8_t)received.at(0) == 0x80 && (uint8_t)received.at(1) == 0xf0 && (uint8_t)received.at(2) == 0x10 && (uint8_t)received.at(3) == 0x02 && (uint8_t)received.at(4) == 0xEF && (uint8_t)received.at(5) == 0x42)
                {
                    send_log_window_message("", false, true);
                    send_log_window_message("Flash erase in progress, please wait...", true, true);
                    qDebug() << "Flash erase in progress, please wait...";
                    break;
                }
                else
                {
                    send_log_window_message("", false, true);
                    send_log_window_message("Flash erase cmd failed!", true, true);
                    qDebug() << "Flash erase cmd failed!";
                    send_log_window_message("Received: " + parse_message_to_hex(received), true, true);
                    qDebug() << "Received: " + parse_message_to_hex(received);
                    return STATUS_ERROR;
                }
            }
            delay(500);
        }
        if (received == "")
        {
            send_log_window_message("", false, true);
            send_log_window_message("Flash erase cmd failed!", true, true);
            qDebug() << "Flash erase cmd failed!";
            send_log_window_message("Received: " + parse_message_to_hex(received), true, true);
            qDebug() << "Received: " + parse_message_to_hex(received);
            serial->reset_connection();

            return STATUS_ERROR;
        }
    }
    send_log_window_message("", true, false);
    received.clear();
    for (int i = 0; i < 20; i++)
    {
        received.append(serial->read_serial_data(10, 1));
        send_log_window_message(".", false, false);
        qDebug() << ".";
        if (received.length() > 6)
        {
            if ((uint8_t)received.at(0) == 0x80 && (uint8_t)received.at(1) == 0xf0 && (uint8_t)received.at(2) == 0x10 && (uint8_t)received.at(3) == 0x02 && (uint8_t)received.at(4) == 0xEF && (uint8_t)received.at(5) == 0x52)
            {
                send_log_window_message("", false, true);
                send_log_window_message("Flash erased!", true, true);
                qDebug() << "Flash erased!";
                break;
            }
            else
            {
                send_log_window_message("", false, true);
                send_log_window_message("Flash erase failed!", true, true);
                qDebug() << "Flash erase failed!";
                send_log_window_message("Received: " + parse_message_to_hex(received), true, true);
                qDebug() << "Received: " + parse_message_to_hex(received);
                return STATUS_ERROR;
            }
        }
        delay(1000);
    }

    delay(1000);

    QByteArray filedata;
    filedata = ecuCalDef->FullRomData;

    int flashfilesize = filedata.length();
    dataaddr = 0;
    int blocksize = 128;
    int blocks = flashfilesize / blocksize;
    //int encrypt = 0x82;

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
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent: " + parse_message_to_hex(output);
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
                        send_log_window_message("Received: " + parse_message_to_hex(received), true, true);
                        qDebug() << "Received: " + parse_message_to_hex(received);
                        break;
                    }
                    else
                    {
                        send_log_window_message("Received: " + parse_message_to_hex(received), true, true);
                        qDebug() << "Received: " + parse_message_to_hex(received);
                        send_log_window_message("Block flash failed!", true, true);
                        qDebug() << "Block flash failed!";
                        return STATUS_ERROR;
                    }
                }
                delay(50);
            }
            if (received == "")
            {
                send_log_window_message("Flash failed!", true, true);
                qDebug() << "Flash failed!";
                return STATUS_ERROR;
            }
        }
        else
        {
            send_log_window_message("File written to flash, please wait...", true, true);
            delay(5);
            send_log_window_message("Done! Please remove VPP voltage, power cycle ECU and request SSM Init to confirm.", true, true);
        }
        dataaddr += blocksize;
        pleft = (float)(i * blocksize) / (float)flashfilesize * 100.0f;
        set_progressbar_value(pleft);
    }

    set_progressbar_value(100);

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

    //qDebug() << "Start BF";
    send_log_window_message("SSM init", true, true);
    //qDebug() << "SSM init";
    output.append((uint8_t)0xBF);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    delay(250);
    received = serial->read_serial_data(100, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        if (kill_process)
            break;

        //qDebug() << "Next BF loop";
        send_log_window_message("SSM init", true, true);
        //qDebug() << "SSM init";
        //qDebug() << "SSM init" << parse_message_to_hex(received);
        serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(100, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0xBF loop_cnt:" << loop_cnt;

    //qDebug() << "BF received:" << parse_message_to_hex(received);

    return received;
}

QByteArray FlashEcuSubaruUnisiaJecsM32rBootMode::send_subaru_sid_b8_change_baudrate_4800()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start B8";
    output.clear();
    output.append((uint8_t)0xB8);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x15);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    delay(200);
    received = serial->read_serial_data(8, receive_timeout);

    return received;
}

QByteArray FlashEcuSubaruUnisiaJecsM32rBootMode::send_subaru_sid_b8_change_baudrate_38400()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start B8";
    output.clear();
    output.append((uint8_t)0xB8);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x75);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    delay(200);
    received = serial->read_serial_data(8, receive_timeout);

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

    //send_log_window_message("Send: " + parse_message_to_hex(output), true, true);
    //qDebug () << "Send:" << parse_message_to_hex(output);
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

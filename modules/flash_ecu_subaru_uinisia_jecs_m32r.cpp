#include "flash_ecu_subaru_uinisia_jecs_m32r.h"

FlashEcuSubaruUnisiaJecs::FlashEcuSubaruUnisiaJecs(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
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

void FlashEcuSubaruUnisiaJecs::run()
{
    this->show();

    int result = STATUS_ERROR;

    result = init_flash_subaru_unisia_jecs();

    if (result == STATUS_SUCCESS)
    {
        QMessageBox::information(this, tr("ECU Operation"), "ECU operation was succesful, press OK to exit");
        this->close();
    }
    else
    {
        QMessageBox::warning(this, tr("ECU Operation"), "ECU operation failed, press OK to exit and try again");
    }
}

FlashEcuSubaruUnisiaJecs::~FlashEcuSubaruUnisiaJecs()
{

}

void FlashEcuSubaruUnisiaJecs::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

int FlashEcuSubaruUnisiaJecs::init_flash_subaru_unisia_jecs()
{
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

    int result = STATUS_ERROR;

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
    //serial->set_is_iso14230_connection(true);
    //serial->set_add_iso14230_header(true);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    //serial->set_serial_port_baudrate("10400");
    tester_id = 0xF0;
    target_id = 0x10;
    // Open serial port
    serial->open_serial_port();
    //serial->change_port_speed("10400");

    QMessageBox::information(this, tr("Connecting to ECU"), "Turn ignition ON and press OK to start initializing connection");
    //QMessageBox::information(this, tr("Connecting to ECU"), "Press OK to start countdown!");

    if (cmd_type == "read")
    {
        emit external_logger("Reading ROM, please wait...");
        send_log_window_message("Reading ROM from Subaru Unisia Jecs UJ20/30/40/70WWW using K-Line", true, true);
        result = read_mem_subaru_unisia_jecs(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
    }
    else if (cmd_type == "test_write" || cmd_type == "write")
    {
        emit external_logger("Writing ROM, please wait...");
        send_log_window_message("Writing ROM to Subaru Unisia Jecs UJ20/30/40/70WWW using K-Line", true, true);
        result = write_mem_subaru_unisia_jecs(test_write);
    }
    emit external_logger("Finished");
    return result;
}

/*
 * Read memory from Subaru Unisia Jecs UJ20/30/40/70 K-Line 32bit ECUs
 *
 * @return success
 */
int FlashEcuSubaruUnisiaJecs::read_mem_subaru_unisia_jecs(uint32_t start_addr, uint32_t length)
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
    if (received == "" || (uint8_t)received.at(4) != 0xFF)
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
    if (received == "" || (uint8_t)received.at(4) != 0xF8)
        return STATUS_ERROR;

    serial->change_port_speed("38400");

    output.clear();
    output.append((uint8_t)0xBF);
    received = serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    delay(50);
    received = serial->read_serial_data(20, 1000);
    send_log_window_message("SSM init response: " + parse_message_to_hex(received), true, true);
    qDebug() << "SSM init response:" << parse_message_to_hex(received);
    if (received == "")
        return STATUS_ERROR;

    datalen = 6;
    pagesize = 0x80;
    if (start_addr == 0 && length == 0)
    {
        start_addr = 0;
        length = 0x030000;
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

/*
 * Write memory to Subaru Unisia Jecs UJ20/30/40/70 K-Line 32bit ECUs
 *
 * @return success
 */
int FlashEcuSubaruUnisiaJecs::write_mem_subaru_unisia_jecs(bool test_write)
{
    QByteArray output;
    QByteArray received;
    QString msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->set_add_iso14230_header(false);

    // Start countdown
    //if (connect_bootloader_start_countdown(bootloader_start_countdown))
    //    return STATUS_ERROR;

    // SSM init
    received = send_subaru_sid_bf_ssm_init();
    //send_log_window_message("SID BF = " + parse_message_to_hex(received), true, true);
    if (received == "")
        return STATUS_ERROR;
    //qDebug() << "SID_BF received:" << parse_message_to_hex(received);
    received.remove(0, 8);
    received.remove(5, received.length() - 5);
    //qDebug() << "Received length:" << received.length();
    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
    }
    QString ecuid = msg;

    QMessageBox::information(this, tr("Forester, Impreza, Legacy 2000-2002 K-Line (UJ WA12212920/128KB)"), "Forester, Impreza, Legacy 2000-2002 K-Line (UJ WA12212920/128KB) writing not yet confirmed!");
    received = send_subaru_unisia_jecs_sid_af_enter_flash_mode(received);

    qDebug() << "SID AF enter flash mode response:" << parse_message_to_hex(received);

    return STATUS_ERROR;
}





















/*
 * ECU init
 *
 * @return ECU ID and capabilities
 */
QByteArray FlashEcuSubaruUnisiaJecs::send_subaru_sid_bf_ssm_init()
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

QByteArray FlashEcuSubaruUnisiaJecs::send_subaru_sid_b8_change_baudrate_4800()
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
    //serial->write_serial_data_echo_check(output); // No SSM header?
    delay(200);
    received = serial->read_serial_data(8, receive_timeout);

    return received;
}

QByteArray FlashEcuSubaruUnisiaJecs::send_subaru_sid_b8_change_baudrate_38400()
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
    //serial->write_serial_data_echo_check(output); // No SSM header?
    delay(200);
    received = serial->read_serial_data(8, receive_timeout);

    return received;
}

QByteArray FlashEcuSubaruUnisiaJecs::send_subaru_unisia_jecs_sid_af_enter_flash_mode(QByteArray ecu_id)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    uint32_t rom_size = flashdevices[mcu_type_index].fblocks->len - 1;
    qDebug() << hex << rom_size;

    //qDebug() << "Start AF";
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
    //serial->write_serial_data_echo_check(add_ssm_header(output, false));
    serial->write_serial_data_echo_check(output); // No SSM header?
    delay(500);
    received = serial->read_serial_data(8, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next AF loop";
        //serial->write_serial_data_echo_check(add_ssm_header(output, false));
        serial->write_serial_data_echo_check(output); // No SSM header?
        delay(comm_try_timeout);
        received = serial->read_serial_data(8, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0x31 loop_cnt:" << loop_cnt;

    //qDebug() << "31 received:" << parse_message_to_hex(received);

    return received;
}

QByteArray FlashEcuSubaruUnisiaJecs::send_subaru_unisia_jecs_sid_af_erase_memory_block(uint32_t address)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0xAF);
    output.append((uint8_t)0x31);

    return received;
}

QByteArray FlashEcuSubaruUnisiaJecs::send_subaru_unisia_jecs_sid_af_write_memory_block(uint32_t address, QByteArray payload)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0xAF);
    output.append((uint8_t)0x61);

    return received;
}

QByteArray FlashEcuSubaruUnisiaJecs::send_subaru_unisia_jecs_sid_af_write_last_memory_block(uint32_t address, QByteArray payload)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0xAF);
    output.append((uint8_t)0x69);

    return received;
}

/*
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray FlashEcuSubaruUnisiaJecs::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
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
uint8_t FlashEcuSubaruUnisiaJecs::calculate_checksum(QByteArray output, bool dec_0x100)
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
int FlashEcuSubaruUnisiaJecs::connect_bootloader_start_countdown(int timeout)
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
QString FlashEcuSubaruUnisiaJecs::parse_message_to_hex(QByteArray received)
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
int FlashEcuSubaruUnisiaJecs::send_log_window_message(QString message, bool timestamp, bool linefeed)
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

void FlashEcuSubaruUnisiaJecs::set_progressbar_value(int value)
{
    bool valueChanged = true;
    if (ui->progressbar)
    {
        ui->progressbar->setValue(value);
        valueChanged = ui->progressbar->value() != value;
    }
    if (valueChanged)
        emit external_logger(value);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void FlashEcuSubaruUnisiaJecs::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

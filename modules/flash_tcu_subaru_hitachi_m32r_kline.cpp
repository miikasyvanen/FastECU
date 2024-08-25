#include "flash_tcu_subaru_hitachi_m32r_kline.h"

FlashTcuSubaruHitachiM32rKline::FlashTcuSubaruHitachiM32rKline(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
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
        this->setWindowTitle("Read ROM from TCU");

    this->serial = serial;
}

void FlashTcuSubaruHitachiM32rKline::run()
{
    this->show();

    int result = STATUS_ERROR;
    set_progressbar_value(0);

    //result = init_flash_hitachi_can();

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

    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_iso14230_connection(true);
    serial->open_serial_port();
    serial->change_port_speed("4800");
    serial->set_add_iso14230_header(false);
    tester_id = 0xF0;
    target_id = 0x18;

    int ret = QMessageBox::warning(this, tr("Connecting to TCU"),
                                   tr("Turn ignition ON and press OK to start initializing connection to TCU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);

    switch (ret)
    {
        case QMessageBox::Ok:
            send_log_window_message("Connecting to Subaru TCU Hitachi CAN bootloader, please wait...", true, true);
            result = connect_bootloader_subaru_tcu_hitachi_kline();

            if (result == STATUS_SUCCESS)
            {
                if (cmd_type == "read")
                {
                    emit external_logger("Reading ROM, please wait...");
                    send_log_window_message("Reading ROM from TCU using K-Line", true, true);
                    result = read_a0_rom_subaru_tcu_hitachi_kline(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                    //result = read_a0_ram_subaru_tcu_hitachi_kline(0x80000, 0x100000);
                    //result = read_b8_subaru_tcu_hitachi_kline(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                    //result = read_b0_subaru_tcu_hitachi_kline(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                }
                else if (cmd_type == "test_write" || cmd_type == "write")
                {
                    emit external_logger("Writing ROM, please wait...");
                    send_log_window_message("Not yet implemented: Writing ROM to TCU Subaru Hitachi using CAN", true, true);
                    //result = write_mem_subaru_denso_can_02_32bit(test_write);
                }
            }
            emit external_logger("Finished");

            if (result == STATUS_SUCCESS)
            {
                QMessageBox::information(this, tr("TCU Operation"), "TCU operation was succesful, press OK to exit");
                this->close();
            }
            else
            {
                QMessageBox::warning(this, tr("TCU Operation"), "TCU operation failed, press OK to exit and try again");
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

FlashTcuSubaruHitachiM32rKline::~FlashTcuSubaruHitachiM32rKline()
{

}

void FlashTcuSubaruHitachiM32rKline::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

/*
 * Connect to Subaru TCU Hitachi CAN bootloader
 *
 * @return success
 */
int FlashTcuSubaruHitachiM32rKline::connect_bootloader_subaru_tcu_hitachi_kline()
{
    QByteArray received;
    QByteArray seed;
    QByteArray seed_key;

    QString msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    delay(100);

    send_log_window_message("Initializing k-line communications", true, true);

    received = send_subaru_tcu_sid_bf_ssm_init();
    //if (received == "")
    //    return STATUS_ERROR;
    //qDebug() << "SID_BF received:" << parse_message_to_hex(received);
    received.remove(0, 8);
    received.remove(5, received.length() - 5);
    //qDebug() << "Received length:" << received.length();
    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
    }
    QString calid = msg;
    //QString ecuid = QString("%1%2%3%4%5").arg(received.at(0),2,16,QLatin1Char('0')).toUpper().arg(received.at(1),2,16,QLatin1Char('0')).arg(received.at(2),2,16,QLatin1Char('0')).arg(received.at(3),2,16,QLatin1Char('0')).arg(received.at(4),2,16,QLatin1Char('0'));

    send_log_window_message("Init Success: CAL ID = " + calid, true, true);
    qDebug() << "CAL ID = " << calid;

    // Start communication
    received = send_subaru_tcu_sid_81_start_communication();
    send_log_window_message("SID_81 = " + parse_message_to_hex(received), true, true);
    //if (received == "" || (uint8_t)received.at(4) != 0xC1)
    //    return STATUS_ERROR;

    send_log_window_message("Start communication ok", true, true);
    qDebug() << "Start communication ok";

    // Request timing parameters
    received = send_subaru_tcu_sid_83_request_timings();
    send_log_window_message("SID_83 = " + parse_message_to_hex(received), true, true);
    //if (received == "" || (uint8_t)received.at(4) != 0xC3)
    //    return STATUS_ERROR;

    send_log_window_message("Request timing parameters ok", true, true);
    qDebug() << "Request timing parameters ok";

    // Request seed
    received = send_subaru_tcu_sid_27_request_seed();
    send_log_window_message("SID_27_01 = " + parse_message_to_hex(received), true, true);
    //if (received == "" || (uint8_t)received.at(4) != 0x67)
    //    return STATUS_ERROR;

    send_log_window_message("Seed request ok", true, true);
    qDebug() << "Seed request ok";

    seed.append(received.at(6));
    seed.append(received.at(7));
    seed.append(received.at(8));
    seed.append(received.at(9));

    seed_key = subaru_tcu_generate_kline_seed_key(seed);
    msg = parse_message_to_hex(seed_key);
    //qDebug() << "Calculated seed key: " + msg + ", sending to ECU";
    send_log_window_message("Calculated seed key: " + msg + ", sending to ECU", true, true);
    send_log_window_message("Sending seed key", true, true);
    qDebug() << "Sending seed key";

    received = send_subaru_tcu_sid_27_send_seed_key(seed_key);
    send_log_window_message("SID_27_02 = " + parse_message_to_hex(received), true, true);
    //if (received == "" || (uint8_t)received.at(4) != 0x67)
    //    return STATUS_ERROR;

    send_log_window_message("Seed key ok", true, true);
    qDebug() << "Seed key ok";

    // Start diagnostic session
    //received = send_subaru_tcu_sid_10_start_diagnostic();
    //send_log_window_message("SID_10 = " + parse_message_to_hex(received), true, true);
    //if (received == "" || (uint8_t)received.at(4) != 0x50)
    //    return STATUS_ERROR;

    //send_log_window_message("Start diagnostic session ok", true, true);
    //qDebug() << "Start diagnostic session ok";

    return STATUS_SUCCESS;
}


/*
 * For reading a portion of ROM using a0 command
 *
 *
 */int FlashTcuSubaruHitachiM32rKline::read_a0_rom_subaru_tcu_hitachi_kline(uint32_t start_addr, uint32_t length)
{

    QByteArray received;
    QByteArray msg;
    QByteArray mapdata;

    uint32_t curr_addr, block_count, num_blocks;
    uint32_t tail_size, block_len, loop_ctr, block_size;

    block_size = 96;
    //length = 0x400;   // hack for testing

    send_log_window_message("Commencing TCU ROM read", true, true);

    curr_addr = start_addr; //- 0x00100000;          // manual adjustment until fix processor details
    num_blocks = length / block_size;
    tail_size = length % block_size;
    if (tail_size != 0) num_blocks++;
    else tail_size = block_size;

    for (block_count = 0; block_count < num_blocks; block_count++)
    {
        if (block_count == (num_blocks - 1))
            block_len = tail_size;
        else
            block_len = block_size;

        msg = QString("ROM read addr: %1 length: %2").arg(curr_addr).arg(block_len).toUtf8();
        send_log_window_message(msg, true, true);

        for (loop_ctr = 0; loop_ctr < 5; loop_ctr++)
        {
            received = send_subaru_tcu_sid_a0_block_read(curr_addr, block_len - 1);
            if (received.length() > 5) break;
        }

        if (received.length() > 5)
        {
            received.remove(0, 5);
            mapdata.append(received, (received.length() - 1));
        }

        curr_addr += block_len;
    }

    ecuCalDef->FullRomData = mapdata;

    send_log_window_message("Saving TCU ROM to default.bin...", true, true);
    QString filename = "default.bin";
    QFile file(filename);
    QFileInfo fileInfo(file.fileName());
    QString file_name_str = fileInfo.fileName();

    if (!file.open(QIODevice::WriteOnly ))
    {
        //qDebug() << "Unable to open file for writing";
        QMessageBox::warning(this, tr("Ecu calibration file"), "Unable to open file for writing");
        return NULL;
    }

    //ecuCalDef = apply_subaru_cal_changes_to_rom_data(ecuCalDef);
    //checksum_correction(ecuCalDef);

    file.write(ecuCalDef->FullRomData);
    file.close();

    ecuCalDef->FullFileName = filename;
    ecuCalDef->FileName = file_name_str;
    send_log_window_message("TCU ROM saved successfully", true, true);
    return STATUS_SUCCESS;
}

/*
 * For reading a portion of ROM using b8 command
 *
 *
 */int FlashTcuSubaruHitachiM32rKline::read_b8_subaru_tcu_hitachi_kline(uint32_t start_addr, uint32_t length)
{

    QByteArray received;
    QByteArray msg;
    QByteArray mapdata;

    uint32_t curr_addr, block_count, num_blocks;
    uint32_t tail_size, block_len, loop_ctr, block_size;

    block_size = 1;
    length = 0x10;  // hack for testing

    send_log_window_message("Commencing TCU ROM read", true, true);

    curr_addr = start_addr - 0x00100000;          // manual adjustment until fix processor details
    num_blocks = length / block_size;
    tail_size = length % block_size;
    if (tail_size != 0) num_blocks++;
    else tail_size = block_size;

    for (block_count = 0; block_count < num_blocks; block_count++)
    {
        if (block_count == (num_blocks - 1))
            block_len = tail_size;
        else
            block_len = block_size;

        msg = QString("ROM read addr: %1 length: %2").arg(curr_addr).arg(block_len).toUtf8();
        send_log_window_message(msg, true, true);

        for (loop_ctr = 0; loop_ctr < 5; loop_ctr++)
        {
            received = send_subaru_tcu_sid_b8_byte_read(curr_addr);
            if (received.length() > 5) break;
        }

        if (received.length() > 5)
        {
            received.remove(0, 5);
            mapdata.append(received, (received.length() - 1));
        }

        curr_addr += block_len;
    }

    ecuCalDef->FullRomData = mapdata;

    send_log_window_message("Saving TCU ROM to default.bin...", true, true);
    QString filename = "default.bin";
    QFile file(filename);
    QFileInfo fileInfo(file.fileName());
    QString file_name_str = fileInfo.fileName();

    if (!file.open(QIODevice::WriteOnly ))
    {
        //qDebug() << "Unable to open file for writing";
        QMessageBox::warning(this, tr("Ecu calibration file"), "Unable to open file for writing");
        return NULL;
    }

    //ecuCalDef = apply_subaru_cal_changes_to_rom_data(ecuCalDef);
    //checksum_correction(ecuCalDef);

    file.write(ecuCalDef->FullRomData);
    file.close();

    ecuCalDef->FullFileName = filename;
    ecuCalDef->FileName = file_name_str;
    send_log_window_message("TCU ROM saved successfully", true, true);
    return STATUS_SUCCESS;
}

/*
 * For reading a portion of ROM using b0 command
 *
 *
 */int FlashTcuSubaruHitachiM32rKline::read_b0_subaru_tcu_hitachi_kline(uint32_t start_addr, uint32_t length)
{

    QByteArray received;
    QByteArray msg;
    QByteArray mapdata;

    uint32_t curr_addr, block_count, num_blocks;
    uint32_t tail_size, block_len, loop_ctr, block_size;

    block_size = 96;
    length = 4;  // for testing

    send_log_window_message("Commencing TCU ROM read", true, true);

    curr_addr = 0x804ffc;       // for testing
    num_blocks = length / block_size;
    tail_size = length % block_size;
    if (tail_size != 0) num_blocks++;
    else tail_size = block_size;

    for (block_count = 0; block_count < num_blocks; block_count++)
    {
        if (block_count == (num_blocks - 1))
            block_len = tail_size;
        else
            block_len = block_size;

        msg = QString("ROM read addr: %1 length: %2").arg(curr_addr).arg(block_len).toUtf8();
        send_log_window_message(msg, true, true);

        for (loop_ctr = 0; loop_ctr < 5; loop_ctr++)
        {
            received = send_subaru_tcu_sid_b0_block_write(curr_addr, block_len);
            if (received.length() > 5) break;
        }

        if (received.length() > 5)
        {
            received.remove(0, 5);
            mapdata.append(received, (received.length() - 1));
        }

        curr_addr += block_len;
    }

    ecuCalDef->FullRomData = mapdata;

    send_log_window_message("Saving TCU ROM to default.bin...", true, true);
    QString filename = "default.bin";
    QFile file(filename);
    QFileInfo fileInfo(file.fileName());
    QString file_name_str = fileInfo.fileName();

    if (!file.open(QIODevice::WriteOnly ))
    {
        //qDebug() << "Unable to open file for writing";
        QMessageBox::warning(this, tr("Ecu calibration file"), "Unable to open file for writing");
        return NULL;
    }

    //ecuCalDef = apply_subaru_cal_changes_to_rom_data(ecuCalDef);
    //checksum_correction(ecuCalDef);

    file.write(ecuCalDef->FullRomData);
    file.close();

    ecuCalDef->FullFileName = filename;
    ecuCalDef->FileName = file_name_str;
    send_log_window_message("TCU ROM saved successfully", true, true);
    return STATUS_SUCCESS;
}

/*
 * For reading a portion of RAM
 *
 *
 */
int FlashTcuSubaruHitachiM32rKline::read_a0_ram_subaru_tcu_hitachi_kline(uint32_t start_addr, uint32_t length)
{

    QByteArray received;
    QByteArray msg;
    QByteArray mapdata;

    uint32_t curr_addr, block_count, num_blocks;
    uint32_t tail_size, block_len, loop_ctr, block_size;

    block_size = 96;
    length = 0x400; // hack for testing

    send_log_window_message("Commencing TCU ROM read", true, true);

    curr_addr = start_addr;
    num_blocks = length / block_size;
    tail_size = length % block_size;
    if (tail_size != 0) num_blocks++;
    else tail_size = block_size;

    for (block_count = 0; block_count < num_blocks; block_count++)
    {
        if (block_count == (num_blocks - 1))
            block_len = tail_size;
        else
            block_len = block_size;

        msg = QString("ROM read addr: %1 length: %2").arg(curr_addr).arg(block_len).toUtf8();
        send_log_window_message(msg, true, true);

        for (loop_ctr = 0; loop_ctr < 5; loop_ctr++)
        {
            received = send_subaru_tcu_sid_a0_block_read(curr_addr, block_len - 1);
            if (received.length() > 5) break;
        }

        if (received.length() > 5)
        {
            received.remove(0, 5);
            mapdata.append(received, (received.length() - 1));
        }

        curr_addr += block_len;
    }

    ecuCalDef->FullRomData = mapdata;

    send_log_window_message("Saving TCU ROM to default.bin...", true, true);
    QString filename = "default.bin";
    QFile file(filename);
    QFileInfo fileInfo(file.fileName());
    QString file_name_str = fileInfo.fileName();

    if (!file.open(QIODevice::WriteOnly ))
    {
        //qDebug() << "Unable to open file for writing";
        QMessageBox::warning(this, tr("Ecu calibration file"), "Unable to open file for writing");
        return NULL;
    }

    //ecuCalDef = apply_subaru_cal_changes_to_rom_data(ecuCalDef);
    //checksum_correction(ecuCalDef);

    file.write(ecuCalDef->FullRomData);
    file.close();

    ecuCalDef->FullFileName = filename;
    ecuCalDef->FileName = file_name_str;
    send_log_window_message("TCU ROM saved successfully", true, true);
    return STATUS_SUCCESS;
}

/*
 * ECU init
 *
 * @return ECU ID and capabilities
 */
QByteArray FlashTcuSubaruHitachiM32rKline::send_subaru_tcu_sid_bf_ssm_init()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;


    output.append((uint8_t)0xBF);

    while (received == "" && loop_cnt < 5)
    {
        if (kill_process)
            break;

        send_log_window_message("SSM init", true, true);
        serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
        send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
        delay(200);
        received = serial->read_serial_data(100, receive_timeout);
        //received = QByteArray::fromHex("80f01839ffa61022ada02160000100800400000000a1462c000800000000000000dc06000829c0047e011e003e00000000000080a6e000fefe00002000da");
        send_log_window_message("Recd: " + parse_message_to_hex(received), true, true);
        loop_cnt++;
    }

    return received;
}

/*
 * Start diagnostic connection
 *
 * @return received response
 */
QByteArray FlashTcuSubaruHitachiM32rKline::send_subaru_tcu_sid_81_start_communication()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0x81);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);

    return received;
}

/*
 * Start diagnostic connection
 *
 * @return received response
 */
QByteArray FlashTcuSubaruHitachiM32rKline::send_subaru_tcu_sid_83_request_timings()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0x83);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(12, receive_timeout);

    return received;
}

/*
 * Request seed
 *
 * @return seed (4 bytes)
 */
QByteArray FlashTcuSubaruHitachiM32rKline::send_subaru_tcu_sid_27_request_seed()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0x27);
    output.append((uint8_t)0x01);
    received = serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(11, receive_timeout);

    return received;
}

/*
 * Send seed key
 *
 * @return received response
 */
QByteArray FlashTcuSubaruHitachiM32rKline::send_subaru_tcu_sid_27_send_seed_key(QByteArray seed_key)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0x27);
    output.append((uint8_t)0x02);
    output.append(seed_key);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(8, receive_timeout);

    return received;
}

/*
 * Request start diagnostic
 *
 * @return received response
 */
QByteArray FlashTcuSubaruHitachiM32rKline::send_subaru_tcu_sid_10_start_diagnostic()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0x10);
    output.append((uint8_t)0x85);
    output.append((uint8_t)0x02);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(7, receive_timeout);
    /*
    while (received == "" && loop_cnt < comm_try_count)
    {
        serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(7, receive_timeout);
        loop_cnt++;
    }
*/
    return received;
}

/*
 * Generate denso kline seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashTcuSubaruHitachiM32rKline::subaru_tcu_generate_kline_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex[]={
        0x0FE9, 0xCA58, 0x5E90, 0xDFF1,
        0x690B, 0xF591, 0x1794, 0x5C7B,
        0xA7BF, 0x98E5, 0x0B63, 0xA1C9,
        0x79BF, 0xF413, 0x82B1, 0xA895
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = subaru_tcu_hitachi_calculate_seed_key(requested_seed, keytogenerateindex, indextransformation);

    return key;
}


/*
 * SSM Block Write (K-Line)
 *
 * @return received response
 */
QByteArray FlashTcuSubaruHitachiM32rKline::send_subaru_tcu_sid_b0_block_write(uint32_t dataaddr, uint32_t datalen)
{
    QByteArray output;
    QByteArray received;
    int i;

    output.append((uint8_t)0xb0);
    output.append((uint8_t)(dataaddr >> 16) & 0xFF);
    output.append((uint8_t)(dataaddr >> 8) & 0xFF);
    output.append((uint8_t)dataaddr & 0xFF);
    output.append((uint8_t)0x80);
    output.append((uint8_t)0x18);
    output.append((uint8_t)0xf0);
    output.append((uint8_t)((datalen + 4) & 0xFF));

    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(100);
    received = serial->read_serial_data(datalen + 6, receive_timeout);
    //received = QByteArray::fromHex("80f018f8e0");
    delay(100);
    send_log_window_message("Recd: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * SSM Byte Read (K-Line)
 *
 * @return received response
 */
QByteArray FlashTcuSubaruHitachiM32rKline::send_subaru_tcu_sid_b8_byte_read(uint32_t dataaddr)
{
    QByteArray output;
    QByteArray received;

    output.append((uint8_t)0xb8);
    output.append((uint8_t)(dataaddr >> 16) & 0xFF);
    output.append((uint8_t)(dataaddr >> 8) & 0xFF);
    output.append((uint8_t)dataaddr & 0xFF);

    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(100);
    received = serial->read_serial_data(7, receive_timeout);
    //received = QByteArray::fromHex("80f018f8e0");
    delay(100);
    send_log_window_message("Recd: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * SSM Block Read (K-Line)
 *
 * @return received response
 */
QByteArray FlashTcuSubaruHitachiM32rKline::send_subaru_tcu_sid_a0_block_read(uint32_t dataaddr, uint32_t datalen)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    output.append((uint8_t)0xa0);
    output.append((uint8_t)0x00);
    output.append((uint8_t)(dataaddr >> 16) & 0xFF);
    output.append((uint8_t)(dataaddr >> 8) & 0xFF);
    output.append((uint8_t)dataaddr & 0xFF);
    output.append((uint8_t)datalen & 0xFF);

    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(100);
    received = serial->read_serial_data(datalen + 2 + 6, receive_timeout);
    //received = QByteArray::fromHex("80f01861e06000f000a04115ec6000f000a04115f06000f000a04115f46000f000a04115f86000f000a04115fc1fcef0002e7ff000fe00087e6000f000fe00022684ad806060072004fe0003e07e02f0002eef1fce6200f00081c200ff600401407d0e1081ab");
    delay(100);
    send_log_window_message("Recd: " + parse_message_to_hex(received), true, true);

    return received;
}


/*
 * Generate tcu hitachi can seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashTcuSubaruHitachiM32rKline::subaru_tcu_hitachi_generate_can_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex[]={
        0xF2CA, 0x2417, 0x21DE, 0x8475,
        0x39AB, 0xF767, 0x6204, 0x6BE0,
        0xBC63, 0x5988, 0x2845, 0x9846,
        0xEB97, 0x99DE, 0xC7DB, 0xEFAE
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = subaru_tcu_hitachi_calculate_seed_key(requested_seed, keytogenerateindex, indextransformation);

    return key;
}

/*
 * Calculate subaru tcu hitachi seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashTcuSubaruHitachiM32rKline::subaru_tcu_hitachi_calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray FlashTcuSubaruHitachiM32rKline::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
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
uint8_t FlashTcuSubaruHitachiM32rKline::calculate_checksum(QByteArray output, bool dec_0x100)
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
QString FlashTcuSubaruHitachiM32rKline::parse_message_to_hex(QByteArray received)
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
int FlashTcuSubaruHitachiM32rKline::send_log_window_message(QString message, bool timestamp, bool linefeed)
{
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']  ");

    if (timestamp)
        message = dateTimeString + message;
    if (linefeed)
        message = message + "\n";

    QString filename = "log.txt";
    QFile file(filename);
    //QFileInfo fileInfo(file.fileName());
    //QString file_name_str = fileInfo.fileName();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Append ))
    {
        //qDebug() << "Unable to open file for writing";
        QMessageBox::warning(this, tr("Ecu calibration file"), "Unable to open file for writing");
        return NULL;
    }

    file.write(message.toUtf8());
    file.close();

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

void FlashTcuSubaruHitachiM32rKline::set_progressbar_value(int value)
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

void FlashTcuSubaruHitachiM32rKline::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}


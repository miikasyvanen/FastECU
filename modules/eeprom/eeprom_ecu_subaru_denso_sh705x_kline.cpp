#include "eeprom_ecu_subaru_denso_sh705x_kline.h"

EepromEcuSubaruDensoSH705xKline::EepromEcuSubaruDensoSH705xKline(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::EcuOperationsWindow)
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

void EepromEcuSubaruDensoSH705xKline::run()
{
    this->show();

    int result = STATUS_ERROR;
    set_progressbar_value(0);

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
    emit LOG_D("MCU type: " + mcu_name + " " + mcu_type_string + " and index: " + QString::number(mcu_type_index), true, true);

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
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    serial->set_serial_port_baudrate("4800");
    tester_id = 0xF0;
    target_id = 0x10;
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

            emit LOG_I("Connecting to Subaru 07+ 32-bit CAN bootloader, please wait...", true, true);
            result = connect_bootloader();

            if (result == STATUS_SUCCESS && !kernel_alive)
            {
                emit external_logger("Preparing, please wait...");
                emit LOG_I("Initializing Subaru 07+ 32-bit CAN kernel upload, please wait...", true, true);
                result = upload_kernel(kernel, ecuCalDef->KernelStartAddr.toUInt(&ok, 16));
            }
            if (result == STATUS_SUCCESS)
            {
                if (cmd_type == "read")
                {
                    emit external_logger("Reading ROM, please wait...");
                    emit LOG_I("Reading EEPROM from Subaru 07+ 32-bit using CAN", true, true);
                    result = read_mem(flashdevices[mcu_type_index].eblocks[0].start, flashdevices[mcu_type_index].eblocks[0].len);
                }
                else if (cmd_type == "test_write" || cmd_type == "write")
                {
                    emit external_logger("Writing ROM, please wait...");
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
        LOG_D("Operation canceled", true, true);
        this->close();
        break;
    default:
        QMessageBox::warning(this, tr("Connecting to ECU"), "Unknown operation selected!");
        LOG_D("Unknown operation selected!", true, true);
        this->close();
        break;
    }
    if (result != STATUS_SUCCESS)
    {
        QMessageBox::warning(this, tr("ECU Operation"), "ECU operation failed, press OK to exit and try again");
    }
}

EepromEcuSubaruDensoSH705xKline::~EepromEcuSubaruDensoSH705xKline()
{
    delete ui;
}

void EepromEcuSubaruDensoSH705xKline::closeEvent(QCloseEvent *bar)
{
    kill_process = true;
}

/*
 * Connect to Subaru Denso K-Line bootloader 32bit ECUs
 *
 * @return success
 */
int EepromEcuSubaruDensoSH705xKline::connect_bootloader()
{
    QByteArray output;
    QByteArray received;
    QByteArray seed;
    QByteArray seed_key;

    QString msg;

    if (!serial->is_serial_port_open())
    {
        emit LOG_E("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->change_port_speed("62500");

    delay(100);

    emit LOG_I("Checking if kernel is already running...", true, true);

    emit LOG_I("Requesting kernel ID", true, true);

    received.clear();
    received = request_kernel_id();
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != (SUB_KERNEL_ID | 0x40))
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(8, received.length()-1)), true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        }
        else
        {
            received.remove(0, 5);
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

    serial->change_port_speed("4800");
    //serial->set_add_iso14230_header(false);
    delay(100);

    emit LOG_I("Initializing K-Line communications", true, true);
    received = send_sid_bf_ssm_init();
    if (received == "" || (uint8_t)received.at(4) != 0xFF)
    {
        QString nrc = FileActions::parse_nrc_message(received.mid(4, received.length()-1));
        emit LOG_E(nrc, true, true);
        return STATUS_ERROR;
    }

    received.remove(0, 8);
    received.remove(5, received.length() - 5);
    for (int i = 0; i < received.length(); i++)
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());

    QString ecuid = msg;
    emit LOG_I("Init Success: ECU ID = " + ecuid, true, true);
    if (cmd_type == "read")
        ecuCalDef->RomId = ecuid;

    emit LOG_I("Requesting to start communication", true, true);
    received = send_sid_81_start_communication();
    if (received == "" || (uint8_t)received.at(4) != 0xC1)
    {
        QString nrc = FileActions::parse_nrc_message(received.mid(4, received.length()-1));
        emit LOG_E(nrc, true, true);
        return STATUS_ERROR;
    }
    emit LOG_I("Start communication ok", true, true);

    emit LOG_I("Requesting timings params", true, true);
    received = send_sid_83_request_timings();
    if (received == "" || (uint8_t)received.at(4) != 0xC3)
    {
        QString nrc = FileActions::parse_nrc_message(received.mid(4, received.length()-1));
        emit LOG_E(nrc, true, true);
        return STATUS_ERROR;
    }
    emit LOG_I("Timing parameters ok", true, true);

    emit LOG_I("Requesting seed", true, true);
    received = send_sid_27_request_seed();
    if (received == "" || (uint8_t)received.at(4) != 0x67)
    {
        QString nrc = FileActions::parse_nrc_message(received.mid(4, received.length()-1));
        emit LOG_E(nrc, true, true);
        return STATUS_ERROR;
    }
    emit LOG_I("Seed request ok", true, true);

    seed.append(received.at(6));
    seed.append(received.at(7));
    seed.append(received.at(8));
    seed.append(received.at(9));

    msg = parse_message_to_hex(seed);
    emit LOG_I("Received seed: " + msg, true, true);

    if (flash_method.endsWith("_ecutek"))
        seed_key = generate_ecutek_seed_key(seed);
    else
        seed_key = generate_seed_key(seed);

    msg = parse_message_to_hex(seed_key);
    emit LOG_I("Calculated seed key: " + msg, true, true);

    emit LOG_I("Sending seed key to ECU", true, true);
    received = send_sid_27_send_seed_key(seed_key);
    if (received == "" || (uint8_t)received.at(4) != 0x67)
    {
        QString nrc = FileActions::parse_nrc_message(received.mid(4, received.length()-1));
        emit LOG_E(nrc, true, true);
        return STATUS_ERROR;
    }
    emit LOG_I("Seed key ok", true, true);

    emit LOG_I("Set session mode", true, true);
    received = send_sid_10_start_diagnostic();
    if (received == "" || (uint8_t)received.at(4) != 0x50)
    {
        QString nrc = FileActions::parse_nrc_message(received.mid(4, received.length()-1));
        emit LOG_E(nrc, true, true);
        return STATUS_ERROR;
    }
    emit LOG_I("Succesfully set to programming session", true, true);

    return STATUS_SUCCESS;
}

/*
 * Upload kernel to Subaru Denso K-Line 32bit ECUs
 *
 * @return success
 */
int EepromEcuSubaruDensoSH705xKline::upload_kernel(QString kernel, uint32_t kernel_start_addr)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray payload;
    QByteArray received;
    QByteArray msg;
    QByteArray pl_encr;
    uint32_t pl_len = 0;
    uint32_t start_address = 0;
    uint32_t len = 0;
    QByteArray cks_bypass;
    uint16_t chksum = 0;

    QString mcu_name;

    start_address = kernel_start_addr;//flashdevices[mcu_type_index].kblocks->start;
    emit LOG_D("Start address to upload kernel: " + QString::number(start_address), true, true);

    if (!serial->is_serial_port_open())
    {
        emit LOG_E("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    //serial->set_add_iso14230_header(false);

    // Check kernel file
    if (!file.open(QIODevice::ReadOnly ))
    {
        emit LOG_E("Unable to open kernel file for reading", true, true);
        return STATUS_ERROR;
    }

    pl_encr = file.readAll();
    /*
    pl_encr.append((uint8_t)0x00);
    pl_encr.append((uint8_t)0x00);
    pl_encr.append((uint8_t)0x00);
    pl_encr.append((uint8_t)0x00);
*/
    pl_len = pl_encr.length();
    pl_len = (pl_len + 3) & ~3;
    len = pl_len;
    /*
    uint16_t balance_value = 0;
    for (uint32_t i = 0; i < pl_len; i++)
        chksum += (uint8_t)pl_encr.at(i);

    balance_value = 0x5aa5 - chksum;
    emit LOG_D("Checksum: " + QString::number(chksum, 16), true, true);
    emit LOG_D("Balance value: " + QString::number(balance_value, 16), true, true);
    pl_encr.remove(pl_encr.length()-2, 2);
    pl_encr.append((balance_value >> 8) & 0xff);
    pl_encr.append(balance_value & 0xff);
*/
    // Change port speed to upload kernel
    if (serial->change_port_speed("15625"))
        return STATUS_ERROR;

    emit LOG_I("Requesting kernel upload'", true, true);
    received = send_sid_34_request_upload(start_address, len);
    if (received == "" || (uint8_t)received.at(4) != 0x74)
        return STATUS_ERROR;
    emit LOG_I("Kernel upload request ok, uploading now, please wait...", true, true);

    //pl_encr = sub_encrypt_buf(pl_encr, (uint32_t) pl_len);
    pl_encr = encrypt_payload(pl_encr, pl_len);

    emit LOG_I("Transfer kernel data", true, true);
    received = send_sid_36_transferdata(start_address, pl_encr, len);
    if (received == "" || (uint8_t)received.at(4) != 0x76)
        return STATUS_ERROR;
    //emit LOG_I("Kernel uploaded", true, true);

    //emit LOG_I("Kernel checksum bypass", true, true);
    received = send_sid_34_request_upload(start_address + len, 4);
    if (received == "" || (uint8_t)received.at(4) != 0x74)
        return STATUS_ERROR;

    cks_bypass.append((uint8_t)0x00);
    cks_bypass.append((uint8_t)0x00);
    cks_bypass.append((uint8_t)0x5A);
    cks_bypass.append((uint8_t)0xA5);

    cks_bypass = encrypt_payload(cks_bypass, (uint32_t) 4);

    // sid36 transferData for checksum bypass
    //emit LOG_D("Send 'sid36_transfer_data' for chksum bypass", true, true);
    received = send_sid_36_transferdata(start_address + len, cks_bypass, 4);
    if (received == "" || (uint8_t)received.at(4) != 0x76)
        return STATUS_ERROR;

    //emit LOG_I("Checksum bypass ok", true, true);
    emit LOG_I("Kernel uploaded", true, true);

    emit LOG_I("Jump to kernel", true, true);
    received = send_sid_31_start_routine();
    if (received == "" || (uint8_t)received.at(4) != 0x71)
        return STATUS_ERROR;
    emit LOG_I("Kernel started, initializing...", true, true);

    serial->change_port_speed("62500");

    emit LOG_I("Requesting kernel ID...", true, true);

    for (int i = 0; i < 10; i++)
    {
        if (kill_process)
            return STATUS_ERROR;

        received = request_kernel_id();
        if (received.length() > 4)
        {
            if ((uint8_t)received.at(0) == ((SUB_KERNEL_START_COMM >> 8) & 0xFF) && (uint8_t)received.at(1) == (SUB_KERNEL_START_COMM & 0xFF) && (uint8_t)received.at(4) == (SUB_KERNEL_ID | 0x40))
            {
                received.remove(0, 5);
                received.remove(received.length() - 1, 1);
                emit LOG_I("Kernel ID: " + received, true, true);
                emit LOG_D("Kernel ID: " + parse_message_to_hex(received), true, true);
                delay(100);
                kernel_alive = true;
                return STATUS_SUCCESS;
            }
        }
        delay(200);
    }
    return STATUS_ERROR;
}

/*
 * Read memory from Subaru Denso K-Line 32bit ECUs, nisprog kernel
 *
 * @return success
 */
int EepromEcuSubaruDensoSH705xKline::read_mem(uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray pagedata;
    QByteArray mapdata;
    uint32_t cplen = 0;
    uint32_t timeout = 0;

    uint32_t skip_start = 0;
    uint32_t addr = 0;
    uint32_t willget = 0;
    uint32_t len_done = 0;

    skip_start = start_addr & (32 - 1);
    addr = start_addr - skip_start;
    willget = (skip_start + length + 31) & ~(32 - 1);
    len_done = 0;

    LOG_D("Read EEPROM start at: 0x" + QString::number(start_addr, 16) + " and size of 0x" + QString::number(length, 16), true, true);

    #define NP10_MAXBLKS    32   //# of blocks to request per loop. Too high might flood us
    serial->set_add_iso14230_header(true);

    output.clear();
    output.append((uint8_t)SID_DUMP);
    output.append((uint8_t)EEPROM_MODE);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    timer.start();
    set_progressbar_value(0);

    while (willget)
    {
        if (kill_process)
            return STATUS_ERROR;

        uint32_t numblocks = 0;
        unsigned curspeed = 0, tleft;
        float pleft = 0;
        unsigned long chrono;

        //delay(1);
        numblocks = willget / 32;

        if (numblocks > NP10_MAXBLKS)
            numblocks = NP10_MAXBLKS;

        uint32_t curblock = (addr / 32);

        uint32_t pagesize = numblocks * 32 + numblocks * 3;
        pleft = (float)(addr - start_addr) / (float)length * 100.0f;
        set_progressbar_value(pleft);

        output[2] = numblocks >> 8;
        output[3] = numblocks >> 0;

        output[4] = curblock >> 8;
        output[5] = curblock >> 0;

        received = serial->write_serial_data_echo_check(output);
        delay(500);
        timeout = 0;
        pagedata.clear();
        while ((uint32_t)pagedata.length() < pagesize && timeout < 5)
        {
            if (kill_process)
                return STATUS_ERROR;
            delay(100);
            received = serial->read_serial_data(pagesize / numblocks, serial_read_timeout);
            if (received.length()) {
                pagedata.append(received);
            }
            else
                timeout++;

            received.clear();
        }
        for (uint32_t i = 0; i < numblocks; i++)
        {
            pagedata.remove(i * 32, 2);
            pagedata.remove((i + 1) * 32, 1);
            emit LOG_I(parse_message_to_hex(pagedata.mid(i * 32, 32)), false, true);
        }
        if (timeout >= 1000)
        {
            emit LOG_I("Page data timeout!", true, true);
            return STATUS_ERROR;
        }
        mapdata.append(pagedata);

        // don't count skipped first bytes //
        cplen = (numblocks * 32) - skip_start; //this is the actual # of valid bytes in buf[]
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
        addr += (numblocks * 32);
        willget -= (numblocks * 32);

    }

    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*
 * 8bit checksum
 *
 * @return
 */
uint8_t EepromEcuSubaruDensoSH705xKline::cks_add8(QByteArray chksum_data, unsigned len)
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
 * ECU init
 *
 * @return ECU ID and capabilities
 */
QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_bf_ssm_init()
{
    QByteArray output;
    QByteArray received;

    output.append((uint8_t)0xBF);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    received = serial->read_serial_data(8, serial_read_extra_long_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Start diagnostic connection
 *
 * @return received response
 */
QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_81_start_communication()
{
    QByteArray output;
    QByteArray received;

    output.append((uint8_t)0x81);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    received = serial->read_serial_data(8, serial_read_extra_long_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Start diagnostic connection
 *
 * @return received response
 */
QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_83_request_timings()
{
    QByteArray output;
    QByteArray received;

    output.append((uint8_t)0x83);
    output.append((uint8_t)0x00);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    received = serial->read_serial_data(8, serial_read_extra_long_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Request seed
 *
 * @return seed (4 bytes)
 */
QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_27_request_seed()
{
    QByteArray output;
    QByteArray received;

    output.append((uint8_t)0x27);
    output.append((uint8_t)0x01);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    received = serial->read_serial_data(8, serial_read_extra_long_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Send seed key
 *
 * @return received response
 */
QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_27_send_seed_key(QByteArray seed_key)
{
    QByteArray output;
    QByteArray received;

    output.append((uint8_t)0x27);
    output.append((uint8_t)0x02);
    output.append(seed_key);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    received = serial->read_serial_data(8, serial_read_extra_long_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Request start diagnostic
 *
 * @return received response
 */
QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_10_start_diagnostic()
{
    QByteArray output;
    QByteArray received;

    output.append((uint8_t)0x10);
    output.append((uint8_t)0x85);
    output.append((uint8_t)0x02);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    received = serial->read_serial_data(8, serial_read_extra_long_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Request data upload (kernel)
 *
 * @return received response
 */
QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_34_request_upload(uint32_t dataaddr, uint32_t datalen)
{
    QByteArray output;
    QByteArray received;

    output.append((uint8_t)0x34);
    output.append((uint8_t)(dataaddr >> 16) & 0xFF);
    output.append((uint8_t)(dataaddr >> 8) & 0xFF);
    output.append((uint8_t)dataaddr & 0xFF);
    output.append((uint8_t)0x04);
    output.append((uint8_t)(datalen >> 16) & 0xFF);
    output.append((uint8_t)(datalen >> 8) & 0xFF);
    output.append((uint8_t)datalen & 0xFF);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    received = serial->read_serial_data(6, serial_read_extra_long_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Transfer data (kernel)
 *
 * @return received response
 */
QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_36_transferdata(uint32_t dataaddr, QByteArray buf, uint32_t len)
{
    QByteArray output;
    QByteArray received;
    uint32_t blockaddr = 0;
    uint32_t blocksize = 0x80;
    uint16_t blockno = 0;
    uint16_t maxblocks = 0;

    len &= ~0x03;
    if (!buf.length() || !len) {
        emit LOG_E("Error in kernel data length!", true, true);
        return NULL;
    }

    maxblocks = (len - 1) / blocksize;  // number of 128 byte blocks - 1

    set_progressbar_value(0);

    for (blockno = 0; blockno <= maxblocks; blockno++)
    {
        if (kill_process)
            return NULL;

        blockaddr = dataaddr + blockno * blocksize;
        output.clear();
        output.append(0x36);
        output.append((uint8_t)(blockaddr >> 16) & 0xFF);
        output.append((uint8_t)(blockaddr >> 8) & 0xFF);
        output.append((uint8_t)blockaddr & 0xFF);
        emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);

        if (blockno == maxblocks)
        {
            for (uint32_t i = 0; i < len; i++)
            {
                output.append(buf.at(i + blockno * blocksize));
            }
        }
        else
        {
            for (uint32_t i = 0; i < blocksize; i++)
            {
                output.append(buf.at(i + blockno * blocksize));
            }
            len -= blocksize;
        }

        output = add_ssm_header(output, tester_id, target_id, false);
        serial->write_serial_data_echo_check(output);
        received = serial->read_serial_data(6, serial_read_timeout);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        if (received.length() > 4)
        {
            if ((uint8_t)received.at(4) != 0x76)
            {
                emit LOG_E("Write data failed!", true, true);
                emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
                return received;
            }
        }
        else
        {
            emit LOG_E("Write data failed!", true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return received;
        }
        float pleft = (float)blockno / (float)maxblocks * 100;
        set_progressbar_value(pleft);
    }

    set_progressbar_value(100);

    return received;
}

QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_31_start_routine()
{
    QByteArray output;
    QByteArray received;

    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x01);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    received = serial->read_serial_data(8, serial_read_extra_long_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Generate denso kline seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray EepromEcuSubaruDensoSH705xKline::generate_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex_1[]={
        0x53DA, 0x33BC, 0x72EB, 0x437D,
        0x7CA3, 0x3382, 0x834F, 0x3608,
        0xAFB8, 0x503D, 0xDBA3, 0x9D34,
        0x3563, 0x6B70, 0x6E74, 0x88F0
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

    key = calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

/*
 * Generate denso kline ecutek seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray EepromEcuSubaruDensoSH705xKline::generate_ecutek_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex_1[]={
        0x53DA, 0x33BC, 0x72EB, 0x437D,
        0x7CA3, 0x3382, 0x834F, 0x3608,
        0xAFB8, 0x503D, 0xDBA3, 0x9D34,
        0x3563, 0x6B70, 0x6E74, 0x88F0
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

    key = calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

/*
 * Calculate denso seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray EepromEcuSubaruDensoSH705xKline::calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
QByteArray EepromEcuSubaruDensoSH705xKline::encrypt_payload(QByteArray buf, uint32_t len)
{
    QByteArray encrypted;

    uint16_t keytogenerateindex[]={
        0x7856, 0xCE22, 0xF513, 0x6E86
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

QByteArray EepromEcuSubaruDensoSH705xKline::decrypt_payload(QByteArray buf, uint32_t len)
{
    QByteArray decrypted;

    uint16_t keytogenerateindex[]={
        0x6E86, 0xF513, 0xCE22, 0x7856
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    decrypted = calculate_payload(buf, len, keytogenerateindex, indextransformation);

    return decrypted;
}

QByteArray EepromEcuSubaruDensoSH705xKline::calculate_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
QByteArray EepromEcuSubaruDensoSH705xKline::request_kernel_id()
{
    QByteArray output;
    QByteArray received;
    QByteArray kernelid;
    uint32_t datalen = 0;
    uint8_t chksum = 0;

    request_denso_kernel_id = true;

    datalen = 0;
    output.clear();
    output.append((uint8_t)((SUB_KERNEL_START_COMM >> 8) & 0xFF));
    output.append((uint8_t)(SUB_KERNEL_START_COMM & 0xFF));
    output.append((uint8_t)((datalen + 1) >> 8) & 0xFF);
    output.append((uint8_t)(datalen + 1) & 0xFF);
    output.append((uint8_t)(SUB_KERNEL_ID & 0xFF));
    chksum = calculate_checksum(output, false);
    output.append((uint8_t)chksum & 0xFF);

    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(200);
    received = serial->read_serial_data(100, serial_read_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    kernelid = received;

    request_denso_kernel_id = false;

    return kernelid;
}

/*
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray EepromEcuSubaruDensoSH705xKline::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
{
    uint8_t length = output.length();

    output.insert(0, (uint8_t)0x80);
    output.insert(1, target_id & 0xFF);
    output.insert(2, tester_id & 0xFF);
    output.insert(3, length);

    output.append(calculate_checksum(output, dec_0x100));

    //emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    return output;
}

/*
 * Calculate SSM checksum to message
 *
 * @return 8-bit checksum
 */
uint8_t EepromEcuSubaruDensoSH705xKline::calculate_checksum(QByteArray output, bool dec_0x100)
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
QString EepromEcuSubaruDensoSH705xKline::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

void EepromEcuSubaruDensoSH705xKline::set_progressbar_value(int value)
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

void EepromEcuSubaruDensoSH705xKline::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

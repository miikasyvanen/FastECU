#include "flash_tcu_subaru_hitachi_m32r_kline.h"
#include "serial_port_actions.h"

FlashTcuSubaruHitachiM32rKline::FlashTcuSubaruHitachiM32rKline(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
    : QDialog(parent)
    , ecuCalDef(ecuCalDef)
    , cmd_type(cmd_type)
    , ui(new Ui::EcuOperationsWindow)
{
    ui->setupUi(this);

    if (cmd_type == "test_write")
        this->setWindowTitle("Test write ROM " + ecuCalDef->FileName + " to TCU (stops before erase)");
    else if (cmd_type == "write")
        this->setWindowTitle("Write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "read")
        this->setWindowTitle("Read ROM from TCU");

    this->serial = serial;
}

void FlashTcuSubaruHitachiM32rKline::run()
{
    this->show();

    if (cmd_type != "read" && cmd_type != "write" && cmd_type != "test_write")
    {
        emit LOG_E("Operation '" + cmd_type + "' is not supported with this flash method", true, true);
        QMessageBox::warning(this, tr("ECU Operation"), "Operation '" + cmd_type + "' is not supported with this flash method!");
        return;
    }

    int result = STATUS_ERROR;
    set_progressbar_value(0);

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
        emit LOG_I("Test write (dry run, stops before erase) with flashmethod '" + flash_method + "'", true, true);
    }
    else if (cmd_type == "write")
    {
        test_write = false;
        emit LOG_I("Write memory with flashmethod '" + flash_method + "' and kernel '" + ecuCalDef->Kernel + "'", true, true);
    }

    // Writes: check the ROM file and the built-in crypto before touching the TCU
    if (cmd_type == "test_write" || cmd_type == "write")
    {
        if (self_test() != STATUS_SUCCESS || check_rom_file() != STATUS_SUCCESS)
        {
            emit external_logger("Finished");
            QMessageBox::warning(this, tr("TCU Operation"), "TCU write blocked before connecting, see log. Nothing was sent to the TCU.");
            return;
        }
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
            if (cmd_type == "read")
            {
                if (flash_method.endsWith("_recovery"))
                {
                    emit LOG_E("Reading is not supported in recovery mode, use 'sub_tcu_hitachi_m32r_kline'", true, true);
                    result = STATUS_ERROR;
                }
                else
                {
                    emit LOG_I("Connecting to Subaru TCU Hitachi K-Line, please wait...", true, true);
                    result = connect_bootloader();

                    if (result == STATUS_SUCCESS)
                    {
                        emit external_logger("Reading ROM, please wait...");
                        emit LOG_I("Reading ROM from TCU using K-Line", true, true);
                        result = read_a0_rom(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                        //result = read_a0_ram_subaru_tcu_hitachi_kline(0x80000, 0x100000);
                        //result = read_b8_subaru_tcu_hitachi_kline(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                        //result = read_b0_subaru_tcu_hitachi_kline(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                    }
                }
            }
            else if (cmd_type == "test_write" || cmd_type == "write")
            {
                emit external_logger("Writing ROM, please wait...");
                if (flash_method.endsWith("_recovery"))
                    result = write_tcu_recovery();
                else
                    result = write_tcu();
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
            LOG_D("Operation canceled", true, true);
            this->close();
            break;
        default:
            QMessageBox::warning(this, tr("Connecting to ECU"), "Unknown operation selected!");
            LOG_D("Unknown operation selected!", true, true);
            this->close();
            break;
    }
}

FlashTcuSubaruHitachiM32rKline::~FlashTcuSubaruHitachiM32rKline()
{
    delete ui;
}

void FlashTcuSubaruHitachiM32rKline::closeEvent(QCloseEvent *event)
{
    // Aborting after erase would leave the TCU without a program; every step has its own timeout
    if (flash_in_progress)
    {
        event->ignore();
        QMessageBox::warning(this, tr("TCU Operation"), "TCU flash in progress, the window cannot be closed until it finishes.");
        return;
    }
    kill_process = true;
}

/*
 * Connect to Subaru TCU Hitachi CAN bootloader
 *
 * @return success
 */
int FlashTcuSubaruHitachiM32rKline::connect_bootloader()
{
    QByteArray output;
    QByteArray received;
    QByteArray seed;
    QByteArray seed_key;

    QString msg;

    if (!serial->is_serial_port_open())
    {
        emit LOG_I("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    delay(100);

    emit LOG_I("Initializing k-line communications", true, true);

    emit LOG_I("Requesting ECU ID", true, true);
    output.clear();
    output.append((uint8_t)0xBF);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_timeout);
    
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(4) != 0xFF)
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }

    received.remove(0, 8);
    received.remove(5, received.length() - 5);
    for (int i = 0; i < received.length(); i++)
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());

    QString ecuid = msg;
    tcu_rom_id = received;
    emit LOG_I("ECU ID: " + ecuid, true, true);
    if (cmd_type == "read")
        ecuCalDef->RomId = ecuid + "_";

    emit LOG_I("Requesting to start communication", true, true);
    output.clear();
    output.append((uint8_t)0x81);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_timeout);
    
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(4) != 0xC1)
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }
    emit LOG_I("Start communication ok", true, true);

    emit LOG_I("Requesting timings params", true, true);
    output.clear();
    output.append((uint8_t)0x83);
    output.append((uint8_t)0x00);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_timeout);
    
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(4) != 0xC3)
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }
    emit LOG_I("Timing parameters ok", true, true);

    emit LOG_I("Requesting seed", true, true);
    output.clear();
    output.append((uint8_t)0x27);
    output.append((uint8_t)0x01);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_timeout);
    
    if (received.length() > 9)
    {
        if ((uint8_t)received.at(4) != 0x67 || (uint8_t)received.at(5) != 0x01)
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }
    emit LOG_I("Seed request ok", true, true);

    seed.append(received.at(6));
    seed.append(received.at(7));
    seed.append(received.at(8));
    seed.append(received.at(9));

    msg = parse_message_to_hex(seed);
    emit LOG_I("Received seed: " + msg, true, true);

    seed_key = generate_kline_seed_key(seed);

    msg = parse_message_to_hex(seed_key);
    emit LOG_I("Calculated seed key: " + msg, true, true);

    emit LOG_I("Sending seed key to ECU", true, true);
    output.clear();
    output.append((uint8_t)0x27);
    output.append((uint8_t)0x02);
    output.append(seed_key);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_timeout);
    
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) != 0x67 || (uint8_t)received.at(5) != 0x02)
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }
    emit LOG_I("Seed key ok", true, true);
/*
    emit LOG_I("Set session mode", true, true);
    output.clear();
    output.append((uint8_t)0x10);
    output.append((uint8_t)0x85);
    output.append((uint8_t)0x02);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_timeout);
    
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(4) != 0x50)
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(4, received.length()-1)), true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }
    emit LOG_I("Succesfully set to programming session", true, true);
*/
    return STATUS_SUCCESS;
}


/*
 * For reading a portion of ROM using a0 command
 *
 *
 */int FlashTcuSubaruHitachiM32rKline::read_a0_rom(uint32_t start_addr, uint32_t length)
{

    QByteArray received;
    QByteArray msg;
    QByteArray mapdata;

    uint32_t curr_addr, block_count, num_blocks;
    uint32_t tail_size, block_len, loop_ctr, block_size;

    block_size = 96;
    //length = 0x400;   // hack for testing

    emit LOG_I("Commencing TCU ROM read", true, true);

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
        emit LOG_I(msg, true, true);

        for (loop_ctr = 0; loop_ctr < 5; loop_ctr++)
        {
            received = send_sid_a0_block_read(curr_addr, block_len - 1);
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

    return STATUS_SUCCESS;
}

/*
 * For reading a portion of ROM using b8 command
 *
 *
 */int FlashTcuSubaruHitachiM32rKline::read_b8(uint32_t start_addr, uint32_t length)
{

    QByteArray received;
    QByteArray msg;
    QByteArray mapdata;

    uint32_t curr_addr, block_count, num_blocks;
    uint32_t tail_size, block_len, loop_ctr, block_size;

    block_size = 1;
    length = 0x10;  // hack for testing

    emit LOG_I("Commencing TCU ROM read", true, true);

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
        emit LOG_I(msg, true, true);

        for (loop_ctr = 0; loop_ctr < 5; loop_ctr++)
        {
            received = send_sid_b8_byte_read(curr_addr);
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

    return STATUS_SUCCESS;
}

/*
 * For reading a portion of ROM using b0 command
 *
 *
 */int FlashTcuSubaruHitachiM32rKline::read_b0(uint32_t start_addr, uint32_t length)
{
    Q_UNUSED(start_addr)

    QByteArray received;
    QByteArray msg;
    QByteArray mapdata;

    uint32_t curr_addr, block_count, num_blocks;
    uint32_t tail_size, block_len, loop_ctr, block_size;

    block_size = 96;
    length = 4;  // for testing

    emit LOG_I("Commencing TCU ROM read", true, true);

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
        emit LOG_I(msg, true, true);

        for (loop_ctr = 0; loop_ctr < 5; loop_ctr++)
        {
            received = send_sid_b0_block_write(curr_addr, block_len);
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

    return STATUS_SUCCESS;
}

/*
 * For reading a portion of RAM
 *
 *
 */
int FlashTcuSubaruHitachiM32rKline::read_a0_ram(uint32_t start_addr, uint32_t length)
{

    QByteArray received;
    QByteArray msg;
    QByteArray mapdata;

    uint32_t curr_addr, block_count, num_blocks;
    uint32_t tail_size, block_len, loop_ctr, block_size;

    block_size = 96;
    length = 0x400; // hack for testing

    emit LOG_I("Commencing TCU ROM read", true, true);

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
        emit LOG_I(msg, true, true);

        for (loop_ctr = 0; loop_ctr < 5; loop_ctr++)
        {
            received = send_sid_a0_block_read(curr_addr, block_len - 1);
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

    return STATUS_SUCCESS;
}

/*
 * ============================================================================
 * WRITE SUPPORT for the 0x5FFFF-generation Hitachi/JECS M32R K-Line TCU
 * (e.g. 91FE216300, chip WA12212963WWN). Protocol decoded from the TCU ROM:
 *
 *  App session, 4800 baud (target 0x18):
 *    81 -> C1 EF 8F, 83 00 -> C3 00 00 FF 00 FF 00
 *    27 01 -> 67 01 <seed4>, 27 02 <key4> -> 67 02 34 (2 bad keys = lockout)
 *    10 85 02 -> 50 85 only if unlocked AND lever P (RAM 0x8047E7 == 0x84,
 *    0x804935 == 0x01), else 7F 10 22. The TCU then runs its on-board
 *    kernel from RAM (application stopped, flash untouched until erase).
 *  Kernel, 15625 baud:
 *    34 00 00 00 04 06 00 00 -> 74 84   (start must be 0: the 36 range test
 *                                        compares addr + 128 against LENGTH)
 *    31 02                   -> 71 02 00 00 00 00   erase 0x8000-0x5FFFF
 *    36 <addr24> <128 B enc> -> 76      sequential 0x8000..0x5FFF80
 *    31 01 02                -> 71 01 02  (kernel sum check low16 == 0x5AA5)
 *    after verify the kernel goes silent; ignition off >= 10 s restarts.
 *  Recovery: when the program checksum / a block marker is bad, the boot
 *  block listener answers 34 at 15625 without seed/key and starts the same
 *  kernel. 0x0-0x7FFF is never erased or written.
 * ============================================================================
 */

/*
 * Check seed/key and payload encryption against known vectors
 * (vectors proven against the TCU ROM code in an emulator and on the car)
 *
 * @return success
 */
int FlashTcuSubaruHitachiM32rKline::self_test()
{
    // Payload encryption: 8 zero bytes -> 62 88 9F FF 62 88 9F FF
    if (encrypt_payload(QByteArray(8, '\0')) != QByteArray::fromHex("62889fff62889fff") ||
        encrypt_payload(QByteArray::fromHex("0011223344556677")) != QByteArray::fromHex("ce43200ff671d593"))
    {
        emit LOG_E("Internal self test failed: payload encryption mismatch, write blocked", true, true);
        return STATUS_ERROR;
    }

    // Seed/key: model vector and the pair logged from a real 91FE216300 TCU
    if (generate_kline_seed_key(QByteArray::fromHex("0c5df251")) != QByteArray::fromHex("12345678") ||
        generate_kline_seed_key(QByteArray::fromHex("42e62972")) != QByteArray::fromHex("12a79014"))
    {
        emit LOG_E("Internal self test failed: seed/key mismatch, write blocked", true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Internal self test OK (seed/key, payload encryption)", true, true);
    return STATUS_SUCCESS;
}

/*
 * Check that the ROM file is a valid 0x5FFFF-generation TCU image
 *
 * @return success
 */
int FlashTcuSubaruHitachiM32rKline::check_rom_file()
{
    QString msg;
    QByteArray rom = ecuCalDef->FullRomData;

    if (!ChecksumTcuSubaruHitachiM32rKline::detect_layout(rom, &msg))
    {
        emit LOG_E("ROM file rejected: " + msg, true, true);
        return STATUS_ERROR;
    }
    if (!ChecksumTcuSubaruHitachiM32rKline::verify_checksum(rom, &msg))
    {
        emit LOG_E("ROM file rejected, checksums invalid: " + msg, true, true);
        return STATUS_ERROR;
    }

    QByteArray rom_id = rom.mid(0x802A, 5);
    if (rom_id == QByteArray(5, (char)0xFF) || rom_id == QByteArray(5, '\0'))
    {
        emit LOG_E("ROM file rejected: no ROM ID at 0x802A", true, true);
        return STATUS_ERROR;
    }

    // Only TCUs whose on-board kernel and security use the keys this module implements:
    // kernel payload key (boot block 0x5AAC), index table (0x5AB4), seed/key table (0x5F960)
    const QByteArray kernel_key = QByteArray::fromHex("3e27b29166401336");
    const QByteArray index_table = QByteArray::fromHex("05060701090c0d080a0d020b0f0400030b0406000f020d09050c010a030d0e08");
    const QByteArray seed_table = QByteArray::fromHex("0fe9ca585e90dff1690bf59117945c7ba7bf98e50b63a1c979bff41382b1a895");
    if (rom.mid(0x5AAC, 8) != kernel_key || rom.mid(0x5AB4, 32) != index_table || rom.mid(0x5F960, 32) != seed_table)
    {
        emit LOG_E("ROM file rejected: flash kernel / security keys differ from the supported TCU (or boot block not in file). "
                   "Writing this TCU is not supported by this module.", true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("ROM file OK: ID " + parse_message_to_hex(rom_id) + "| " + msg, true, true);
    return STATUS_SUCCESS;
}

/*
 * Battery voltage check (OpenPort only, other adapters ask the user)
 *
 * @return success
 */
int FlashTcuSubaruHitachiM32rKline::check_battery()
{
    // First call can return a cached value
    serial->read_vbatt();
    delay(100);
    unsigned long vbatt = serial->read_vbatt();

    if (vbatt > 1000)
    {
        emit LOG_I("Battery voltage: " + QString::number(vbatt / 1000.0, 'f', 2) + " V", true, true);

        if (vbatt < tcu_vbatt_min_mv)
        {
            emit LOG_E("Battery voltage too low for flashing, connect a battery charger", true, true);
            return STATUS_ERROR;
        }
        if (vbatt < tcu_vbatt_warn_mv)
        {
            int ret = QMessageBox::warning(this, tr("Battery voltage"),
                                           "Battery voltage is " + QString::number(vbatt / 1000.0, 'f', 2) +
                                           " V. A battery charger should be connected during flashing.\n\nContinue anyway?",
                                           QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (ret != QMessageBox::Yes)
                return STATUS_ERROR;
        }
        return STATUS_SUCCESS;
    }

    int ret = QMessageBox::warning(this, tr("Battery voltage"),
                                   "This adapter cannot read battery voltage.\n\nIs a battery charger connected and the voltage at least 12.5 V?",
                                   QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return STATUS_ERROR;

    return STATUS_SUCCESS;
}

/*
 * Live checks before entering the programming session (no user dialogs here):
 * engine stopped (SSM 0x0E/0x0F), lever in P (RAM 0x8047E7 / 0x804935)
 *
 * @return success
 */
int FlashTcuSubaruHitachiM32rKline::check_tcu_conditions()
{
    QByteArray received;

    received = send_sid_a8_read(QList<uint32_t>() << 0x00000E << 0x00000F << 0x8047E7 << 0x804935);
    if (received.length() < 10 || (uint8_t)received.at(4) != 0xE8)
    {
        emit LOG_E("Could not read engine speed / lever position from TCU: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    uint16_t engine_speed = (((uint8_t)received.at(5) << 8) | (uint8_t)received.at(6)) / 4;
    uint8_t lever = (uint8_t)received.at(7);
    uint8_t lever_flag = (uint8_t)received.at(8);

    emit LOG_I("Engine speed: " + QString::number(engine_speed) + " rpm, lever code: 0x" + QString::number(lever, 16) +
               ", lever flag: 0x" + QString::number(lever_flag, 16), true, true);

    if (engine_speed != 0)
    {
        emit LOG_E("Engine is running: stop the engine, leave ignition ON", true, true);
        return STATUS_ERROR;
    }

    if (lever == 0xFF && lever_flag == 0xFF)
    {
        emit LOG_W("Lever position not readable, the TCU checks it itself (P required)", true, true);
    }
    else if (lever != 0x84 || lever_flag != 0x01)
    {
        emit LOG_E("Select lever must be in P", true, true);
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
}

/*
 * Last confirmation before the TCU application is stopped
 *
 * @return success if user confirmed
 */
int FlashTcuSubaruHitachiM32rKline::confirm_write(bool recovery)
{
    QString text;

    if (test_write)
        text = "DRY RUN (test write): the TCU program will be stopped and the flash kernel started, but NOTHING will be "
               "erased or written. Afterwards turn ignition OFF for at least 10 seconds.\n\nContinue?";
    else
        text = QString(recovery ? "RECOVERY WRITE. The TCU ROM ID cannot be read in recovery mode: make sure this file "
                                  "belongs to this TCU (same ROM ID as the original).\n\n" : "") +
               "The TCU flash 0x8000-0x5FFFF will be ERASED and rewritten (about 5 minutes).\n\n"
               "- engine OFF, ignition ON, lever in P\n"
               "- battery charger connected\n"
               "- do not touch ignition, adapter or cable until finished\n\n"
               "Continue?";

    int ret = QMessageBox::warning(this, tr("Write TCU"), text, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes)
    {
        emit LOG_I("Write canceled by user, nothing sent", true, true);
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
}

/*
 * SSM 10 85 02: start reflash session (TCU starts its on-board kernel)
 *
 * @return success
 */
int FlashTcuSubaruHitachiM32rKline::enter_programming_session()
{
    QByteArray output;
    QByteArray received;

    emit LOG_I("Requesting programming session", true, true);
    output.append((uint8_t)0x10);
    output.append((uint8_t)0x85);
    output.append((uint8_t)0x02);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    received = read_ssm_response(serial_read_timeout);

    if (received.length() > 5 && (uint8_t)received.at(4) == 0x50 && (uint8_t)received.at(5) == 0x85)
    {
        emit LOG_I("Programming session ok, TCU flash kernel starting", true, true);
        return STATUS_SUCCESS;
    }

    if (received.length() > 6 && (uint8_t)received.at(4) == 0x7F)
        emit LOG_E("Programming session refused (lever not in P or security not unlocked): " + parse_message_to_hex(received), true, true);
    else
        emit LOG_E("No valid response to programming session request: " + parse_message_to_hex(received), true, true);

    return STATUS_ERROR;
}

/*
 * Kernel 34: set range start 0, length 0x60000
 *
 * @return success
 */
int FlashTcuSubaruHitachiM32rKline::kernel_set_range(int tries)
{
    QByteArray output;
    QByteArray received;

    output.append((uint8_t)0x34);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x04);
    output.append((uint8_t)((tcu_range_length >> 16) & 0xFF));
    output.append((uint8_t)((tcu_range_length >> 8) & 0xFF));
    output.append((uint8_t)(tcu_range_length & 0xFF));

    for (int i = 0; i < tries && !kill_process; i++)
    {
        received = send_kernel_request(output, serial_read_timeout);

        if (received.length() > 5 && (uint8_t)received.at(4) == 0x74 && (uint8_t)received.at(5) == 0x84)
        {
            emit LOG_I("Kernel range set: start 0x000000 length 0x" + QString::number(tcu_range_length, 16), true, true);
            return STATUS_SUCCESS;
        }
        if (received.length() > 4 && (uint8_t)received.at(4) == 0x7F)
        {
            emit LOG_E("Kernel refused range: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
        delay(200);
    }

    emit LOG_E("No answer from TCU flash kernel at " + kernel_baudrate + " baud", true, true);
    return STATUS_ERROR;
}

/*
 * Kernel 31 02: erase 0x8000-0x5FFFF
 *
 * @return success
 */
int FlashTcuSubaruHitachiM32rKline::kernel_erase()
{
    QByteArray output;
    QByteArray received;

    emit LOG_I("Erasing TCU flash 0x008000-0x05FFFF, please wait...", true, true);
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x02);
    received = send_kernel_request(output, tcu_erase_timeout);

    if (received.length() > 5 && (uint8_t)received.at(4) == 0x71 && (uint8_t)received.at(5) == 0x02)
    {
        emit LOG_I("Flash erased", true, true);
        return STATUS_SUCCESS;
    }

    emit LOG_E("Flash erase failed: " + parse_message_to_hex(received), true, true);
    return STATUS_ERROR;
}

/*
 * Kernel 36: write encrypted 0x8000-0x5FFFF in 128-byte blocks, strictly sequential
 *
 * @return success
 */
int FlashTcuSubaruHitachiM32rKline::kernel_write_region(const QByteArray &encrypted)
{
    QByteArray output;
    QByteArray received;
    uint32_t total = tcu_write_end - tcu_write_start;

    if ((uint32_t)encrypted.length() != total)
    {
        emit LOG_E("Internal error: payload length 0x" + QString::number(encrypted.length(), 16), true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Writing TCU flash 0x008000-0x05FFFF...", true, true);
    set_progressbar_value(0);

    for (uint32_t addr = tcu_write_start; addr < tcu_write_end; addr += tcu_block_size)
    {
        if (kill_process)
        {
            emit LOG_E("Write interrupted by user", true, true);
            return STATUS_ERROR;
        }

        output.clear();
        output.append((uint8_t)0x36);
        output.append((uint8_t)((addr >> 16) & 0xFF));
        output.append((uint8_t)((addr >> 8) & 0xFF));
        output.append((uint8_t)(addr & 0xFF));
        output.append(encrypted.mid(addr - tcu_write_start, tcu_block_size));

        received = send_kernel_request(output, tcu_block_timeout);

        if (received.length() < 5 || (uint8_t)received.at(4) != 0x76)
        {
            emit LOG_E("Write failed at 0x" + QString("%1").arg(addr, 6, 16, QLatin1Char('0')) + ": " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }

        set_progressbar_value((int)((uint64_t)(addr + tcu_block_size - tcu_write_start) * 100 / total));
    }

    emit LOG_I("All blocks written", true, true);
    return STATUS_SUCCESS;
}

/*
 * Kernel 31 01 02: flash sum check
 *
 * @return success
 */
int FlashTcuSubaruHitachiM32rKline::kernel_verify()
{
    QByteArray output;
    QByteArray received;

    emit LOG_I("Verifying flash checksum...", true, true);
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x02);
    received = send_kernel_request(output, tcu_verify_timeout);

    if (received.length() > 6 && (uint8_t)received.at(4) == 0x71 && (uint8_t)received.at(5) == 0x01 && (uint8_t)received.at(6) == 0x02)
    {
        emit LOG_I("Flash checksum verified by TCU", true, true);
        return STATUS_SUCCESS;
    }

    emit LOG_E("Flash checksum verify failed: " + parse_message_to_hex(received), true, true);
    return STATUS_ERROR;
}

/*
 * Erase + write + verify, with full restarts on failure. Kernel must already
 * have a valid range set.
 *
 * @return success
 */
int FlashTcuSubaruHitachiM32rKline::kernel_flash(const QByteArray &encrypted, bool erased)
{
    flash_in_progress = true;

    for (int attempt = 1; attempt <= tcu_write_attempts && !kill_process; attempt++)
    {
        if (attempt > 1)
        {
            emit LOG_W("Restarting from range/erase, attempt " + QString::number(attempt) + " of " + QString::number(tcu_write_attempts), true, true);
            delay(1000);
            // Car log 2026-11-19 (tcuflash, same protocol): a block reply was lost at 0x009800 and the loader did not
            // answer 3 range requests (~6.5 s); a recovery run ~5 min later (ignition left ON) was answered at once at
            // 15625 baud and completed. Keep asking for ~30 s so the same restart can continue without a separate recovery run.
            if (kernel_set_range(tcu_restart_range_tries) != STATUS_SUCCESS)
                break;
        }

        if (kernel_erase() != STATUS_SUCCESS)
        {
            // An erase reply can be lost after the erase has run: treat the flash as erased from here on
            erased = true;
            continue;
        }
        erased = true;

        if (kernel_write_region(encrypted) != STATUS_SUCCESS)
            continue;

        if (kernel_verify() != STATUS_SUCCESS)
            continue;

        flash_in_progress = false;
        emit LOG_I("TCU write complete and verified", true, true);
        QMessageBox::information(this, tr("TCU Operation"),
                                 "TCU write complete and verified by the TCU.\n\n"
                                 "Turn ignition OFF, wait at least 10 seconds, then turn it ON.\n"
                                 "Then clear ECU trouble codes if needed, Clear Memory 2 and do the learning drive.");
        return STATUS_SUCCESS;
    }

    flash_in_progress = false;
    show_recovery_instructions("TCU write failed", erased);
    return STATUS_ERROR;
}

/*
 * Normal write: app session, checks, programming session, kernel flash
 *
 * @return success
 */
int FlashTcuSubaruHitachiM32rKline::write_tcu()
{
    // All user dialogs come before the security unlock, so that unlock -> 10 85 02 runs without pauses
    if (check_battery() != STATUS_SUCCESS)
        return STATUS_ERROR;

    if (confirm_write(false) != STATUS_SUCCESS)
        return STATUS_ERROR;

    emit LOG_I("Connecting to Subaru TCU Hitachi K-Line, please wait...", true, true);
    if (connect_bootloader() != STATUS_SUCCESS)
        return STATUS_ERROR;

    QByteArray file_id = ecuCalDef->FullRomData.mid(0x802A, 5);
    if (tcu_rom_id != file_id)
    {
        emit LOG_E("ROM ID mismatch: TCU " + parse_message_to_hex(tcu_rom_id) + "| file " + parse_message_to_hex(file_id), true, true);
        return STATUS_ERROR;
    }

    if (check_tcu_conditions() != STATUS_SUCCESS)
        return STATUS_ERROR;

    if (enter_programming_session() != STATUS_SUCCESS)
    {
        // A lost 50 85 reply would leave the TCU in its kernel: an ignition cycle always restores the program here
        show_recovery_instructions("Programming session not confirmed", false);
        return STATUS_ERROR;
    }

    delay(300);
    serial->change_port_speed(kernel_baudrate);
    delay(200);

    if (kernel_set_range() != STATUS_SUCCESS)
    {
        show_recovery_instructions("TCU flash kernel did not accept the range", false);
        return STATUS_ERROR;
    }

    if (test_write)
    {
        emit LOG_I("Dry run complete: unlock, programming session and kernel range OK, nothing erased", true, true);
        QMessageBox::information(this, tr("TCU dry run"),
                                 "Dry run OK: security unlock, programming session and flash kernel all answered.\n"
                                 "Nothing was erased or written.\n\n"
                                 "Turn ignition OFF, wait at least 10 seconds, then turn it ON to restart the TCU program.");
        return STATUS_SUCCESS;
    }

    QByteArray encrypted = encrypt_payload(ecuCalDef->FullRomData.mid(tcu_write_start, tcu_write_end - tcu_write_start));
    return kernel_flash(encrypted, false);
}

/*
 * Recovery write: TCU program not running (boot listener) or flash kernel
 * still resident after a failed write. Talks to it directly at kernel baud.
 *
 * @return success
 */
int FlashTcuSubaruHitachiM32rKline::write_tcu_recovery()
{
    emit LOG_I("Recovery mode: contacting TCU boot listener / flash kernel at " + kernel_baudrate + " baud", true, true);

    if (check_battery() != STATUS_SUCCESS)
        return STATUS_ERROR;

    if (confirm_write(true) != STATUS_SUCCESS)
        return STATUS_ERROR;

    serial->change_port_speed(kernel_baudrate);
    delay(200);

    // First 34 starts the kernel when the boot listener answers; repeat it to the kernel itself
    if (kernel_set_range() != STATUS_SUCCESS)
    {
        emit LOG_E("No answer from boot listener or kernel. If the TCU program runs normally, use the normal write protocol.", true, true);
        return STATUS_ERROR;
    }
    delay(300);
    if (kernel_set_range() != STATUS_SUCCESS)
    {
        // Flash state unknown in recovery: assume erased
        show_recovery_instructions("TCU flash kernel did not accept the range", true);
        return STATUS_ERROR;
    }

    if (test_write)
    {
        emit LOG_I("Recovery dry run complete: kernel answered, nothing erased", true, true);
        QMessageBox::information(this, tr("TCU dry run"),
                                 "Recovery dry run OK: the flash kernel answered. Nothing was erased or written by this run.\n\n"
                                 "Turn ignition OFF, wait at least 10 seconds, then turn it ON.");
        return STATUS_SUCCESS;
    }

    QByteArray encrypted = encrypt_payload(ecuCalDef->FullRomData.mid(tcu_write_start, tcu_write_end - tcu_write_start));
    return kernel_flash(encrypted, true);
}

void FlashTcuSubaruHitachiM32rKline::show_recovery_instructions(QString reason, bool flash_erased)
{
    QString text;

    if (!flash_erased)
        text = reason + ".\n\nNothing was erased. Turn ignition OFF, wait at least 10 seconds, then turn it ON: "
                        "the TCU program starts normally.";
    else
        text = reason + ".\n\nThe TCU flash was erased, so the transmission program will not run until a write succeeds. "
                        "Do not drive the car. Keep the battery charger connected.\n\n"
                        "1. Keep ignition ON and run the write again with protocol 'sub_tcu_hitachi_m32r_kline_recovery' "
                        "(talks to the still running flash kernel).\n"
                        "2. If it does not answer: ignition OFF 10 s, ON, and run the recovery write again "
                        "(the TCU boot listener starts because the program checksum is invalid).\n"
                        "The TCU boot block is never erased, so the recovery write is always possible.";

    emit LOG_E(reason, true, true);
    QMessageBox::critical(this, tr("TCU Operation"), text);
}

/*
 * SSM A8 single read of several addresses (PP = 0x00)
 *
 * @return valid response frame or empty
 */
QByteArray FlashTcuSubaruHitachiM32rKline::send_sid_a8_read(QList<uint32_t> addresses)
{
    QByteArray output;

    output.append((uint8_t)0xA8);
    output.append((uint8_t)0x00);
    for (int i = 0; i < addresses.length(); i++)
    {
        output.append((uint8_t)((addresses.at(i) >> 16) & 0xFF));
        output.append((uint8_t)((addresses.at(i) >> 8) & 0xFF));
        output.append((uint8_t)(addresses.at(i) & 0xFF));
    }
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);

    return read_ssm_response(serial_read_timeout);
}

/*
 * Send one request to the flash kernel and wait for its reply
 *
 * @return valid response frame or empty
 */
QByteArray FlashTcuSubaruHitachiM32rKline::send_kernel_request(QByteArray payload, int timeout)
{
    QByteArray output = add_ssm_header(payload, tester_id, target_id, false);

    // Drop late replies from an earlier request so they cannot be taken as this reply
    serial->clear_rx_buffer();
    serial->write_serial_data_echo_check(output);

    return read_ssm_response(timeout);
}

/*
 * Collect bytes until a complete, checksum-valid 80 F0 18 frame arrives
 *
 * @return frame or empty on timeout / bad checksum
 */
QByteArray FlashTcuSubaruHitachiM32rKline::read_ssm_response(int timeout)
{
    QByteArray received;
    QByteArray header = QByteArray::fromHex("80f018");
    QElapsedTimer timer;

    timer.start();
    while (timer.elapsed() < timeout && !kill_process)
    {
        received.append(serial->read_serial_data(serial_read_short_timeout));

        int start = received.indexOf(header);
        if (start < 0 || received.length() < start + 4)
            continue;

        int frame_length = (uint8_t)received.at(start + 3) + 5;
        if (received.length() < start + frame_length)
            continue;

        QByteArray frame = received.mid(start, frame_length);
        if (is_valid_response(frame))
            return frame;

        emit LOG_E("Response checksum error: " + parse_message_to_hex(frame), true, true);
        return QByteArray();
    }

    return QByteArray();
}

bool FlashTcuSubaruHitachiM32rKline::is_valid_response(const QByteArray &frame)
{
    if (frame.length() < 5)
        return false;
    if ((uint8_t)frame.at(0) != 0x80 || (uint8_t)frame.at(1) != 0xF0 || (uint8_t)frame.at(2) != 0x18)
        return false;
    if (frame.length() != (uint8_t)frame.at(3) + 5)
        return false;

    return calculate_checksum(frame.left(frame.length() - 1), false) == (uint8_t)frame.at(frame.length() - 1);
}

/*
 * Encrypt write payload for the TCU flash kernel: 4 rounds per 32-bit block
 * with key 1336 6640 B291 3E27 (the kernel decrypts with the reverse order,
 * ROM 0x5AAC) and the standard Subaru index table, then swap halves.
 *
 * @return encrypted data
 */
QByteArray FlashTcuSubaruHitachiM32rKline::encrypt_payload(QByteArray buf)
{
    QByteArray encrypted;

    const uint16_t keytogenerateindex[]={
        0x1336, 0x6640, 0xB291, 0x3E27
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    uint32_t data, index;
    uint16_t wordtogenerateindex, wordtobeencrypted, encryptionkey;
    int len = buf.length() & ~3;

    for (int i = 0; i < len; i += 4)
    {
        data = ((uint32_t)(uint8_t)buf.at(i) << 24) | ((uint32_t)(uint8_t)buf.at(i + 1) << 16) |
               ((uint32_t)(uint8_t)buf.at(i + 2) << 8) | (uint32_t)(uint8_t)buf.at(i + 3);

        for (int ki = 0; ki < 4; ki++)
        {
            wordtogenerateindex = (uint16_t)(data & 0xFFFF);
            wordtobeencrypted = (uint16_t)(data >> 16);
            index = wordtogenerateindex ^ keytogenerateindex[ki];
            index += index << 16;
            encryptionkey = 0;

            for (int n = 0; n < 4; n++)
                encryptionkey += indextransformation[(index >> (n * 4)) & 0x1F] << (n * 4);

            encryptionkey = (uint16_t)((encryptionkey >> 3) | (encryptionkey << 13));
            data = (uint32_t)(encryptionkey ^ wordtobeencrypted) | ((uint32_t)wordtogenerateindex << 16);
        }

        data = (data >> 16) | (data << 16);

        encrypted.append((char)((data >> 24) & 0xFF));
        encrypted.append((char)((data >> 16) & 0xFF));
        encrypted.append((char)((data >> 8) & 0xFF));
        encrypted.append((char)(data & 0xFF));
    }

    return encrypted;
}

/*
 * Generate denso kline seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashTcuSubaruHitachiM32rKline::generate_kline_seed_key(QByteArray requested_seed)
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

    key = calculate_seed_key(requested_seed, keytogenerateindex, indextransformation);

    return key;
}

QByteArray FlashTcuSubaruHitachiM32rKline::send_sid_b0_block_write(uint32_t dataaddr, uint32_t datalen)
{
    QByteArray output;
    QByteArray received;

    output.append((uint8_t)0xb0);
    output.append((uint8_t)(dataaddr >> 16) & 0xFF);
    output.append((uint8_t)(dataaddr >> 8) & 0xFF);
    output.append((uint8_t)dataaddr & 0xFF);
    output.append((uint8_t)0x80);
    output.append((uint8_t)0x18);
    output.append((uint8_t)0xf0);
    output.append((uint8_t)((datalen + 4) & 0xFF));

    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_I("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(100);
    received = serial->read_serial_data(receive_timeout);
    //received = QByteArray::fromHex("80f018f8e0");
    delay(100);
    emit LOG_I("Recd: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * SSM Byte Read (K-Line)
 *
 * @return received response
 */
QByteArray FlashTcuSubaruHitachiM32rKline::send_sid_b8_byte_read(uint32_t dataaddr)
{
    QByteArray output;
    QByteArray received;

    output.append((uint8_t)0xb8);
    output.append((uint8_t)(dataaddr >> 16) & 0xFF);
    output.append((uint8_t)(dataaddr >> 8) & 0xFF);
    output.append((uint8_t)dataaddr & 0xFF);

    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    emit LOG_I("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(100);
    received = serial->read_serial_data(receive_timeout);
    //received = QByteArray::fromHex("80f018f8e0");
    delay(100);
    emit LOG_I("Recd: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * SSM Block Read (K-Line)
 *
 * @return received response
 */
QByteArray FlashTcuSubaruHitachiM32rKline::send_sid_a0_block_read(uint32_t dataaddr, uint32_t datalen)
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
    emit LOG_I("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(100);
    received = serial->read_serial_data(receive_timeout);
    //received = QByteArray::fromHex("80f01861e06000f000a04115ec6000f000a04115f06000f000a04115f46000f000a04115f86000f000a04115fc1fcef0002e7ff000fe00087e6000f000fe00022684ad806060072004fe0003e07e02f0002eef1fce6200f00081c200ff600401407d0e1081ab");
    delay(100);
    emit LOG_I("Recd: " + parse_message_to_hex(received), true, true);

    return received;
}


/*
 * Generate tcu hitachi can seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashTcuSubaruHitachiM32rKline::generate_can_seed_key(QByteArray requested_seed)
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

    key = calculate_seed_key(requested_seed, keytogenerateindex, indextransformation);

    return key;
}

/*
 * Calculate subaru tcu hitachi seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashTcuSubaruHitachiM32rKline::calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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

    //emit LOG_I("Send: " + parse_message_to_hex(output), true, true);
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


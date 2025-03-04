#include "flash_ecu_subaru_denso_sh705x_kline.h"

FlashEcuSubaruDensoSH705xKline::FlashEcuSubaruDensoSH705xKline(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
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

void FlashEcuSubaruDensoSH705xKline::run()
{
    this->show();

    int result = STATUS_ERROR;
    set_progressbar_value(0);

    //result = init_flash_denso_kline_04(ecuCalDef, cmd_type);

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
    serial->reset_connection();
    serial->set_add_iso14230_header(false);
    serial->set_is_iso14230_connection(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    serial->set_serial_port_baudrate("4800");
    tester_id = 0xF0;
    target_id = 0x10;
    // Open serial port
    serial->open_serial_port();

    int ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                   tr("Turn ignition ON and press OK to start initializing connection to ECU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);

    switch (ret)
    {
        case QMessageBox::Ok:
            emit LOG_I("Connecting to Subaru 04 32-bit K-Line bootloader, please wait...", true, true);
            result = connect_bootloader();

            if (result == STATUS_SUCCESS && !kernel_alive)
            {
                emit external_logger("Preparing, please wait...");
                emit LOG_I("Initializing Subaru 04 32-bit K-Line kernel upload, please wait...", true, true);
                result = upload_kernel(kernel, ecuCalDef->KernelStartAddr.toUInt(&ok, 16));
            }
            if (result == STATUS_SUCCESS)
            {
                if (cmd_type == "read")
                {
                    emit external_logger("Reading ROM, please wait...");
                    emit LOG_I("Reading ROM from Subaru 04 32-bit using K-Line", true, true);
                    result = read_mem(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                }
                else if (cmd_type == "test_write" || cmd_type == "write")
                {
                    emit external_logger("Writing ROM, please wait...");
                    emit LOG_I("Writing ROM to Subaru 04 32-bit using K-Line", true, true);
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

FlashEcuSubaruDensoSH705xKline::~FlashEcuSubaruDensoSH705xKline()
{
    delete ui;
}

void FlashEcuSubaruDensoSH705xKline::closeEvent(QCloseEvent *bar)
{
    kill_process = true;
}

/*
 * Connect to Subaru Denso K-Line bootloader 32bit ECUs
 *
 * @return success
 */
int FlashEcuSubaruDensoSH705xKline::connect_bootloader()
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
            
        }
        else
        {
            received.remove(0, 5);
            received.remove(received.length() - 1, 1);
            emit LOG_I("Kernel ID: " + received, true, true);
            
            kernel_alive = true;
            return STATUS_SUCCESS;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
    }

    emit LOG_I("No response from kernel, initialising ECU...", true, true);

    serial->change_port_speed("4800");
    delay(100);

    emit LOG_I("Requesting ECU ID", true, true);
    received = send_sid_bf_ssm_init();
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
    emit LOG_I("ECU ID: " + ecuid, true, true);
    if (cmd_type == "read")
        ecuCalDef->RomId = ecuid + "_";

    emit LOG_I("Requesting to start communication", true, true);
    received = send_sid_81_start_communication();
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
    received = send_sid_83_request_timings();
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
    received = send_sid_27_request_seed();
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

    if (flash_method.endsWith("_ecutek"))
        seed_key = generate_ecutek_seed_key(seed);
    else
        seed_key = generate_seed_key(seed);

    msg = parse_message_to_hex(seed_key);
    emit LOG_I("Calculated seed key: " + msg, true, true);

    emit LOG_I("Sending seed key to ECU", true, true);
    received = send_sid_27_send_seed_key(seed_key);
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

    emit LOG_I("Set session mode", true, true);
    received = send_sid_10_start_diagnostic();
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

    return STATUS_SUCCESS;
}

/*
 * Upload kernel to Subaru Denso K-Line 32bit ECUs
 *
 * @return success
 */
int FlashEcuSubaruDensoSH705xKline::upload_kernel(QString kernel, uint32_t addr)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray payload;
    QByteArray received;
    QByteArray msg;
    QByteArray pl_encr;
    uint32_t pl_len = 0;
    QByteArray cks_bypass;

    QString mcu_name;

    emit LOG_D("Start address to upload kernel: 0x" + QString::number(addr, 16), true, true);

    if (!serial->is_serial_port_open())
    {
        emit LOG_E("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    // Change port speed to upload kernel
    if (serial->change_port_speed("15625"))
        return STATUS_ERROR;

    // Check kernel file
    if (!file.open(QIODevice::ReadOnly ))
    {
        emit LOG_E("Unable to open kernel file for reading", true, true);
        return STATUS_ERROR;
    }

    pl_encr = file.readAll();

    pl_encr.append((uint8_t)0x00);
    pl_encr.append((uint8_t)0x00);

    pl_len = pl_encr.length();
    pl_len = (pl_len + 3) & ~3;
    while((uint32_t)pl_encr.length() < pl_len)
        pl_encr.append((uint8_t)0x00);
    pl_encr.remove(pl_encr.length()-2, 2);

    uint16_t chk_sum = 0;
    for (int i = 0; i < pl_encr.length(); i+=4)
        chk_sum += (((uint8_t)pl_encr.at(i) << 24) & 0xFF000000) | (((uint8_t)pl_encr.at(i + 1) << 16) & 0xFF0000) | (((uint8_t)pl_encr.at(i + 2) << 8) & 0xFF00) | (((uint8_t)pl_encr.at(i + 3)) & 0xFF);

    chk_sum = 0x5aa5 - chk_sum;
    pl_encr.append((uint8_t)((chk_sum >> 8) & 0xff));
    pl_encr.append((uint8_t)(chk_sum & 0xff));
    pl_encr = encrypt_payload(pl_encr, pl_encr.length());

    emit LOG_I("Requesting kernel upload", true, true);
    received = send_sid_34_request_upload(addr, pl_encr.length());

    if (received.length() > 4)
    {
        if ((uint8_t)received.at(4) != 0x74)
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
    emit LOG_I("Kernel upload request ok", true, true);
    
    emit LOG_I("Transfer kernel data", true, true);
    received = send_sid_36_transferdata(addr, pl_encr, pl_encr.length());
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(4) != 0x76)
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
    emit LOG_I("Kernel uploaded", true, true);

    emit LOG_I("Jump to kernel", true, true);
    output.clear();
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x01);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_extra_long_timeout);
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(4) != 0x71)
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
    emit LOG_I("Kernel started, initializing...", true, true);
    

    delay(100);
    serial->change_port_speed("62500");

    emit LOG_I("Requesting kernel ID", true, true);
    received.clear();
    received = request_kernel_id();
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != (SUB_KERNEL_ID | 0x40))
        {
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(8, received.length()-1)), true, true);
            
            return STATUS_ERROR;
        }
        else
        {
            received.remove(0, 5);
            received.remove(received.length() - 1, 1);
            emit LOG_I("Kernel ID: " + received, true, true);
            
            kernel_alive = true;
            return STATUS_SUCCESS;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }
}

/*
 * Read memory from Subaru Denso K-Line 32bit ECUs, nisprog kernel
 *
 * @return success
 */
int FlashEcuSubaruDensoSH705xKline::read_mem(uint32_t start_addr, uint32_t length)
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
    uint32_t pagesize = 0x0400;

    uint32_t skip_start = start_addr & (pagesize - 1); //if unaligned, we'll be receiving this many extra bytes
    uint32_t addr = start_addr - skip_start;
    uint32_t willget = (skip_start + length + pagesize - 1) & ~(pagesize - 1);
    uint32_t len_done = 0;
    uint8_t chksum = 0;

    emit LOG_I("Start reading ROM, please wait..." + received, true, true);

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

        output.clear();
        output.append((uint8_t)((SUB_KERNEL_START_COMM >> 8) & 0xFF));
        output.append((uint8_t)(SUB_KERNEL_START_COMM & 0xFF));
        output.append((uint8_t)((datalen + 1) >> 8) & 0xFF);
        output.append((uint8_t)(datalen + 1) & 0xFF);
        output.append((uint8_t)SUB_KERNEL_READ_AREA);
        output.append((uint8_t)0x00 & 0xFF);
        output.append((uint8_t)(addr >> 16) & 0xFF);
        output.append((uint8_t)(addr >> 8) & 0xFF);
        output.append((uint8_t)addr & 0xFF);
        output.append((uint8_t)(pagesize >> 8) & 0xFF);
        output.append((uint8_t)pagesize & 0xFF);
        chksum = calculate_checksum(output, false);
        output.append((uint8_t)chksum & 0xFF);
        serial->write_serial_data_echo_check(output);
        received = serial->read_serial_data(serial_read_extra_long_timeout);
        if (received.length() > 5)
        {
            if ((uint8_t)received.at(0) == ((SUB_KERNEL_START_COMM >> 8) & 0xFF) && (uint8_t)received.at(1) == (SUB_KERNEL_START_COMM & 0xFF) && (uint8_t)received.at(4) == (SUB_KERNEL_READ_AREA | 0x40))
            {
                received.remove(0, 5);
                received.remove(received.length() - 1, 1);
                mapdata.append(received);
            }
            else
            {
                emit LOG_E("Wrong response from ECU", true, true);
                
                return STATUS_ERROR;
            }
        }
        else
        {
            emit LOG_E("No valid response from ECU", true, true);
            
            return STATUS_ERROR;
        }

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
        msg = QString("Kernel read addr: 0x%1 length: 0x%2, %3 B/s %4 s").arg(start_address).arg(block_len).arg(curspeed, 6, 10, QLatin1Char(' ')).arg(tleft, 6, 10, QLatin1Char(' ')).toUtf8();
        emit LOG_I(msg, true, true);

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

    emit LOG_I("ROM read ready", true, true);

    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*
 * Write memory to Subaru Denso K-Line 32bit ECUs, nisprog kernel
 *
 * @return success
 */
int FlashEcuSubaruDensoSH705xKline::write_mem(bool test_write)
{
    QByteArray filedata;

    filedata = ecuCalDef->FullRomData;

    QScopedArrayPointer<uint8_t> data_array(new uint8_t[filedata.length()]);

    int block_modified[16] = {0};

    unsigned bcnt = 0;
    unsigned blockno;

    set_progressbar_value(0);

    for (int i = 0; i < filedata.length(); i++)
    {
        data_array[i] = filedata.at(i);
    }

    emit LOG_I("--- Comparing ECU flash memory pages to image file ---", true, true);
    emit LOG_I("blk\tstart\tlen\tecu crc\timg crc\tsame?", true, true);

    if (get_changed_blocks(&data_array[0], block_modified))
    {
        emit LOG_E("Error in ROM compare", true, true);
        return STATUS_ERROR;
    }

    bcnt = 0;
    emit LOG_I("Different blocks : ", true, false);
    for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++) {
        if (block_modified[blockno]) {
            emit LOG_I(QString::number(blockno) + ", ", false, false);
            bcnt += 1;
        }
    }
    emit LOG_I(" (total: " + QString::number(bcnt) + ")", false, true);

    if (bcnt)
    {
        flashbytescount = 0;
        flashbytesindex = 0;

        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++)
        {
            if (block_modified[blockno])
            {
                flashbytescount += flashdevices[mcu_type_index].fblocks[blockno].len;
            }
        }

        emit LOG_I("--- Start writing ROM file to ECU flash memory ---", true, true);
        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++)
        {
            if (block_modified[blockno])
            {
                if (reflash_block(&data_array[flashdevices[mcu_type_index].fblocks->start], &flashdevices[mcu_type_index], blockno, test_write))
                {
                    emit LOG_I("Block " + QString::number(blockno) + " reflash failed.", true, true);
                    return STATUS_ERROR;
                }
                else
                {
                    flashbytesindex += flashdevices[mcu_type_index].fblocks[blockno].len;
                    emit LOG_I("Block " + QString::number(blockno) + " reflash complete.", true, true);
                }
            }
        }
        set_progressbar_value(100);

        emit LOG_I("--- Comparing ECU flash memory pages to image file after reflash ---", true, true);
        emit LOG_I("blk\tstart\tlen\tecu crc\timg crc\tsame?", true, true);

        if (get_changed_blocks(&data_array[0], block_modified))
        {
            LOG_E("Error in ROM compare", true, true);
            return STATUS_ERROR;
        }

        bcnt = 0;
        emit LOG_I("Different blocks : ", true, false);
        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++) {
            if (block_modified[blockno])
            {
                emit LOG_I(QString::number(blockno) + ", ", false, false);
                bcnt += 1;
            }
        }
        emit LOG_I(" (total: " + QString::number(bcnt) + ")", false, true);
        if (!test_write)
        {
            if (bcnt)
            {
                emit LOG_E("*** ERROR IN FLASH PROCESS ***", true, true);
                emit LOG_E("Don't power off your ECU, kernel is still running and you can try flashing again!", true, true);
            }
        }
        else
            emit LOG_I("*** Test write PASS, it's ok to perform actual write! ***", true, true);
    }
    else
    {
        emit LOG_I("*** Compare results no difference between ROM and ECU data, no flashing needed! ***", true, true);
    }

    return STATUS_SUCCESS;
}

/*
 * Compare ROM 32bit K-Line ECUs, nisprog kernel
 *
 * @return
 */
int FlashEcuSubaruDensoSH705xKline::get_changed_blocks(const uint8_t *src, int *modified)
{
    unsigned blockno;
    QByteArray msg;

    for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++) {
        if (kill_process)
            return STATUS_ERROR;

        uint32_t bs, blen;
        bs = flashdevices[mcu_type_index].fblocks[blockno].start;
        blen = flashdevices[mcu_type_index].fblocks[blockno].len;

        QString block_no = QString("%1").arg((uint8_t)blockno,2,10,QLatin1Char('0')).toUpper();
        QString block_start = QString("%1").arg((uint32_t)bs,8,16,QLatin1Char('0')).toUpper();
        QString block_length = QString("%1").arg((uint32_t)blen,8,16,QLatin1Char('0')).toUpper();

        msg = QString("FB" + block_no + "\t0x" + block_start + "\t0x" + block_length).toUtf8();
        emit LOG_I(msg, true, false);
        // do CRC comparison with ECU //
        if (check_romcrc(&src[bs], bs, blen, &modified[blockno])) {
            return -1;
        }
        delay(5);
    }
    return 0;
}

/*
 * ROM CRC 32bit K-Line ECUs, nisprog kernel
 *
 * @return
 */
int FlashEcuSubaruDensoSH705xKline::check_romcrc(const uint8_t *src, uint32_t addr, uint32_t len, int *modified)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint32_t imgcrc32 = 0;
    uint32_t ecucrc32 = 0;
    uint32_t pagesize = len; // Test 32-bit CRC with block size
    uint32_t datalen = 8;
    uint8_t chksum = 0;

    // Test 32-bit CRC with block size
    output.clear();
    output.append((uint8_t)((SUB_KERNEL_START_COMM >> 8) & 0xFF));
    output.append((uint8_t)(SUB_KERNEL_START_COMM & 0xFF));
    output.append((uint8_t)((datalen + 1) >> 8) & 0xFF);
    output.append((uint8_t)(datalen + 1) & 0xFF);
    output.append((uint8_t)SUB_KERNEL_CRC);
    output.append((uint8_t)(addr >> 24) & 0xFF);
    output.append((uint8_t)(addr >> 16) & 0xFF);
    output.append((uint8_t)(addr >> 8) & 0xFF);
    output.append((uint8_t)addr & 0xFF);
    output.append((uint8_t)0x00 & 0xFF);
    output.append((uint8_t)(pagesize >> 16) & 0xFF);
    output.append((uint8_t)(pagesize >> 8) & 0xFF);
    output.append((uint8_t)pagesize & 0xFF);
    chksum = calculate_checksum(output, false);
    output.append((uint8_t)chksum & 0xFF);
    serial->write_serial_data_echo_check(output);
    
    received.clear();
    received = serial->read_serial_data(serial_read_extra_long_timeout);
    
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(0) == ((SUB_KERNEL_START_COMM >> 8) & 0xFF) && (uint8_t)received.at(1) == (SUB_KERNEL_START_COMM & 0xFF) && (uint8_t)received.at(4) == (SUB_KERNEL_CRC | 0x40))
        {
            ecucrc32 = ((uint8_t)received.at(5) << 24) | ((uint8_t)received.at(6) << 16) | ((uint8_t)received.at(7) << 8) | (uint8_t)received.at(8);
        }
        else
        {
            emit LOG_E("", false, true);
            emit LOG_E("Wrong response from ECU", true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("", false, true);
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }

    imgcrc32 = crc32(src, pagesize);
    msg.clear();
    msg.append(QString("ROM CRC: 0x%1 IMG CRC: 0x%2").arg(ecucrc32,8,16,QLatin1Char('0')).arg(imgcrc32,8,16,QLatin1Char('0')).toUtf8());
    emit LOG_D(msg, true, true);

    QString ecu_crc32 = QString("%1").arg((uint32_t)ecucrc32,8,16,QLatin1Char('0')).toUpper();
    QString img_crc32 = QString("%1").arg((uint32_t)imgcrc32,8,16,QLatin1Char('0')).toUpper();
    msg = QString("\t" + ecu_crc32 + "\t" + img_crc32).toUtf8();
    emit LOG_I(msg, false, false);
    if (ecucrc32 != imgcrc32)
    {
        emit LOG_I("\tNO", false, true);
        *modified = 1;
        serial->read_serial_data(serial_read_short_timeout);
        return 0;
    }

    emit LOG_I("\tYES", false, true);
    *modified = 0;
    serial->read_serial_data(serial_read_short_timeout);
    return 0;
}

unsigned int FlashEcuSubaruDensoSH705xKline::crc32(const unsigned char *buf, unsigned int len)
{
    unsigned int crc = 0xFFFFFFFF;

    if (!crc_tab32_init)
        init_crc32_tab();

    if (buf == NULL)
        return 0L;
    while (len--)
        crc = crc_tab32[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);

    return crc ^ 0xFFFFFFFF;
}

void FlashEcuSubaruDensoSH705xKline::init_crc32_tab( void )
{
    uint32_t i, j;
    uint32_t crc, c;

    for (i=0; i<256; i++) {
        crc = 0;
        c = (uint32_t)i;

        for (j=0; j<8; j++) {
            if ( (crc ^ c) & 0x00000001 )
                crc = ( crc >> 1 ) ^ CRC32;
            else
                crc =   crc >> 1;
            c = c >> 1;
        }
        crc_tab32[i] = crc;
    }

    crc_tab32_init = 1;
}

int FlashEcuSubaruDensoSH705xKline::init_flash_write()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    uint32_t datalen = 0;
    uint8_t chksum;

    emit LOG_I("Check max message length", true, false);
    datalen = 0;
    output.clear();
    output.append((uint8_t)((SUB_KERNEL_START_COMM >> 8) & 0xFF));
    output.append((uint8_t)(SUB_KERNEL_START_COMM & 0xFF));
    output.append((uint8_t)((datalen + 1) >> 8) & 0xFF);
    output.append((uint8_t)(datalen + 1) & 0xFF);
    output.append((uint8_t)(SUB_KERNEL_GET_MAX_MSG_SIZE & 0xFF));
    chksum = calculate_checksum(output, false);
    output.append((uint8_t)chksum & 0xFF);
    received = serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_medium_timeout);
    
    if (received.length() > 9)
    {
        if ((uint8_t)received.at(0) == ((SUB_KERNEL_START_COMM >> 8) & 0xFF) && (uint8_t)received.at(1) == (SUB_KERNEL_START_COMM & 0xFF) && (uint8_t)received.at(4) == (SUB_KERNEL_GET_MAX_MSG_SIZE | 0x40))
        {
            flashmessagesize = (uint8_t)received.at(5) << 24 | (uint8_t)received.at(6) << 16 | (uint8_t)received.at(7) << 8 | (uint8_t)received.at(8) << 0;
            msg.clear();
            msg.append(QString(": 0x%1").arg(flashmessagesize,4,16,QLatin1Char('0')).toUtf8());
            emit LOG_I(msg, false, true);
        }
        else
        {
            emit LOG_E("", false, true);
            emit LOG_E("Wrong response from ECU", true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("", false, true);
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }

    emit LOG_I("Check flashblock size", true, false);
    datalen = 0;
    output.clear();
    output.append((uint8_t)((SUB_KERNEL_START_COMM >> 8) & 0xFF));
    output.append((uint8_t)(SUB_KERNEL_START_COMM & 0xFF));
    output.append((uint8_t)((datalen + 1) >> 8) & 0xFF);
    output.append((uint8_t)(datalen + 1) & 0xFF);
    output.append((uint8_t)(SUB_KERNEL_GET_MAX_BLK_SIZE & 0xFF));
    chksum = calculate_checksum(output, false);
    output.append((uint8_t)chksum & 0xFF);
    received = serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_medium_timeout);
    
    if (received.length() > 9)
    {
        if ((uint8_t)received.at(0) == ((SUB_KERNEL_START_COMM >> 8) & 0xFF) && (uint8_t)received.at(1) == (SUB_KERNEL_START_COMM & 0xFF) && (uint8_t)received.at(4) == (SUB_KERNEL_GET_MAX_BLK_SIZE | 0x40))
        {
            flashblocksize = (uint8_t)received.at(5) << 24 | (uint8_t)received.at(6) << 16 | (uint8_t)received.at(7) << 8 | (uint8_t)received.at(8) << 0;
            msg.clear();
            msg.append(QString(": 0x%1").arg(flashblocksize,4,16,QLatin1Char('0')).toUtf8());
            emit LOG_I(msg, false, true);
        }
        else
        {
            emit LOG_E("", false, true);
            emit LOG_E("Wrong response from ECU", true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("", false, true);
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }

    uint8_t SUB_KERNEL_CMD = 0;
    if (test_write)
    {
        SUB_KERNEL_CMD = (uint8_t)(SUB_KERNEL_FLASH_DISABLE & 0xFF);
        emit LOG_I("Test write mode on, no actual flash write is performed", true, true);
    }
    else if (!test_write)
    {
        SUB_KERNEL_CMD = (uint8_t)(SUB_KERNEL_FLASH_ENABLE & 0xFF);
        emit LOG_I("Test write mode off, perform actual flash write", true, true);
    }

    datalen = 0;
    output.clear();
    output.append((uint8_t)((SUB_KERNEL_START_COMM >> 8) & 0xFF));
    output.append((uint8_t)(SUB_KERNEL_START_COMM & 0xFF));
    output.append((uint8_t)((datalen + 1) >> 8) & 0xFF);
    output.append((uint8_t)(datalen + 1) & 0xFF);
    output.append((uint8_t)(SUB_KERNEL_CMD & 0xFF));
    chksum = calculate_checksum(output, false);
    output.append((uint8_t)chksum & 0xFF);
    received = serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_medium_timeout);
    
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(0) == ((SUB_KERNEL_START_COMM >> 8) & 0xFF) && (uint8_t)received.at(1) == (SUB_KERNEL_START_COMM & 0xFF) && (uint8_t)received.at(4) == (SUB_KERNEL_CMD | 0x40))
        {
            emit LOG_E("Flash mode succesfully set", true, true);
        }
        else
        {
            emit LOG_E("Wrong response from ECU", true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }

    flash_write_init = true;

    return STATUS_SUCCESS;
}

/*
 *  Reflash ROM 32bit K-Line ECUs
 *
 *  @return
 */
int FlashEcuSubaruDensoSH705xKline::reflash_block(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write)
{
    uint32_t block_start;
    uint32_t block_len;
    uint32_t datalen = 0;
    uint8_t chksum = 0;

    QByteArray output;
    QByteArray received;
    QByteArray msg;

    if (!flash_write_init)
        if (init_flash_write())
            return STATUS_ERROR;

    if (blockno >= fdt->numblocks)
    {
        emit LOG_E("Block " + QString::number(blockno) + " out of range!", true, true);
        return -1;
    }

    block_start = fdt->fblocks[blockno].start;
    block_len = fdt->fblocks[blockno].len;

    QString start_addr = QString("%1").arg((uint32_t)block_start,8,16,QLatin1Char('0')).toUpper();
    QString length = QString("%1").arg((uint32_t)block_len,8,16,QLatin1Char('0')).toUpper();
    msg = QString("Flash block addr: 0x" + start_addr + " len: 0x" + length).toUtf8();
    emit LOG_I(msg, true, true);

    emit LOG_I("Check flash voltage", true, false);
    datalen = 0;
    output.clear();
    output.append((uint8_t)((SUB_KERNEL_START_COMM >> 8) & 0xFF));
    output.append((uint8_t)(SUB_KERNEL_START_COMM & 0xFF));
    output.append((uint8_t)((datalen + 1) >> 8) & 0xFF);
    output.append((uint8_t)(datalen + 1) & 0xFF);
    output.append((uint8_t)(SUB_KERNEL_PROG_VOLT & 0xFF));
    chksum = calculate_checksum(output, false);
    output.append((uint8_t)chksum & 0xFF);
    received = serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_medium_timeout);
    
    if (received.length() > 9)
    {
        if ((uint8_t)received.at(0) == ((SUB_KERNEL_START_COMM >> 8) & 0xFF) && (uint8_t)received.at(1) == (SUB_KERNEL_START_COMM & 0xFF) && (uint8_t)received.at(4) == (SUB_KERNEL_PROG_VOLT | 0x40))
        {
            float prog_voltage = (((uint8_t)received.at(5) << 8) + (uint8_t)received.at(6)) / 50.0;
            emit LOG_I(": " + QString::number(prog_voltage) + "V", false, true);
        }
        else
        {
            emit LOG_E("", false, true);
            emit LOG_E("Wrong response from ECU", true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("", false, true);
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }


    if (flash_block(newdata, block_start, block_len)) {
        emit LOG_E("Reflash error! Do not panic, do not reset the ECU immediately. The kernel is most likely still running and receiving commands!", true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Flash block ok", true, true);

    return STATUS_SUCCESS;
}

/*
 * Flash block 32bit K-Line ECUs, nisprog kernel
 *
 * @return
 */
int FlashEcuSubaruDensoSH705xKline::flash_block(const uint8_t *src, uint32_t start, uint32_t len)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    uint32_t remain = len;
    uint32_t block_start = start;
    uint32_t block_len = len;
    uint32_t byteindex = flashbytesindex;
    uint32_t datalen = 0;
    uint32_t imgcrc32 = 0;
    uint32_t flashblockstart = start;
    uint16_t blocksize = 0x200;
    uint8_t chksum = 0;

    QElapsedTimer timer;

    unsigned long chrono;
    unsigned curspeed, tleft;

    flashblocksize = 0x1000;

    msg = QString("Flash page erase addr: 0x%1 len: 0x%2").arg(block_start,8,16,QLatin1Char('0')).arg(block_len,8,16,QLatin1Char('0')).toUtf8();
    emit LOG_I(msg, true, true);

    emit LOG_I("Erasing flash page...", true, false);
    datalen = 4;
    output.clear();
    output.append((uint8_t)((SUB_KERNEL_START_COMM >> 8) & 0xFF));
    output.append((uint8_t)(SUB_KERNEL_START_COMM & 0xFF));
    output.append((uint8_t)((datalen + 1) >> 8) & 0xFF);
    output.append((uint8_t)(datalen + 1) & 0xFF);
    output.append((uint8_t)(SUB_KERNEL_BLANK_PAGE & 0xFF));
    output.append((uint8_t)(start >> 24) & 0xFF);
    output.append((uint8_t)(start >> 16) & 0xFF);
    output.append((uint8_t)(start >> 8) & 0xFF);
    output.append((uint8_t)start & 0xFF);
    chksum = calculate_checksum(output, false);
    output.append((uint8_t)chksum & 0xFF);
    received = serial->write_serial_data_echo_check(output);
    
    //delay(500);
    received = serial->read_serial_data(serial_read_extra_long_timeout);
    
    if (received.length() > 4)
    {
        if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != (SUB_KERNEL_BLANK_PAGE | 0x40))
        {
            emit LOG_E("", false, true);
            emit LOG_E("Wrong response from ECU: " + FileActions::parse_nrc_message(received.mid(8, received.length()-1)), true, true);
            
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("", false, true);
        emit LOG_E("No valid response from ECU", true, true);
        
        return STATUS_ERROR;
    }

    emit LOG_I(" erased", false, true);

    msg = QString("Start flash write addr: 0x%1 len: 0x%2").arg(block_start,8,16,QLatin1Char('0')).arg(block_len,8,16,QLatin1Char('0')).toUtf8();
    emit LOG_I(msg, true, true);

    timer.start();
    while (remain) {
        if (kill_process)
            return STATUS_ERROR;

        datalen = blocksize + 4;
        output.clear();
        output.append((uint8_t)((SUB_KERNEL_START_COMM >> 8) & 0xFF));
        output.append((uint8_t)(SUB_KERNEL_START_COMM & 0xFF));
        output.append((uint8_t)((datalen + 1) >> 8) & 0xFF);
        output.append((uint8_t)(datalen + 1) & 0xFF);
        output.append((uint8_t)(SUB_KERNEL_WRITE_FLASH_BUFFER & 0xFF));
        output.append((uint8_t)(start >> 24) & 0xFF);
        output.append((uint8_t)(start >> 16) & 0xFF);
        output.append((uint8_t)(start >> 8) & 0xFF);
        output.append((uint8_t)start & 0xFF);
        for (unsigned int i = start; i < (start + blocksize); i++)
        {
            output.append(src[i]);
        }
        chksum = calculate_checksum(output, false);
        output.append((uint8_t)chksum & 0xFF);
        serial->write_serial_data_echo_check(output);
        //
        //delay(50);
        received = serial->read_serial_data(serial_read_extra_long_timeout);
        if (received.length() > 5)
        {
            if ((uint8_t)received.at(0) == ((SUB_KERNEL_START_COMM >> 8) & 0xFF) && (uint8_t)received.at(1) == (SUB_KERNEL_START_COMM & 0xFF) && (uint8_t)received.at(4) == (SUB_KERNEL_WRITE_FLASH_BUFFER | 0x40))
            {
                emit LOG_D("Data written to flash buffer", true, true);
            }
            else
            {
                emit LOG_E("Wrong response from ECU", true, true);
                
                return STATUS_ERROR;
            }
        }
        else
        {
            emit LOG_E("No valid response from ECU", true, true);
            
            return STATUS_ERROR;
        }

        QString start_address = QString("%1").arg(start,8,16,QLatin1Char('0')).toUpper();
        msg = QString("Write flash buffer: 0x%1 (%2\% - %3 B/s, ~ %4 s remain)").arg(start_address).arg((unsigned) 100 * (len - remain) / len,1,10,QLatin1Char('0')).arg((uint32_t)curspeed,1,10,QLatin1Char('0')).arg(tleft,1,10,QLatin1Char('0')).toUtf8();
        emit LOG_I(msg, true, true);

        remain -= blocksize;
        start += blocksize;
        byteindex += blocksize;
        //src += blocksize;

        chrono = timer.elapsed();
        timer.start();

        if (!chrono) {
            chrono += 1;
        }
        curspeed = blocksize * (1000.0f / chrono);  //avg B/s
        if (!curspeed) {
            curspeed += 1;
        }

        tleft = ((float)flashbytescount - byteindex) / curspeed;  //s
        if (tleft > 9999) {
            tleft = 9999;
        }
        tleft++;

        float pleft = (float)byteindex / (float)flashbytescount * 100.0f;
        set_progressbar_value(pleft);

        if ((flashblockstart + flashblocksize) == start)
        {
            emit LOG_I("Flash buffer write complete... ", true, true);
            imgcrc32 = crc32(&src[flashblockstart], flashblocksize);
            emit LOG_D("Image CRC32: 0x" + QString::number(imgcrc32, 16), true, true);

            uint8_t SUB_KERNEL_CMD = 0;
            if (test_write)
            {
                SUB_KERNEL_CMD = (uint8_t)(SUB_KERNEL_VALIDATE_FLASH_BUFFER & 0xFF);
                emit LOG_I("Validate flash addr: 0x" + QString::number(flashblockstart, 16), true, false);
                emit LOG_I(" len: 0x" + QString::number(flashblocksize, 16), false, false);
                emit LOG_I(" crc32: 0x" + QString::number(imgcrc32, 16), false, true);
            }
            else if (!test_write)
            {
                SUB_KERNEL_CMD = (uint8_t)(SUB_KERNEL_COMMIT_FLASH_BUFFER & 0xFF);
                emit LOG_I("Committ flash addr: 0x" + QString::number(flashblockstart, 16), true, false);
                emit LOG_I(" len: 0x" + QString::number(flashblocksize, 16), false, false);
                emit LOG_I(" crc32: 0x" + QString::number(imgcrc32, 16), false, true);
            }

            datalen = 10;
            output.clear();
            output.append((uint8_t)((SUB_KERNEL_START_COMM >> 8) & 0xFF));
            output.append((uint8_t)(SUB_KERNEL_START_COMM & 0xFF));
            output.append((uint8_t)((datalen + 1) >> 8) & 0xFF);
            output.append((uint8_t)(datalen + 1) & 0xFF);
            output.append((uint8_t)(SUB_KERNEL_CMD & 0xFF));
            output.append((uint8_t)(flashblockstart >> 24) & 0xFF);
            output.append((uint8_t)(flashblockstart >> 16) & 0xFF);
            output.append((uint8_t)(flashblockstart >> 8) & 0xFF);
            output.append((uint8_t)flashblockstart & 0xFF);
            output.append((uint8_t)(flashblocksize >> 8) & 0xFF);
            output.append((uint8_t)flashblocksize & 0xFF);
            output.append((uint8_t)(imgcrc32 >> 24) & 0xFF);
            output.append((uint8_t)(imgcrc32 >> 16) & 0xFF);
            output.append((uint8_t)(imgcrc32 >> 8) & 0xFF);
            output.append((uint8_t)imgcrc32 & 0xFF);
            chksum = calculate_checksum(output, false);
            output.append((uint8_t)chksum & 0xFF);
            received = serial->write_serial_data_echo_check(output);
            
            //delay(200);
            received = serial->read_serial_data(serial_read_extra_long_timeout);
            
            if (received.length() > 5)
            {
                if ((uint8_t)received.at(0) == ((SUB_KERNEL_START_COMM >> 8) & 0xFF) && (uint8_t)received.at(1) == (SUB_KERNEL_START_COMM & 0xFF) && (uint8_t)received.at(4) == (SUB_KERNEL_CMD + 0x40))
                {
                }
                else
                {
                    emit LOG_E("Wrong response from ECU", true, true);
                    
                    return STATUS_ERROR;
                }
            }
            else
            {
                emit LOG_E("No valid response from ECU", true, true);
                
                return STATUS_ERROR;
            }
            flashblockstart += flashblocksize;
        }
    }
    return STATUS_SUCCESS;
}

/*
 * 8bit checksum
 *
 * @return
 */
uint8_t FlashEcuSubaruDensoSH705xKline::cks_add8(QByteArray chksum_data, unsigned len)
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
QByteArray FlashEcuSubaruDensoSH705xKline::send_sid_bf_ssm_init()
{
    QByteArray output;
    QByteArray received;

    output.clear();
    output.append((uint8_t)0xBF);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_timeout);
    

    return received;
}

/*
 * Start diagnostic connection
 *
 * @return received response
 */
QByteArray FlashEcuSubaruDensoSH705xKline::send_sid_81_start_communication()
{
    QByteArray output;
    QByteArray received;

    output.clear();
    output.append((uint8_t)0x81);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_timeout);
    

    return received;
}

/*
 * Start diagnostic connection
 *
 * @return received response
 */
QByteArray FlashEcuSubaruDensoSH705xKline::send_sid_83_request_timings()
{
    QByteArray output;
    QByteArray received;

    output.clear();
    output.append((uint8_t)0x83);
    output.append((uint8_t)0x00);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_timeout);
    

    return received;
}

/*
 * Request seed
 *
 * @return seed (4 bytes)
 */
QByteArray FlashEcuSubaruDensoSH705xKline::send_sid_27_request_seed()
{
    QByteArray output;
    QByteArray received;

    output.clear();
    output.append((uint8_t)0x27);
    output.append((uint8_t)0x01);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_timeout);
    

    return received;
}

/*
 * Send seed key
 *
 * @return received response
 */
QByteArray FlashEcuSubaruDensoSH705xKline::send_sid_27_send_seed_key(QByteArray seed_key)
{
    QByteArray output;
    QByteArray received;

    output.clear();
    output.append((uint8_t)0x27);
    output.append((uint8_t)0x02);
    output.append(seed_key);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_timeout);
    

    return received;
}

/*
 * Request start diagnostic
 *
 * @return received response
 */
QByteArray FlashEcuSubaruDensoSH705xKline::send_sid_10_start_diagnostic()
{
    QByteArray output;
    QByteArray received;

    output.clear();
    output.append((uint8_t)0x10);
    output.append((uint8_t)0x85);
    output.append((uint8_t)0x02);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_timeout);
    

    return received;
}

QByteArray FlashEcuSubaruDensoSH705xKline::send_sid_34_request_upload(uint32_t addr, uint32_t len)
{
    QByteArray output;
    QByteArray received;

    output.clear();
    output.append((uint8_t)0x34);
    output.append((uint8_t)(addr >> 16) & 0xFF);
    output.append((uint8_t)(addr >> 8) & 0xFF);
    output.append((uint8_t)addr & 0xFF);
    output.append((uint8_t)0x04);
    output.append((uint8_t)(len >> 16) & 0xFF);
    output.append((uint8_t)(len >> 8) & 0xFF);
    output.append((uint8_t)len & 0xFF);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    
    received = serial->read_serial_data(serial_read_extra_long_timeout);
    

    return received;
}

/*
 * Transfer data (kernel)
 *
 * @return received response
 */
QByteArray FlashEcuSubaruDensoSH705xKline::send_sid_36_transferdata(uint32_t addr, QByteArray buf, uint32_t len)
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

        blockaddr = addr + blockno * blocksize;
        output.clear();
        output.append(0x36);
        output.append((uint8_t)(blockaddr >> 16) & 0xFF);
        output.append((uint8_t)(blockaddr >> 8) & 0xFF);
        output.append((uint8_t)blockaddr & 0xFF);

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
        received = serial->read_serial_data(serial_read_timeout);
        
        if (received.length() > 4)
        {
            if ((uint8_t)received.at(4) != 0x76)
            {
                emit LOG_E("Write data failed!", true, true);
                
                return received;
            }
        }
        else
        {
            emit LOG_E("Write data failed!", true, true);
            
            return received;
        }
        float pleft = (float)blockno / (float)maxblocks * 100;
        set_progressbar_value(pleft);
    }

    set_progressbar_value(100);

    return received;
}

/*
 * Generate denso kline seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashEcuSubaruDensoSH705xKline::generate_seed_key(QByteArray requested_seed)
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
QByteArray FlashEcuSubaruDensoSH705xKline::generate_ecutek_seed_key(QByteArray requested_seed)
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
QByteArray FlashEcuSubaruDensoSH705xKline::calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
QByteArray FlashEcuSubaruDensoSH705xKline::encrypt_payload(QByteArray buf, uint32_t len)
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

QByteArray FlashEcuSubaruDensoSH705xKline::decrypt_payload(QByteArray buf, uint32_t len)
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

QByteArray FlashEcuSubaruDensoSH705xKline::calculate_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
QByteArray FlashEcuSubaruDensoSH705xKline::request_kernel_id()
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
    
    delay(200);
    received = serial->read_serial_data(serial_read_long_timeout);
    
    kernelid = received;

    request_denso_kernel_id = false;

    return kernelid;
}

/*
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray FlashEcuSubaruDensoSH705xKline::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
{
    uint8_t length = output.length();

    output.insert(0, (uint8_t)0x80);
    output.insert(1, target_id & 0xFF);
    output.insert(2, tester_id & 0xFF);
    output.insert(3, length);

    output.append(calculate_checksum(output, dec_0x100));

    //
    return output;
}

/*
 * Calculate SSM checksum to message
 *
 * @return 8-bit checksum
 */
uint8_t FlashEcuSubaruDensoSH705xKline::calculate_checksum(QByteArray output, bool dec_0x100)
{
    uint8_t checksum = 0;

    for (uint16_t i = 0; i < output.length(); i++)
        checksum += (uint8_t)output.at(i);

    if (dec_0x100)
        checksum = (uint8_t)(0x100 - checksum);

    return checksum;
}

/*
 * Parse QByteArray to readable form
 *
 * @return parsed message
 */
QString FlashEcuSubaruDensoSH705xKline::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

void FlashEcuSubaruDensoSH705xKline::set_progressbar_value(int value)
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

void FlashEcuSubaruDensoSH705xKline::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

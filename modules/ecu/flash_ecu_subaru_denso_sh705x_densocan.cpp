#include <modules/ecu/flash_ecu_subaru_denso_sh705x_densocan.h>

FlashEcuSubaruDensoSH705xDensoCan::FlashEcuSubaruDensoSH705xDensoCan(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
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

void FlashEcuSubaruDensoSH705xDensoCan::run()
{
    this->show();

    int result = STATUS_ERROR;
    set_progressbar_value(0);

    //result = init_flash_denso_can_02();

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
    emit LOG_D("MCU type: " + mcu_name + " and index: " + mcu_type_index, true, true);

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
    serial->set_is_can_connection(true);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(true);
    serial->set_can_speed("500000");
    serial->set_can_source_address(0x000FFFFE);
    serial->set_can_destination_address(0x21);
    // Open serial port
    serial->open_serial_port();

    int ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                   tr("Turn ignition ON and press OK to start initializing connection to ECU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);

    switch (ret)
    {
        case QMessageBox::Ok:
            emit LOG_I("Connecting to Subaru 02+ 32-bit Denso CAN bootloader, please wait...", true, true);
            result = connect_bootloader_subaru_denso_sh705x_densocan();

            if (result == STATUS_SUCCESS && !kernel_alive)
            {
                emit external_logger("Preparing, please wait...");
                emit LOG_I("Initializing Subaru 02+ 32-bit DensoCAN kernel upload, please wait...", true, true);
                result = upload_kernel_subaru_denso_sh705x_densocan(kernel, ecuCalDef->KernelStartAddr.toUInt(&ok, 16));
            }
            if (result == STATUS_SUCCESS)
            {
                if (cmd_type == "read")
                {
                    emit external_logger("Reading ROM, please wait...");
                    emit LOG_I("Reading ROM from Subaru 02+ 32-bit Denso using DensoCAN", true, true);
                    result = read_mem_subaru_denso_sh705x_densocan(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                }
                else if (cmd_type == "test_write" || cmd_type == "write")
                {
                    emit external_logger("Writing ROM, please wait...");
                    emit LOG_I("Writing ROM to Subaru 02+ 32-bit Denso using DensoCAN", true, true);
                    result = write_mem_subaru_denso_sh705x_densocan(test_write);
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

FlashEcuSubaruDensoSH705xDensoCan::~FlashEcuSubaruDensoSH705xDensoCan()
{
    delete ui;
}

void FlashEcuSubaruDensoSH705xDensoCan::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

/*
 * Connect to Subaru Denso CAN bootloader 32bit ECUs
 *
 * @return success
 */
int FlashEcuSubaruDensoSH705xDensoCan::connect_bootloader_subaru_denso_sh705x_densocan()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    if (!serial->is_serial_port_open())
    {
        emit LOG_E("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    emit LOG_I("Checking if kernel is already running...", true, true);

    received.clear();
    received = request_kernel_id();
    emit LOG_I("Kernel ID: " + received, true, true);
    if (received != "")
    {
        kernel_alive = true;
        return STATUS_SUCCESS;
    }
    emit LOG_I("No response from kernel, continue initializing bootloader...", true, true);

    emit LOG_I("Initializing bootloader", true, true);

    uint16_t loopcount = 0;
    //QTime dieTime = QTime::currentTime().addMSecs(2000);
    //while (QTime::currentTime() < dieTime)
    while (loopcount < 1000)
    {
        //QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        output.clear();
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x0F);
        output.append((uint8_t)0xFF);
        output.append((uint8_t)0xFE);
        output.append((uint8_t)((SID_CAN_ENTER_BL >> 8) & 0xFF));
        output.append((uint8_t)(SID_CAN_ENTER_BL & 0xFF));
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);

        serial->write_serial_data_echo_check(output);
        received = serial->read_serial_data(20, 3);
        //delay(5);
        loopcount++;
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    received = serial->read_serial_data(20, 10);
    //emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    emit LOG_I("Connecting to bootloader", true, true);

    output[4] = (uint8_t)SID_CAN_START_COMM;
    output[5] = (uint8_t)(SID_CAN_CHECK_COMM_BL & 0xFF);
    output[6] = (uint8_t)0x00;
    output[7] = (uint8_t)0x00;
    output[8] = (uint8_t)0x00;
    output[9] = (uint8_t)0x00;
    output[10] = (uint8_t)0x00;
    output[11] = (uint8_t)0x00;
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(200);
    received = serial->read_serial_data(20, 10);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    if (received.length()) {
        if ((uint8_t)(received.at(1) & 0xF8) == 0x90)
        {
            emit LOG_I("Connected to bootloader, start kernel upload", true, true);
            return STATUS_SUCCESS;
        }
    }

    return STATUS_ERROR;

}

/*
 * Upload kernel to Subaru Denso CAN 32bit ECUs
 *
 * @return success
 */
int FlashEcuSubaruDensoSH705xDensoCan::upload_kernel_subaru_denso_sh705x_densocan(QString kernel, uint32_t kernel_start_addr)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray payload;
    QByteArray received;
    QByteArray msg;
    QByteArray pl_encr;
    uint32_t file_len = 0;
    //uint32_t pl_len = 0;
    uint32_t start_address = 0;
    uint32_t end_address = 0;
    //uint32_t len = 0;
    QByteArray cks_bypass;
    uint8_t chk_sum = 0;
    int maxblocks = 0;
    int byte_counter = 0;

    QString mcu_name;

    start_address = kernel_start_addr;//flashdevices[mcu_type_index].kblocks->start;
    emit LOG_D("Start address to upload kernel: " + QString::number(start_address, 16), true, true);

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
    //pl_len = file_len + 6;
    pl_encr = file.readAll();
    maxblocks = file_len / 6;
    if((file_len % 6) != 0) maxblocks++;
    end_address = (start_address + (maxblocks * 6)) & 0xFFFFFFFF;

    emit LOG_I("Starting kernel upload, please wait...", true, true);
    // Send kernel address
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    output.append((uint8_t)SID_CAN_START_COMM);
    output.append((uint8_t)(SID_KERNEL_ADDRESS + 0x04));
    output.append((uint8_t)((start_address >> 24) & 0xFF));
    output.append((uint8_t)((start_address >> 16) & 0xFF));
    output.append((uint8_t)((start_address >> 8) & 0xFF));
    output.append((uint8_t)(start_address & 0xFF));
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    received = serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(200);
    received = serial->read_serial_data(20, 10);
    emit LOG_D("Response: " + parse_message_to_hex(output), true, true);

    output[5] = (uint8_t)(0xA8 + 0x06);

    set_progressbar_value(0);

    emit LOG_I("Uploading kernel, please wait...", true, true);

    emit LOG_D("Sending " + QString::number(maxblocks) + "blocks", true, true);
    for(int blockno = 0; blockno < maxblocks; blockno++)
    {
        if (kill_process)
            return STATUS_ERROR;

        //emit LOG_D("Send block: " + QString::number(blockno), true, true);
        for(int j = 0; j < 6; j++)
        {
            if (pl_encr.length() > (byte_counter + j))
            {
                output[6 + j] = pl_encr.at(byte_counter + j);
                chk_sum += (pl_encr.at(byte_counter + j) & 0xFF);
            }
            else
            {
                output[6 + j] = 0;
                chk_sum += (0x00);
            }

            chk_sum = ((chk_sum >> 8) & 0xFF) + (chk_sum & 0xFF);

        }

        byte_counter += 6;
        received = serial->write_serial_data_echo_check(output);
        //emit LOG_D("Sent kernel block to bootloader: " + QString::number(blockno), true, true);

        delay(1);

        float pleft = (float)blockno / (float)maxblocks * 100;
        set_progressbar_value(pleft);

    }

    set_progressbar_value(100);

    emit LOG_D("All kernel blocks sent, checksum: " + QString::number(chk_sum, 16), true, true);

    // send 0xB0 command to check checksum
    output[5] = (uint8_t)(SID_KERNEL_CHECKSUM + 0x04);
    output[6] = (uint8_t)(((end_address + 1) >> 24) & 0xFF);
    output[7] = (uint8_t)(((end_address + 1) >> 16) & 0xFF);
    output[8] = (uint8_t)(((end_address + 1) >> 8) & 0xFF);
    output[9] = (uint8_t)((end_address + 1) & 0xFF);
    output[10] = (uint8_t)0x00;
    output[11] = (uint8_t)0x00;
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Verifying kernel checksum, please wait...", true, true);
    delay(200);
    received = serial->read_serial_data(20, 10);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    emit LOG_I("Kernel uploaded, starting kernel...", true, true);

    // 7A B1 81 FF 80 00 00 00 ??? Jump address
    // send 0xA0 command to jump into kernel
    output[5] = (uint8_t)(SID_KERNEL_JUMP + 0x04);
    output[6] = (uint8_t)((end_address >> 24) & 0xFF);
    output[7] = (uint8_t)((end_address >> 16) & 0xFF);
    output[8] = (uint8_t)((end_address >> 8) & 0xFF);
    output[9] = (uint8_t)(end_address & 0xFF);
    output[10] = (uint8_t)0x00;
    output[11] = (uint8_t)0x00;
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);

    emit LOG_I("Requesting kernel ID", true, true);

    received.clear();
    received = request_kernel_id();
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
int FlashEcuSubaruDensoSH705xDensoCan::read_mem_subaru_denso_sh705x_densocan(uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray pagedata;
    QByteArray mapdata;
    uint32_t cplen = 0;
    uint32_t timeout = 0;

    uint32_t pagesize = 0x1000;

    uint32_t skip_start = start_addr & (pagesize - 1); //if unaligned, we'll be receiving this many extra bytes
    uint32_t addr = start_addr - skip_start;
    uint32_t willget = (skip_start + length + pagesize - 1) & ~(pagesize - 1);
    uint32_t len_done = 0;  //total data written to file

    emit LOG_I("Start reading ROM, please wait..." + received, true, true);

    // send 0xD8 command to kernel to dump the chunk from ROM
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    output.append((uint8_t)SID_CAN_START_COMM);
    output.append((uint8_t)(SID_CAN_DUMP_ROM + 0x06));
    output.append((uint8_t)0x00);
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

        //qDebug() << "Send 0xD8 message to kernel to initiate dump";
        output[6] = (uint8_t)((pagesize >> 24) & 0xFF);
        output[7] = (uint8_t)((pagesize >> 16) & 0xFF);
        output[8] = (uint8_t)((pagesize >> 8) & 0xFF);
        output[9] = (uint8_t)((addr >> 24) & 0xFF);
        output[10] = (uint8_t)((addr >> 16) & 0xFF);
        output[11] = (uint8_t)((addr >> 8) & 0xFF);
        serial->write_serial_data_echo_check(output);
        //emit LOG_D("Sent:" + parse_message_to_hex(output), true, true);
        received = serial->read_serial_data(1, serial_read_timeout);
        //emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

        if (received.length()) {
            if ((uint8_t)received.at(0) != SID_CAN_START_COMM || (uint8_t)received.at(1) != SID_CAN_DUMP_ROM)
            {
                emit LOG_E("Page data request failed!", true, true);
                emit LOG_E("Response: " + parse_message_to_hex(received), true, true);
                return STATUS_ERROR;
            }
        }
        timeout = 0;
        pagedata.clear();
        while ((uint32_t)pagedata.length() < pagesize && timeout < 1000)
        {
            if (kill_process)
                return STATUS_ERROR;
            received = serial->read_serial_data(1, serial_read_timeout);
            if (received.length())
                pagedata.append(received, 8);
            else
                return STATUS_ERROR;
            timeout++;
            //qDebug() << parse_message_to_hex(received);
        }
        if (timeout >= 1000)
        {
            emit LOG_E("Page data timeout!", true, true);
            return STATUS_ERROR;
        }
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

    emit LOG_I("ROM read ready" + received, true, true);
    qDebug() << "ROM read ready";

    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*
 * Write memory to Subaru Denso CAN 32bit ECUs, nisprog kernel
 *
 * @return success
 */
int FlashEcuSubaruDensoSH705xDensoCan::write_mem_subaru_denso_sh705x_densocan(bool test_write)
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

    if (get_changed_blocks_denso_sh705x_densocan(&data_array[0], block_modified))
    {
        emit LOG_E("Error in ROM compare", true, true);
        return STATUS_ERROR;
    }

    bcnt = 0;
    emit LOG_I("Different blocks: ", true, false);
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
                if (reflash_block_denso_sh705x_densocan(&data_array[flashdevices[mcu_type_index].fblocks->start], &flashdevices[mcu_type_index], blockno, test_write))
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

        emit LOG_I("--- Comparing ECU flash memory pages to image file ---", true, true);
        emit LOG_I("blk\tstart\tlen\tecu crc\timg crc\tsame?", true, true);

        if (get_changed_blocks_denso_sh705x_densocan(&data_array[0], block_modified))
        {
            emit LOG_E("Error in ROM compare", true, true);
            return STATUS_ERROR;
        }

        bcnt = 0;
        emit LOG_I("Different blocks: ", true, false);
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
 * Compare ROM 32bit CAN ECUs
 *
 * @return
 */
int FlashEcuSubaruDensoSH705xDensoCan::get_changed_blocks_denso_sh705x_densocan(const uint8_t *src, int *modified)
{
    unsigned blockno;
    QByteArray msg;

    for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++)
    {
        if (kill_process)
            return STATUS_ERROR;

        uint32_t bs, blen;
        bs = flashdevices[mcu_type_index].fblocks[blockno].start;
        blen = flashdevices[mcu_type_index].fblocks[blockno].len;

        QString block_no = QString("%1").arg((uint8_t)blockno,2,10,QLatin1Char('0')).toUpper();
        QString block_start = QString("%1").arg((uint32_t)bs,8,16,QLatin1Char('0')).toUpper();
        QString block_length = QString("%1").arg((uint32_t)blen,8,16,QLatin1Char('0')).toUpper();

        msg = QString("FB" + block_no + "\t0x" + block_start + "\t0x" + block_length).toUtf8();
        //qDebug() << msg;
        emit LOG_I(msg, true, false);
        // do CRC comparison with ECU //
        if (check_romcrc_denso_sh705x_densocan(&src[bs], bs, blen, &modified[blockno])) {
            return -1;
        }
    }
    return 0;
}

/*
 * ROM CRC 32bit CAN ECUs
 *
 * @return
 */
int FlashEcuSubaruDensoSH705xDensoCan::check_romcrc_denso_sh705x_densocan(const uint8_t *src, uint32_t start_addr, uint32_t len, int *modified)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint32_t imgcrc32 = 0;
    uint32_t ecucrc32 = 0;
    uint32_t pagesize = len; // Test 32-bit CRC with block size

    // Test 32-bit CRC with block size
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    output.append((uint8_t)SID_CAN_START_COMM);
    output.append((uint8_t)(SID_CAN_CONF_CKS + 0x06));
    output.append((uint8_t)((start_addr >> 16) & 0xFF));
    output.append((uint8_t)((start_addr >> 8) & 0xFF));
    output.append((uint8_t)(start_addr & 0xFF));
    output.append((uint8_t)((pagesize >> 16) & 0xFF));
    output.append((uint8_t)((pagesize >> 8) & 0xFF));
    output.append((uint8_t)(pagesize & 0xFF));
    qDebug() << "Send: " + parse_message_to_hex(output);
    delay(100);
    serial->write_serial_data_echo_check(output);
    delay(200);
    received.clear();
    received = serial->read_serial_data(10, serial_read_extra_long_timeout);
    qDebug() << "Received: " + parse_message_to_hex(received);
    if (received.length())
    {
        if (received.at(0) == 0x7f)
        {
            emit LOG_E("", false, true);
            emit LOG_E("Failed: Wrong answer from ECU", true, true);
            emit LOG_E("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
        uint8_t len = (uint8_t)received.at(1) & 0x07;
        if (len > 3)
        {
            received.remove(0, 2);
            received.remove(received.length() - 1, 1);
        }
    }
    else
    {
        emit LOG_E("", false, true);
        emit LOG_E("Failed: No answer from ECU", true, true);
        return STATUS_ERROR;
    }

    imgcrc32 = crc32(src, pagesize);
    if (received.length() > 3)
    {
        qDebug() << "Get rom crc from msg";
        ecucrc32 = ((uint8_t)received.at(0) << 24) | ((uint8_t)received.at(1) << 16) | ((uint8_t)received.at(2) << 8) | (uint8_t)received.at(3);
    }
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
        serial->read_serial_data(100, serial_read_short_timeout);
        return 0;
    }

    emit LOG_I("\tYES", false, true);
    *modified = 0;
    serial->read_serial_data(100, serial_read_short_timeout);
    return 0;
}

unsigned int FlashEcuSubaruDensoSH705xDensoCan::crc32(const unsigned char *buf, unsigned int len)
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

void FlashEcuSubaruDensoSH705xDensoCan::init_crc32_tab( void ) {
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

}  /* init_crc32_tab */

/*
 * Reflash ROM 32bit CAN ECUs
 *
 * @return success
 */
int FlashEcuSubaruDensoSH705xDensoCan::reflash_block_denso_sh705x_densocan(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write)
{
    int errval;

    uint32_t block_start;
    uint32_t block_len;

    QByteArray output;
    QByteArray received;
    QByteArray msg;

    if (blockno >= fdt->numblocks)
    {
        emit LOG_I("block " + QString::number(blockno) + " out of range !", true, true);
        return -1;
    }

    block_start = fdt->fblocks[blockno].start;
    block_len = fdt->fblocks[blockno].len;

    QString start_addr = QString("%1").arg((uint32_t)block_start,8,16,QLatin1Char('0')).toUpper();
    QString length = QString("%1").arg((uint32_t)block_len,8,16,QLatin1Char('0')).toUpper();
    msg = QString("Flash block addr: 0x" + start_addr + " len: 0x" + length).toUtf8();
    emit LOG_I(msg, true, true);

    emit LOG_I("Setting/unsetting real write mode", true, true);
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    output.append((uint8_t)SID_CAN_START_COMM);
    output.append((uint8_t)(SID_CAN_FLASH + 0x01));
    if (test_write)
        output.append((uint8_t)SID_CAN_FL_PROTECT);
    else
        output.append((uint8_t)SID_CAN_FL_UNPROTECT);
    output.append((uint8_t)(0x00));
    output.append((uint8_t)(0x00));
    output.append((uint8_t)(0x00));
    output.append((uint8_t)(0x00));
    output.append((uint8_t)(0x00));
    received = serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(200);
    received = serial->read_serial_data(3, serial_read_short_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    if (received.length())
    {
        if((uint8_t)received.at(0) != SID_CAN_START_COMM || ((uint8_t)received.at(1) & 0xF8) != SID_CAN_FLASH)
        {
            emit LOG_E("Initialize of erasing / flashing microcodes failed!", true, true);
            emit LOG_E("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }

    int num_128_byte_blocks = (block_len >> 7) & 0xFFFFFFFF;

    emit LOG_I("Proceeding to erase and flash of block number: " + QString::number(blockno, 16), true, true);
    output[5] = (uint8_t)(SID_CAN_FL_EB + 0x06);
    output[6] = (uint8_t)(blockno & 0xFF);
    output[7] = (uint8_t)((block_start >> 24) & 0xFF);
    output[8] = (uint8_t)((block_start >> 16) & 0xFF);
    output[9] = (uint8_t)((block_start >> 8) & 0xFF);
    output[10] = (uint8_t)((num_128_byte_blocks >> 8) & 0xFF);
    output[11] = (uint8_t)(num_128_byte_blocks & 0xFF);
    received = serial->write_serial_data_echo_check(output);
    LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(500);

    QTime dieTime = QTime::currentTime().addMSecs(serial_read_extra_long_timeout);
    while ((uint32_t)received.length() < 3 && (QTime::currentTime() < dieTime))
    {
        received = serial->read_serial_data(3, serial_read_short_timeout);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        delay(100);
    }

    LOG_D("Response: " + parse_message_to_hex(received), true, true);

    if (received.length())
    {
        if((uint8_t)received.at(0) != SID_CAN_START_COMM || ((uint8_t)received.at(1) & 0xF8) != SID_CAN_FL_EB)
        {
            LOG_E("Not ready for 128byte block writing", true, true);
            LOG_E("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    errval = flash_block_denso_sh705x_densocan(newdata, block_start, block_len);
    if (errval) {
        LOG_E("Reflash error! Do not panic, do not reset the ECU immediately. The kernel is most likely still running and receiving commands!", true, true);
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
}

/*
 * Flash block 32bit CAN ECUs
 *
 * @return success
 */
int FlashEcuSubaruDensoSH705xDensoCan::flash_block_denso_sh705x_densocan(const uint8_t *src, uint32_t start, uint32_t len)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    uint32_t remain = len;
    uint32_t block_start = start;
    uint32_t block_len = len;
    uint32_t byteindex = flashbytesindex;
    uint16_t chk_sum;
    uint8_t blocksize = 128;

    QElapsedTimer timer;

    unsigned long chrono;
    unsigned curspeed, tleft;

    int num_128_byte_blocks = (block_len >> 7) & 0xFFFFFFFF;
    int byte_index = block_start & 0xFFFFFFFF;

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    for (int i = 0; i < num_128_byte_blocks; i++)
    {
        if (kill_process)
            return STATUS_ERROR;

        chk_sum = 0;
        for (int j = 0; j < 16; j++)
        {
            // send 16 lots of 8 byte pure data messages to load and flash the new block (16 x 8 bytes = 128 bytes)
            for (int k = 0; k < 8; k++)
            {
                output[k + 4] = (uint8_t)(src[byte_index + k] & 0xFF);
                chk_sum += (output[k + 4] & 0xFF);
                chk_sum = ((chk_sum >> 8) & 0xFF) + (chk_sum & 0xFF);
            }
            byte_index += 8;
            received = serial->write_serial_data_echo_check(output);
        }

        output[4] = (uint8_t)SID_CAN_START_COMM;
        output[5] = (uint8_t)(SID_CAN_FL_WB + 0x03);
        output[6] = (uint8_t)((i >> 8) & 0xFF);
        output[7] = (uint8_t)(i & 0xFF);
        output[8] = (uint8_t)(chk_sum & 0xFF);
        received = serial->write_serial_data_echo_check(output);

        received = serial->read_serial_data(3, serial_read_long_timeout);
        if (received.length())
        {
            if((uint8_t)received.at(0) != SID_CAN_START_COMM || ((uint8_t)received.at(1) & 0xF8) != SID_CAN_FL_WB)
            {
                QByteArray msg;
                QString blockno = QString("%1").arg((uint16_t)num_128_byte_blocks,4,16,QLatin1Char('0')).toUpper();
                QString blocks = QString("%1").arg((uint16_t)(i & 0xFFFF),4,16,QLatin1Char('0')).toUpper();

                msg.clear();
                QString str = "Flashing of 128 byte block unsuccessful, stopping... (" + blockno + "/" + blocks+ ")";
                msg.append(str.toUtf8());
                LOG_E(msg, true, true);
            }
            else
            {
                //qDebug() << "Flashing of 128 byte block successful, proceeding to next 128 byte block";
                //qDebug() << hex << num_128_byte_blocks << "/" << (i & 0xFFFF);
            }
        }

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

        tleft = ((float)flashbytescount - byteindex) / curspeed;  //s
        if (tleft > 9999) {
            tleft = 9999;
        }
        tleft++;

        QString start_address = QString("%1").arg(start,8,16,QLatin1Char('0'));
        msg = QString("Writing chunk @ 0x%1 (%2\% - %3 B/s, ~ %4 s remaining)").arg(start_address).arg((unsigned) 100 * byteindex / flashbytescount,1,10,QLatin1Char('0')).arg((uint32_t)curspeed,1,10,QLatin1Char('0')).arg(tleft,1,10,QLatin1Char('0')).toUtf8();
        LOG_I(msg, true, true);

        float pleft = (float)byteindex / (float)flashbytescount * 100.0f;
        set_progressbar_value(pleft);
    }

    return STATUS_SUCCESS;
}

/*
 * 8bit checksum
 *
 * @return
 */
uint8_t FlashEcuSubaruDensoSH705xDensoCan::cks_add8(QByteArray chksum_data, unsigned len)
{
    uint16_t sum = 0;
    uint8_t data[chksum_data.length()];

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
QByteArray FlashEcuSubaruDensoSH705xDensoCan::send_sid_bf_ssm_init()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;


    output.append((uint8_t)0xBF);

    while (received == "" && loop_cnt < 5)
    {
        LOG_I("SSM init", true, true);
        serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
        delay(500);
        received = serial->read_serial_data(100, receive_timeout);
        loop_cnt++;
    }

    return received;
}

/*
 * Start diagnostic connection
 *
 * @return received response
 */
QByteArray FlashEcuSubaruDensoSH705xDensoCan::send_sid_81_start_communication()
{
    QByteArray output;
    QByteArray received;

    output.append((uint8_t)0x81);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    received = serial->read_serial_data(8, receive_timeout);

    return received;
}

/*
 * Start diagnostic connection
 *
 * @return received response
 */
QByteArray FlashEcuSubaruDensoSH705xDensoCan::send_sid_83_request_timings()
{
    QByteArray output;
    QByteArray received;

    output.append((uint8_t)0x83);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    received = serial->read_serial_data(12, receive_timeout);

    return received;
}

/*
 * Request seed
 *
 * @return seed (4 bytes)
 */
QByteArray FlashEcuSubaruDensoSH705xDensoCan::send_sid_27_request_seed()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    output.append((uint8_t)0x27);
    output.append((uint8_t)0x01);
    received = serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    received = serial->read_serial_data(11, receive_timeout);

    return received;
}

/*
 * Send seed key
 *
 * @return received response
 */
QByteArray FlashEcuSubaruDensoSH705xDensoCan::send_sid_27_send_seed_key(QByteArray seed_key)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    output.append((uint8_t)0x27);
    output.append((uint8_t)0x02);
    output.append(seed_key);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    received = serial->read_serial_data(8, receive_timeout);

    return received;
}

/*
 * Request start diagnostic
 *
 * @return received response
 */
QByteArray FlashEcuSubaruDensoSH705xDensoCan::send_sid_10_start_diagnostic()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    output.append((uint8_t)0x10);
    output.append((uint8_t)0x85);
    output.append((uint8_t)0x02);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
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
 * Request data upload (kernel)
 *
 * @return received response
 */
QByteArray FlashEcuSubaruDensoSH705xDensoCan::send_sid_34_request_upload(uint32_t dataaddr, uint32_t datalen)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    output.append((uint8_t)0x34);
    output.append((uint8_t)(dataaddr >> 16) & 0xFF);
    output.append((uint8_t)(dataaddr >> 8) & 0xFF);
    output.append((uint8_t)dataaddr & 0xFF);
    output.append((uint8_t)0x04);
    output.append((uint8_t)(datalen >> 16) & 0xFF);
    output.append((uint8_t)(datalen >> 8) & 0xFF);
    output.append((uint8_t)datalen & 0xFF);

    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    received = serial->read_serial_data(7, receive_timeout);

    return received;
}

/*
 * Transfer data (kernel)
 *
 * @return received response
 */
QByteArray FlashEcuSubaruDensoSH705xDensoCan::send_sid_36_transferdata(uint32_t dataaddr, QByteArray buf, uint32_t len)
{
    QByteArray output;
    QByteArray received;
    uint32_t blockaddr = 0;
    uint16_t blockno = 0;
    uint16_t maxblocks = 0;

    len &= ~0x03;
    if (!buf.length() || !len) {
        LOG_E("Error in kernel data length!", true, true);
        return NULL;
    }

    maxblocks = (len - 1) >> 7;  // number of 128 byte blocks - 1

    set_progressbar_value(0);

    for (blockno = 0; blockno <= maxblocks; blockno++)
    {
        if (kill_process)
            return NULL;

        blockaddr = dataaddr + blockno * 128;
        output.clear();
        output.append(0x36);
        output.append((uint8_t)(blockaddr >> 16) & 0xFF);
        output.append((uint8_t)(blockaddr >> 8) & 0xFF);
        output.append((uint8_t)blockaddr & 0xFF);

        if (blockno == maxblocks)
        {
            for (uint32_t i = 0; i < len; i++)
            {
                output.append(buf.at(i + blockno * 128));
            }
        }
        else
        {
            for (int i = 0; i < 128; i++)
            {
                output.append(buf.at(i + blockno * 128));
            }
            len -= 128;
        }

        serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
        received = serial->read_serial_data(6, receive_timeout);

        float pleft = (float)blockno / (float)maxblocks * 100;
        set_progressbar_value(pleft);
    }

    set_progressbar_value(100);

    return received;

}

QByteArray FlashEcuSubaruDensoSH705xDensoCan::send_sid_31_start_routine()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x01);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    received = serial->read_serial_data(8, receive_timeout);

    return received;
}

/*
 * Encrypt upload data
 *
 * @return encrypted data
 */
QByteArray FlashEcuSubaruDensoSH705xDensoCan::encrypt_payload(QByteArray buf, uint32_t len)
{
    QByteArray encrypted;

    const uint16_t keytogenerateindex[]={
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

QByteArray FlashEcuSubaruDensoSH705xDensoCan::decrypt_payload(QByteArray buf, uint32_t len)
{
    QByteArray decrypted;

    const uint16_t keytogenerateindex[]={
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

QByteArray FlashEcuSubaruDensoSH705xDensoCan::calculate_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
QByteArray FlashEcuSubaruDensoSH705xDensoCan::request_kernel_init()
{
    QByteArray output;
    QByteArray received;

    request_denso_kernel_init = true;

    output.clear();
    /*
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    */
    output.append((uint8_t)SID_KERNEL_INIT);
    /*
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    */
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
QByteArray FlashEcuSubaruDensoSH705xDensoCan::request_kernel_id()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray kernelid;

    request_denso_kernel_id = true;

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    output.append((uint8_t)SID_CAN_START_COMM);
    output.append((uint8_t)SID_CAN_RECUID);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    serial->write_serial_data_echo_check(output);
    qDebug() << "Request kernel id sent:" << parse_message_to_hex(output);
    delay(100);
    received = serial->read_serial_data(100, serial_read_short_timeout);
    qDebug() << "Request kernel id received:" << parse_message_to_hex(received);

    if (received.length() > 1)
        received.remove(0, 2);
    qDebug() << "Initial request kernel id received and length:" << parse_message_to_hex(received) << received.length();
    kernelid = received;

    while (received != "")
    {
        received = serial->read_serial_data(10, serial_read_short_timeout);
        qDebug() << "Request kernel id received:" << parse_message_to_hex(received);
        received.remove(0, 2);
        kernelid.append(received);
    }

    request_denso_kernel_id = false;

    return kernelid;
}
























/*
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray FlashEcuSubaruDensoSH705xDensoCan::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
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
uint8_t FlashEcuSubaruDensoSH705xDensoCan::calculate_checksum(QByteArray output, bool dec_0x100)
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
QString FlashEcuSubaruDensoSH705xDensoCan::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

void FlashEcuSubaruDensoSH705xDensoCan::set_progressbar_value(int value)
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

void FlashEcuSubaruDensoSH705xDensoCan::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

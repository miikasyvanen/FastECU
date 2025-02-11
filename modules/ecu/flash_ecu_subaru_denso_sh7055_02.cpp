#include "flash_ecu_subaru_denso_sh7055_02.h"

FlashEcuSubaruDensoSH7055_02::FlashEcuSubaruDensoSH7055_02(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
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

void FlashEcuSubaruDensoSH7055_02::run()
{
    this->show();

    int result = STATUS_ERROR;
    set_progressbar_value(0);

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
    LOG_D("MCU type: " + mcu_name + " and index: " + mcu_type_index, true, true);

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
    serial->set_is_iso14230_connection(false);
    serial->set_add_iso14230_header(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    serial->set_serial_port_baudrate("9600");
    tester_id = 0xF0;
    target_id = 0x10;
    // Open serial port
    serial->open_serial_port();
    serial->set_lec_lines(serial->get_requestToSendDisabled(), serial->get_dataTerminalDisabled());

    int ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                   tr("Turn ignition ON and press OK to start initializing connection to ECU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);

    switch (ret)
    {
        case QMessageBox::Ok:
            emit LOG_I("Connecting to Subaru 02 32-bit K-Line bootloader, please wait...", true, true);
            result = connect_bootloader();

            if (result == STATUS_SUCCESS && !kernel_alive)
            {
                emit LOG_I("Initializing Subaru 02 32-bit K-Line kernel upload, please wait...", true, true);
                result = upload_kernel(kernel, ecuCalDef->KernelStartAddr.toUInt(&ok, 16));
            }
            if (result == STATUS_SUCCESS)
            {
                if (cmd_type == "read")
                {
                    emit LOG_I("Reading ROM from Subaru 02 32-bit using K-Line", true, true);
                    result = read_mem(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                }
                else if (cmd_type == "test_write" || cmd_type == "write")
                {
                    emit LOG_I("Writing ROM to Subaru 02 32-bit using K-Line", true, true);
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

FlashEcuSubaruDensoSH7055_02::~FlashEcuSubaruDensoSH7055_02()
{
    delete ui;
}

void FlashEcuSubaruDensoSH7055_02::closeEvent(QCloseEvent *bar)
{
    kill_process = true;
}

int FlashEcuSubaruDensoSH7055_02::connect_bootloader()
{
    QByteArray output;
    QByteArray received;

    if (!serial->is_serial_port_open())
    {
        emit LOG_E("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    delay(200);
    serial->pulse_lec_2_line(200);
    delay(200);
    output.clear();
    for (uint8_t i = 0; i < bootloader_init_request_fxt02.length(); i++)
    {
        output.append(bootloader_init_request_fxt02[i]);
    }
    serial->write_serial_data_echo_check(output);
    emit LOG_I("Sent: " + parse_message_to_hex(output), true, true);
    delay(185);
    received = serial->read_serial_data(output.length(), 100);//serial_read_short_timeout);
    emit LOG_I("Response: " + parse_message_to_hex(received), true, true);

    if (flash_method.endsWith("_ecutek"))
        bootloader_init_response_fxt02_ok = bootloader_init_response_ecutek_fxt02_ok;
    else if (flash_method.endsWith("_cobb"))
        bootloader_init_response_fxt02_ok = bootloader_init_response_cobb_fxt02_ok;
    else
        bootloader_init_response_fxt02_ok = bootloader_init_response_stock_fxt02_ok;

    if (received.length() < 3 || !check_received_message(bootloader_init_response_fxt02_ok, received))
    {
        emit LOG_E("Bad response from bootloader: " + parse_message_to_hex(received), true, true);
    }
    else
    {
        emit LOG_I("Connected to bootloader", true, true);
        return STATUS_SUCCESS;
    }

    delay(100);
    serial->set_lec_lines(serial->get_requestToSendDisabled(), serial->get_dataTerminalDisabled());

    emit LOG_I("Checking if Kernel already uploaded, requesting kernel ID", true, true);
    serial->change_port_speed("62500");

    emit LOG_I("Requesting kernel ID...", true, true);

    received = request_kernel_id();
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
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
    return STATUS_ERROR;
}

int FlashEcuSubaruDensoSH7055_02::upload_kernel(QString kernel, uint32_t kernel_start_addr)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray payload;
    QByteArray received;
    QByteArray msg;
    QByteArray pl_encr;
    uint32_t start_address = 0;
    uint32_t file_len = 0;
    uint32_t pl_len = 0;
    uint32_t len = 0;
    uint32_t ram_addr = 0;
    uint8_t chk_sum = 0;

    start_address = kernel_start_addr;

    if (!serial->is_serial_port_open())
    {
        emit LOG_E("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    if (flash_method.endsWith("_ecutek"))
        serial->change_port_speed("11700");
    else
        serial->change_port_speed("9600");

    // Check kernel file
    if (!file.open(QIODevice::ReadOnly ))
    {
        emit LOG_E("Unable to open kernel file for reading", true, true);
        return -1;
    }

    file_len = file.size();
    pl_len = (file_len + 3) & ~3;
    pl_encr = file.readAll();
    len = pl_len &= ~3;

    for (uint32_t i = 0; i < len; i++) {
        pl_encr[i] = (uint8_t) (pl_encr[i] ^ 0x55) + 0x10;
        chk_sum += pl_encr[i];
    }

    msg.clear();
    msg.append(QString("Start address to upload kernel: 0x%1, length: 0x%2").arg(start_address,8,16,QLatin1Char('0')).arg(len,4,16,QLatin1Char('0')).toUtf8());
    emit LOG_I(msg, true, true);

    output.clear();
    output.append((uint8_t)SUB_UPLOAD_KERNEL & 0xFF);
    output.append((uint8_t)(start_address >> 16) & 0xFF);
    output.append((uint8_t)(start_address >> 8) & 0xFF);
    output.append((uint8_t)((len + 4) >> 16) & 0xFF);
    output.append((uint8_t)((len + 4) >> 8) & 0xFF);
    output.append((uint8_t)(len + 4) & 0xFF);
    output.append((uint8_t)((0x00 ^ 0x55) + 0x10) & 0xFF);
    output.append((uint8_t)0x00 & 0xFF);
    output.append((uint8_t)0x31 & 0xFF);
    output.append((uint8_t)0x61 & 0xFF);

    output[7] = calculate_checksum(output, true);
    output.append(pl_encr);
    chk_sum = calculate_checksum(output, true);
    output.append((uint8_t) chk_sum);

    emit LOG_I("Start sending kernel... please wait...", true, true);
    serial->write_serial_data_echo_check(output);
    received = serial->read_serial_data(100, serial_read_short_timeout);
    msg.clear();
    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }
    if (received.length())
    {
        emit LOG_E("Error on kernel upload!", true, true);
        emit LOG_D("Response: " + QString::number(received.length()) + " bytes '" + msg + "'", true, true);
        return STATUS_ERROR;
    }
    else
        emit LOG_I("Kernel uploaded succesfully", true, true);

    emit LOG_I("Requesting kernel ID", true, true);
    serial->change_port_speed("62500");
    delay(100);

    for (int i = 0; i < 10; i++)
    {
        if (kill_process)
            return STATUS_ERROR;

        received = request_kernel_id();
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        if (received.length() > 0)
        {
            received.remove(0, 5);
            received.remove(received.length() - 1, 1);
            emit LOG_I("Kernel ID: " + received, true, true);
            emit LOG_D("Kernel ID: " + parse_message_to_hex(received), true, true);
            delay(100);
            kernel_alive = true;
            return STATUS_SUCCESS;
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
int FlashEcuSubaruDensoSH7055_02::read_mem(uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray mapdata;

    uint32_t pagesize = 0;
    uint32_t end_addr = 0;
    uint32_t datalen = 0;
    uint32_t cplen = 0;

    uint8_t chk_sum = 0;

    datalen = 6;
    pagesize = 0x0400;
    if (start_addr == 0 && length == 0)
    {
        start_addr = 0;
        length = 0x080000;
    }
    end_addr = start_addr + length;

    uint32_t skip_start = start_addr & (pagesize - 1); //if unaligned, we'll be receiving this many extra bytes
    uint32_t addr = start_addr - skip_start;
    uint32_t willget = (skip_start + length + pagesize - 1) & ~(pagesize - 1);
    uint32_t len_done = 0;  //total data written to file

    timer.start();

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

        output.clear();
        output.append((uint8_t)((SUB_KERNEL_START_COMM >> 8) & 0xFF));
        output.append((uint8_t)(SUB_KERNEL_START_COMM & 0xFF));
        output.append((uint8_t)((datalen + 1) >> 8) & 0xFF);
        output.append((uint8_t)(datalen + 1) & 0xFF);
        output.append((uint8_t)SUB_KERNEL_READ_AREA & 0xFF);
        output.append((uint8_t)0x00 & 0xFF);
        output.append((uint8_t)(addr >> 16) & 0xFF);
        output.append((uint8_t)(addr >> 8) & 0xFF);
        output.append((uint8_t)addr & 0xFF);
        output.append((uint8_t)(pagesize >> 8) & 0xFF);
        output.append((uint8_t)pagesize & 0xFF);
        chk_sum = calculate_checksum(output, false);
        output.append((uint8_t) chk_sum);
        received = serial->write_serial_data_echo_check(output);
        delay(10);
        received = serial->read_serial_data(pagesize + 6, serial_read_extra_long_timeout);

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
                emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
                return STATUS_ERROR;
            }
        }
        else
        {
            emit LOG_E("No valid response from ECU", true, true);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
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
        msg = QString("Kernel read addr: 0x%1 length: 0x%2, %3 B/s %4 s").arg(start_address).arg(block_len).arg(curspeed, 6, 10, QLatin1Char(' ')).arg(tleft, 6, 10, QLatin1Char(' ')).toUtf8();
        emit LOG_I(msg, true, true);
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
 * Write memory to Subaru Denso K-Line 32bit ECUs, nisprog kernel
 *
 * @return success
 */
int FlashEcuSubaruDensoSH7055_02::write_mem(bool test_write)
{
    QByteArray filedata;
    QByteArray output;
    QByteArray received;
    uint16_t chksum;

    filedata = ecuCalDef->FullRomData;

    if (filedata.length() < 190 * 1024)
    {
        emit LOG_D("Padding 160kb file to 192kb", true, true);
        for (int i = 0; i < 0x8000; i++)
            filedata.insert(0x20000, (uint8_t)0xff);
    }

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
    emit LOG_I("Different blocks: ", true, false);
    for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++) {
        if (block_modified[blockno]) {
            emit LOG_I(QString::number(blockno) + ", ", false, false);
            bcnt += 1;
        }
    }
    emit LOG_I(" (total: " + QString::number(bcnt) + ")", false, true);

    serial->set_lec_lines(serial->get_requestToSendEnabled(), serial->get_dataTerminalDisabled());

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
                    emit LOG_E("Block " + QString::number(blockno) + " reflash failed.", true, true);
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
            emit LOG_E("Error in ROM compare", true, true);
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
                emit LOG_I("Don't power off your ECU, kernel is still running and you can try flashing again!", true, true);
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
int FlashEcuSubaruDensoSH7055_02::get_changed_blocks(const uint8_t *src, int *modified)
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
    }
    emit LOG_D("ROM CRC check ready", true, true);

    return 0;
}

/*
 * ROM CRC 32bit K-Line ECUs, nisprog kernel
 *
 * @return
 */
int FlashEcuSubaruDensoSH7055_02::check_romcrc(const uint8_t *src, uint32_t start, uint32_t len, int *modified)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint32_t datalen = 0;
    uint16_t chksum;
    uint16_t pagesize = len;
    uint32_t imgcrc32;
    uint32_t ecucrc32;

    datalen = 8;
    flash_write_init = false;

    output.clear();
    output.append((uint8_t)((SUB_KERNEL_START_COMM >> 8) & 0xFF));
    output.append((uint8_t)(SUB_KERNEL_START_COMM & 0xFF));
    output.append((uint8_t)((datalen + 1) >> 8) & 0xFF);
    output.append((uint8_t)(datalen + 1) & 0xFF);
    output.append((uint8_t)SUB_KERNEL_CRC & 0xFF);
    output.append((uint8_t)(start >> 24) & 0xFF);
    output.append((uint8_t)(start >> 16) & 0xFF);
    output.append((uint8_t)(start >> 8) & 0xFF);
    output.append((uint8_t)start & 0xFF);
    output.append((uint8_t)0x00 & 0xFF);
    output.append((uint8_t)(pagesize >> 16) & 0xFF);
    output.append((uint8_t)(pagesize >> 8) & 0xFF);
    output.append((uint8_t)pagesize & 0xFF);
    chksum = calculate_checksum(output, false);
    output.append((uint8_t)chksum & 0xFF);
    //delay(50);
    received = serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    //delay(200);
    received = serial->read_serial_data(10, serial_read_extra_long_timeout);
    int try_count = 0;
    while (received.length() < 10 && try_count < 20)
    {
        received.append(serial->read_serial_data(10, serial_read_extra_short_timeout));
        try_count++;
        delay(100);
    }
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    if (received.length() > 9)
    {
        if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != 0x42)
        {
            emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    if (received.length())
    {
        if (received.at(0) == 0x7f)
        {
            emit LOG_E("Failed: Wrong answer from ECU", true, true);
            return STATUS_ERROR;
        }
        uint8_t len = (uint8_t)received.at(0);
        if (len > 5)
        {
            received.remove(0, 5);
            received.remove(received.length() - 1, 1);

        }
    }
    else
    {
        emit LOG_E("Failed: No answer from ECU", true, true);
        return STATUS_ERROR;
    }

    ecucrc32 = 0;
    imgcrc32 = crc32(src, pagesize);
    if (received.length() > 3)
        ecucrc32 = ((uint8_t)received.at(0) << 24) | ((uint8_t)received.at(1) << 16) | ((uint8_t)received.at(2) << 8) | (uint8_t)received.at(3);
    msg.clear();
    msg.append(QString("ROM CRC: 0x%1 IMG CRC: 0x%2").arg(ecucrc32,8,16,QLatin1Char('0')).arg(imgcrc32,8,16,QLatin1Char('0')).toUtf8());
    //emit LOG_D(msg, true, true);

    QString ecu_crc32 = QString("%1").arg((uint32_t)ecucrc32,8,16,QLatin1Char('0')).toUpper();
    QString img_crc32 = QString("%1").arg((uint32_t)imgcrc32,8,16,QLatin1Char('0')).toUpper();
    msg = QString("\t" + ecu_crc32 + "\t" + img_crc32).toUtf8();
    if (ecucrc32 != imgcrc32)
    {
        msg.append(QString("\tNO").toUtf8());
        emit LOG_I(msg, false, true);
        *modified = 1;
        serial->read_serial_data(100, serial_read_short_timeout);
        return 0;
    }
    msg.append(QString("\tYES").toUtf8());
    emit LOG_I(msg, false, true);
    *modified = 0;
    serial->read_serial_data(100, serial_read_short_timeout);
    return 0;
}

unsigned int FlashEcuSubaruDensoSH7055_02::crc32(const unsigned char *buf, unsigned int len)
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

void FlashEcuSubaruDensoSH7055_02::init_crc32_tab(void)
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

int FlashEcuSubaruDensoSH7055_02::init_flash_write()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    uint32_t datalen = 0;
    uint16_t chksum;

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
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(200);
    received = serial->read_serial_data(10, serial_read_short_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    if (received.length() > 9)
    {
        if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != 0x45)
        {
            emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
        else
        {
            flashmessagesize = (uint8_t)received.at(6) << 24 | (uint8_t)received.at(7) << 16 | (uint8_t)received.at(8) << 8 | (uint8_t)received.at(9) << 0;
            msg.clear();
            msg.append(QString("Max message length: 0x%1").arg(flashmessagesize,4,16,QLatin1Char('0')).toUtf8());
            emit LOG_I(msg, true, true);
        }
    }
    else
    {
        emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

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
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(200);
    received = serial->read_serial_data(10, serial_read_short_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    if (received.length() > 9)
    {
        if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != 0x46)
        {
            emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
        else
        {
            flashblocksize = (uint8_t)received.at(6) << 24 | (uint8_t)received.at(7) << 16 | (uint8_t)received.at(8) << 8 | (uint8_t)received.at(9) << 0;
            msg.clear();
            msg.append(QString("Flashblock size: 0x%1").arg(flashblocksize,4,16,QLatin1Char('0')).toUtf8());
            emit LOG_I(msg, true, true);
        }
    }
    else
    {
        emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }


    uint8_t SUB_KERNEL_CMD = 0;
    if (test_write)
        SUB_KERNEL_CMD = (uint8_t)(SUB_KERNEL_FLASH_DISABLE & 0xFF);
    else if (!test_write)
        SUB_KERNEL_CMD = (uint8_t)(SUB_KERNEL_FLASH_ENABLE & 0xFF);

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
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    delay(200);
    received = serial->read_serial_data(8, serial_read_short_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != (SUB_KERNEL_CMD | 0x40))
        {
            emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
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
int FlashEcuSubaruDensoSH7055_02::reflash_block(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write)
{
    int errval;

    uint32_t start = 0;
    uint32_t len = 0;
    uint32_t datalen = 0;
    uint16_t pagesize = 0x200;
    uint16_t chksum;

    QByteArray output;
    QByteArray received;
    QByteArray msg;

    if (!flash_write_init)
        if (init_flash_write())
            return STATUS_ERROR;

    if (blockno >= fdt->numblocks) {
        emit LOG_E("Block " + QString::number(blockno) + " out of range!", true, true);
        return STATUS_ERROR;
    }

    start = fdt->fblocks[blockno].start;
    len = fdt->fblocks[blockno].len;

    QString start_addr = QString("%1").arg((uint32_t)start,8,16,QLatin1Char('0')).toUpper();
    QString length = QString("%1").arg((uint32_t)len,8,16,QLatin1Char('0')).toUpper();
    msg = QString("Flash block addr: 0x" + start_addr + " len: 0x" + length).toUtf8();
    emit LOG_I(msg, true, true);

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
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    //delay(50);
    received = serial->read_serial_data(8, serial_read_short_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
    if (received.length() > 7)
    {
        if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != 0x44)
        {
            emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }
    else
    {
        emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }
    float prog_voltage = (((uint8_t)received.at(5) << 8) + (uint8_t)received.at(6)) / 50.0;
    emit LOG_I("Programming voltage: " + QString::number(prog_voltage) + "V", true, true);

    if (flash_block(newdata, start, len)) {
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
int FlashEcuSubaruDensoSH7055_02::flash_block(const uint8_t *src, uint32_t start, uint32_t len)
{
    uint32_t remain = len;
    uint32_t byteindex = flashbytesindex;
    uint32_t datalen = 0;
    uint32_t blockstart = start;
    uint32_t imgcrc32 = 0;
    uint32_t flashblockstart = start;
    uint16_t blocksize = 0x200;
    uint16_t chksum;

    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    unsigned long chrono;
    unsigned curspeed, tleft;

    flashblocksize = 0x1000;

    if (!test_write)
    {
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
        emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
        delay(500);
        received = serial->read_serial_data(8, serial_read_extra_long_timeout);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        if (received.length() > 5)
        {
            if ((uint8_t)received.at(0) == ((SUB_KERNEL_START_COMM >> 8) & 0xFF) && (uint8_t)received.at(1) == (SUB_KERNEL_START_COMM & 0xFF) && (uint8_t)received.at(4) == (SUB_KERNEL_BLANK_PAGE | 0x40))
            {
                emit LOG_I("erased", false, true);
                emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
                return STATUS_ERROR;
            }
            else
            {
                emit LOG_E("Wrong response from ECU", true, true);
                emit LOG_E("Response: " + parse_message_to_hex(received), true, true);
                return STATUS_ERROR;
            }
        }
        else
        {
            emit LOG_E("Wrong response from ECU", true, true);
            emit LOG_E("Response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
    }

    timer.start();
    while (remain) {
        if (kill_process)
            return STATUS_ERROR;

        datalen = blocksize + 4; // 0x200 + 4
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
        delay(50);
        received = serial->read_serial_data(6, serial_read_long_timeout);
        uint8_t loopcount = 0;
        while (received.length() && loopcount < 10)
        {
            if ((uint8_t)received.at(0) != 0xBE)
                received.remove(0, 1);
            received.append(serial->read_serial_data(6, 1));
            loopcount++;
        }
        if (received.length() > 5)
        {
            if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != 0x62)
            {
                emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
                return STATUS_ERROR;
            }
        }
        else
        {
            emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }

        QString start_address = QString("%1").arg(start,8,16,QLatin1Char('0')).toUpper();
        msg = QString("Write flash buffer: 0x%1 (%2\% - %3 B/s, ~ %4 s)").arg(start_address).arg((unsigned) 100 * (len - remain) / len,1,10,QLatin1Char('0')).arg((uint32_t)curspeed,1,10,QLatin1Char('0')).arg(tleft,1,10,QLatin1Char('0')).toUtf8();
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
            emit LOG_I("Write complete, validating... ", true, true);
            imgcrc32 = crc32(&src[flashblockstart], flashblocksize);

            uint8_t SUB_KERNEL_CMD = 0;
            if (test_write)
            {
                SUB_KERNEL_CMD = (uint8_t)(SUB_KERNEL_VALIDATE_FLASH_BUFFER & 0xFF);
            }
            else if (!test_write)
            {
                SUB_KERNEL_CMD = (uint8_t)(SUB_KERNEL_COMMIT_FLASH_BUFFER & 0xFF);
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
            emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
            delay(200);
            received = serial->read_serial_data(6, serial_read_extra_long_timeout);
            emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
            if (received.length() > 5)
            {
                if ((uint8_t)received.at(0) != ((SUB_KERNEL_START_COMM >> 8) & 0xFF) || (uint8_t)received.at(1) != (SUB_KERNEL_START_COMM & 0xFF) || (uint8_t)received.at(4) != (SUB_KERNEL_CMD + 0x40))
                {
                    emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
                    return STATUS_ERROR;
                }
            }
            else
            {
                emit LOG_E("Wrong response from ECU: " + parse_message_to_hex(received), true, true);
                return STATUS_ERROR;
            }
            emit LOG_I("Flashblock: OK", true, true);
            flashblockstart += flashblocksize;
        }
    }

    received = serial->read_serial_data(100, serial_read_short_timeout);

    return STATUS_SUCCESS;
}

/*
 * Request kernel id
 *
 * @return kernel id
 */
QByteArray FlashEcuSubaruDensoSH7055_02::request_kernel_id()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray kernelid;

    request_denso_kernel_id = true;

    output.clear();
    output.append(SID_RECUID);

    received = serial->write_serial_data_echo_check(output);
    delay(100);
    received = serial->read_serial_data(100, serial_read_short_timeout);

    received.remove(0, 1);
    kernelid = received;

    while (received != "")
    {
        received = serial->read_serial_data(1, serial_read_short_timeout);
        received.remove(0, 1);
        kernelid.append(received);
    }

    request_denso_kernel_id = false;

    return kernelid;
}

/*
 * ECU init
 *
 * @return ECU ID and capabilities
 */
QByteArray FlashEcuSubaruDensoSH7055_02::send_sid_bf_ssm_init()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;


    output.append((uint8_t)0xBF);
    output = add_ssm_header(output, tester_id, target_id, false);

    while (received == "" && loop_cnt < 5)
    {
        if (kill_process)
            break;

        emit LOG_I("SSM init", true, true);
        serial->write_serial_data_echo_check(output);
        emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
        delay(500);
        received = serial->read_serial_data(8, receive_timeout);
        emit LOG_D("Response: " + parse_message_to_hex(received), true, true);
        loop_cnt++;
    }

    return received;
}

/*
 * Request seed
 *
 * @return seed (4 bytes)
 */
QByteArray FlashEcuSubaruDensoSH7055_02::send_sid_27_request_seed()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0x27);
    output.append((uint8_t)0x01);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Send seed key
 *
 * @return received response
 */
QByteArray FlashEcuSubaruDensoSH7055_02::send_sid_27_send_seed_key(QByteArray seed_key)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0x27);
    output.append((uint8_t)0x02);
    output.append(seed_key);
    output = add_ssm_header(output, tester_id, target_id, false);
    serial->write_serial_data_echo_check(output);
    emit LOG_D("Sent: " + parse_message_to_hex(output), true, true);
    received = serial->read_serial_data(8, receive_timeout);
    emit LOG_D("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray FlashEcuSubaruDensoSH7055_02::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
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
uint8_t FlashEcuSubaruDensoSH7055_02::calculate_checksum(QByteArray output, bool dec_0x100)
{
    uint8_t checksum = 0;

    for (uint16_t i = 0; i < output.length(); i++)
        checksum += (uint8_t)output.at(i);

    if (dec_0x100)
        checksum = (uint8_t) (0x100 - checksum);

    return checksum;
}

int FlashEcuSubaruDensoSH7055_02::check_received_message(QByteArray msg, QByteArray received)
{
    for (int i = 0; i < msg.length(); i++)
    {
        if (received.length() > i - 1)
            if (msg.at(i) != received.at(i))
                return 0;
    }

    return 1;
}

/*
 * Parse QByteArray to readable form
 *
 * @return parsed message
 */
QString FlashEcuSubaruDensoSH7055_02::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

void FlashEcuSubaruDensoSH7055_02::set_progressbar_value(int value)
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

void FlashEcuSubaruDensoSH7055_02::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

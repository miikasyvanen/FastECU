#include "flash_ecu_subaru_denso_mc68hc16y5_02.h"

FlashEcuSubaruDensoMC68HC16Y5_02::FlashEcuSubaruDensoMC68HC16Y5_02(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
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

void FlashEcuSubaruDensoMC68HC16Y5_02::run()
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

    int ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                   tr("Turn ignition ON and press OK to start initializing connection to ECU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);

    switch (ret)
    {
        case QMessageBox::Ok:
            send_log_window_message("Connecting to Subaru 02 32-bit K-Line bootloader, please wait...", true, true);
            result = connect_bootloader_subaru_denso_kline_wrx02();

            if (result == STATUS_SUCCESS && !kernel_alive)
            {
                emit external_logger("Preparing, please wait...");
                send_log_window_message("Initializing Subaru 02 32-bit K-Line kernel upload, please wait...", true, true);
                result = upload_kernel_subaru_denso_kline_wrx02(kernel, ecuCalDef->KernelStartAddr.toUInt(&ok, 16));
            }
            if (result == STATUS_SUCCESS)
            {
                if (cmd_type == "read")
                {
                    emit external_logger("Reading ROM, please wait...");
                    send_log_window_message("Reading ROM from Subaru 02 32-bit using K-Line", true, true);
                    result = read_mem_subaru_denso_kline_16bit(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                }
                else if (cmd_type == "test_write" || cmd_type == "write")
                {
                    emit external_logger("Writing ROM, please wait...");
                    send_log_window_message("Writing ROM to Subaru 02 32-bit using K-Line", true, true);
                    result = write_mem_subaru_denso_kline_16bit(test_write);
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

FlashEcuSubaruDensoMC68HC16Y5_02::~FlashEcuSubaruDensoMC68HC16Y5_02()
{

}

void FlashEcuSubaruDensoMC68HC16Y5_02::closeEvent(QCloseEvent *bar)
{
    kill_process = true;
}
/*
int FlashEcuSubaruDensoMC68HC16Y5_02::init_flash_denso_kline_wrx02()
{
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
    //send_log_window_message("MCU type: " + mcu_name + " and index: " + mcu_type_index, true, true);
    qDebug() << "MCU type:" << mcu_name << mcu_type_string << "and index:" << mcu_type_index;

    int result = STATUS_ERROR;

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

    // Set serial port
    serial->set_is_iso14230_connection(true);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    tester_id = 0xF0;
    target_id = 0x10;
    // Open serial port
    serial->open_serial_port();

    QMessageBox::information(this, tr("Connecting to ECU"), "Turn ignition ON and press OK to start initializing connection");
    //QMessageBox::information(this, tr("Connecting to ECU"), "Press OK to start countdown!");

    send_log_window_message("Connecting to Subaru 02 16-bit K-line bootloader, please wait...", true, true);
    result = connect_bootloader_subaru_denso_kline_wrx02();

    if (result == STATUS_SUCCESS && !kernel_alive)
    {
        emit external_logger("Preparing, please wait...");
        send_log_window_message("Initializing Subaru 02 16-bit K-Line kernel upload, please wait...", true, true);
        result = upload_kernel_subaru_denso_kline_wrx02(kernel, ecuCalDef->KernelStartAddr.toUInt(&ok, 16));
    }
    if (result == STATUS_SUCCESS)
    {
        if (cmd_type == "read")
        {
            emit external_logger("Reading ROM, please wait...");
            send_log_window_message("Reading ROM from Subaru 02 16-bit using K-Line", true, true);
            result = read_mem_subaru_denso_kline_16bit(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
        }
        else if (cmd_type == "test_write" || cmd_type == "write")
        {
            emit external_logger("Writing ROM, please wait...");
            send_log_window_message("Writing ROM to Subaru 02 16-bit using K-Line", true, true);
            result = write_mem_subaru_denso_kline_16bit(test_write);
        }
    }
    emit external_logger("Finished");
    return result;
}
*/
int FlashEcuSubaruDensoMC68HC16Y5_02::connect_bootloader_subaru_denso_kline_wrx02()
{
    QByteArray output;
    QByteArray received;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    // Change serial speed and set 'line end checks' to low level
    serial->change_port_speed("9600");
    serial->set_lec_lines(serial->get_requestToSendDisabled(), serial->get_dataTerminalDisabled());

    //if (connect_bootloader_start_countdown(bootloader_start_countdown))
    //    return STATUS_ERROR;

    delay(200);
    serial->pulse_lec_2_line(200);
    output.clear();
    for (uint8_t i = 0; i < denso_bootloader_init_request_wrx02.length(); i++)
    {
        output.append(denso_bootloader_init_request_wrx02[i]);
    }
    //received = serial->write_serial_data_echo_check(output);
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent to bootloader: " + parse_message_to_hex(output), true, true);
    received = serial->read_serial_data(output.length(), serial_read_short_timeout);
    send_log_window_message("Response from bootloader: " + parse_message_to_hex(received), true, true);

    if (flash_method.endsWith("_ecutek"))
        denso_bootloader_init_response_wrx02_ok = denso_bootloader_init_response_ecutek_wrx02_ok;
    if (flash_method.endsWith("_cobb"))
        denso_bootloader_init_response_wrx02_ok = denso_bootloader_init_response_cobb_wrx02_ok;
    else
        denso_bootloader_init_response_wrx02_ok = denso_bootloader_init_response_stock_wrx02_ok;

    if (received.length() != 3 || !check_received_message(denso_bootloader_init_response_wrx02_ok, received))
    {
        send_log_window_message("Bad response from bootloader: " + parse_message_to_hex(received), true, true);
    }
    else
    {
        send_log_window_message("Connected to bootloader", true, true);
        return STATUS_SUCCESS;
    }

    delay(10);

    send_log_window_message("Checking if Kernel already uploaded, requesting kernel ID", true, true);
    serial->change_port_speed("39473");

    received = request_kernel_id();
    qDebug() << parse_message_to_hex(received);
    if (received.length() > 0)
    {
        send_log_window_message("Kernel already uploaded, requesting kernel ID", true, true);
        delay(100);

        kernel_alive = true;
        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}

int FlashEcuSubaruDensoMC68HC16Y5_02::upload_kernel_subaru_denso_kline_wrx02(QString kernel, uint32_t kernel_start_addr)
{
    QFile file(kernel);
    QFile encrypted_kernel(kernel);

    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray pl_encr;
    uint32_t start_address = 0;
    uint32_t file_len = 0;
    uint32_t pl_len = 0;
    uint32_t len = 0;
    uint8_t chk_sum = 0;

    start_address = kernel_start_addr;//flashdevices[mcu_type_index].kblocks->start;
    qDebug() << "Start address to upload kernel:" << hex << start_address;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    //serial->change_port_speed("9600");
    serial->set_add_iso14230_header(false);

    // Check kernel file
    if (!file.open(QIODevice::ReadOnly ))
    {
        send_log_window_message("Unable to open kernel file for reading", true, true);
        return -1;
    }

    file_len = file.size();
    pl_len = file_len;
    pl_encr = file.readAll();
    len = pl_len;

    output.clear();
    output.append((uint8_t)(SID_OE_UPLOAD_KERNEL) & 0xFF);
    output.append((uint8_t)(start_address >> 16) & 0xFF);
    output.append((uint8_t)(start_address >> 8) & 0xFF);
    output.append((uint8_t)(len >> 16) & 0xFF);
    output.append((uint8_t)(len >> 8) & 0xFF);
    output.append((uint8_t)len & 0xFF);

    //pl_encr = sub_transform_wrx02_kernel_block();
    output.append(pl_encr);
    chk_sum = calculate_checksum(output, true);
    output.append((uint8_t)chk_sum);

    send_log_window_message("Start sending kernel... please wait...", true, true);
    received = serial->write_serial_data_echo_check(output);
    received = serial->read_serial_data(100, serial_read_short_timeout);
    msg.clear();
    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }
    if (received.length())
    {
        send_log_window_message("Message received " + QString::number(received.length()) + " bytes '" + msg + "'", true, true);
        send_log_window_message("Error on kernel upload!", true, true);
        return STATUS_ERROR;
    }
    else
        send_log_window_message("Kernel uploaded succesfully", true, true);

    send_log_window_message("Requesting kernel ID", true, true);
    qDebug() << "Requesting kernel ID";

    serial->change_port_speed("39473");
    delay(200);
    received.clear();
    while (received == "")
    {
        received = request_kernel_id();
        delay(500);
    }
    send_log_window_message("Kernel ID: " + received, true, true);
    qDebug() << "Kernel ID: " << parse_message_to_hex(received);

    delay(200);

    return STATUS_SUCCESS;
}

/*******************************************************
 *  Read ROM 16bit K-Line ECUs
 ******************************************************/
int FlashEcuSubaruDensoMC68HC16Y5_02::read_mem_subaru_denso_kline_16bit(uint32_t start_addr, uint32_t length)
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
        length = 0x030000;
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

        pleft = (float)(addr - start_addr) / (float)(length + 0x8000) * 100.0f;
        set_progressbar_value(pleft);

        if (addr >= flashdevices[mcu_type_index].rblocks->start && addr < (flashdevices[mcu_type_index].rblocks->start + flashdevices[mcu_type_index].rblocks->len))
        {

            received.clear();
            for (unsigned int j = flashdevices[mcu_type_index].rblocks->start; j < (flashdevices[mcu_type_index].rblocks->start + flashdevices[mcu_type_index].rblocks->len); j++)
            {
                received.append((uint8_t)0x00);
            }
            mapdata.append(received);

            addr = 0x028000;
        }

        output.clear();
        output.append((uint8_t)((SID_OE_KERNEL_START_COMM >> 8) & 0xFF));
        output.append((uint8_t)(SID_OE_KERNEL_START_COMM & 0xFF));
        output.append((uint8_t)((datalen + 1) >> 8) & 0xFF);
        output.append((uint8_t)(datalen + 1) & 0xFF);
        output.append((uint8_t)SID_OE_KERNEL_READ_AREA & 0xFF);
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

        if (received.startsWith("\xbe\xef"))
        {
            received.remove(0, 5);
            received.remove(received.length() - 1, 1);
            mapdata.append(received);
            //qDebug() << "DATA:" << addr << parse_message_to_hex(received);
        }
        else
        {
            //qDebug() << "ERROR IN DATA RECEIVE!";
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
        msg = QString("Kernel read addr:  0x%1  length:  0x%2,  %3  B/s  %4 s remaining").arg(start_address).arg(block_len).arg(curspeed, 6, 10, QLatin1Char(' ')).arg(tleft, 6, 10, QLatin1Char(' ')).toUtf8();
        send_log_window_message(msg, true, true);
        delay(1);

        len_done += cplen;
        addr += (numblocks * pagesize);
        willget -= pagesize;
    }

    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*******************************************************
 *  Write ROM 16bit K-Line ECUs
 ******************************************************/
int FlashEcuSubaruDensoMC68HC16Y5_02::write_mem_subaru_denso_kline_16bit(bool test_write)
{
    QByteArray filedata;
    QByteArray output;
    QByteArray received;
    uint16_t chksum;

    filedata = ecuCalDef->FullRomData;

    uint8_t data_array[filedata.length()];

    //qDebug() << filename << origfilename;

    int block_modified[16] = {0};

    unsigned bcnt = 0;
    unsigned blockno;

    for (int i = 0; i < filedata.length(); i++)
    {
        data_array[i] = filedata.at(i);
    }

    send_log_window_message("--- comparing ECU flash memory pages to image file ---", true, true);
    send_log_window_message("seg\tstart\tlen\tsame?", true, true);

    if (get_changed_blocks_16bit_kline(data_array, block_modified))
    {
        send_log_window_message("Error in ROM compare", true, true);
        return STATUS_ERROR;
    }

    bcnt = 0;
    send_log_window_message("Different blocks : ", true, false);
    for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++) {
        if (block_modified[blockno]) {
            send_log_window_message(QString::number(blockno) + ", ", false, false);
            bcnt += 1;
        }
    }
    send_log_window_message(" (total: " + QString::number(bcnt) + ")", false, true);

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

        send_log_window_message("--- start writing ROM file to ECU flash memory ---", true, true);
        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++)
        {
            if (block_modified[blockno])
            {
/*                if (reflash_block_16bit_kline(&data_array[flashdevices[mcu_type_index].fblocks->start], &flashdevices[mcu_type_index], blockno, test_write))
                {
                    send_log_window_message("Block " + QString::number(blockno) + " reflash failed.", true, true);
                    return STATUS_ERROR;
                }
                else
                {
                    flashbytesindex += flashdevices[mcu_type_index].fblocks[blockno].len;
                    send_log_window_message("Block " + QString::number(blockno) + " reflash complete.", true, true);
                }*/
            }
        }

        send_log_window_message("--- comparing ECU flash memory pages to image file after reflash ---", true, true);
        send_log_window_message("seg\tstart\tlen\tsame?", true, true);

        if (get_changed_blocks_16bit_kline(data_array, block_modified))
        {
            send_log_window_message("Error in ROM compare", true, true);
            return STATUS_ERROR;
        }

        bcnt = 0;
        send_log_window_message("Different blocks : ", true, false);
        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++) {
            if (block_modified[blockno])
            {
                send_log_window_message(QString::number(blockno) + ", ", false, false);
                bcnt += 1;
            }
        }
        send_log_window_message(" (total: " + QString::number(bcnt) + ")", false, true);
        if (!test_write)
        {
            if (bcnt)
            {
                send_log_window_message("*** ERROR IN FLASH PROCESS ***", true, true);
                send_log_window_message("Don't power off your ECU, kernel is still running and you can try flashing again!", true, true);
            }
        }
        else
            send_log_window_message("*** Test write PASS, it's ok to perform actual write! ***", true, true);
    }
    else
    {
        send_log_window_message("*** Compare results no difference between ROM and ECU data, no flashing needed! ***", true, true);
    }

    return STATUS_SUCCESS;
}

/*******************************************************
 *  Compare ROM 16bit K-Line ECUs
 ******************************************************/
int FlashEcuSubaruDensoMC68HC16Y5_02::get_changed_blocks_16bit_kline(const uint8_t *src, int *modified)
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
        //qDebug() << msg;
        send_log_window_message(msg, true, false);
        // do CRC comparison with ECU //
        if (check_romcrc_16bit_kline(&src[0], bs, blen, &modified[blockno])) {
            return -1;
        }
    }
    qDebug() << "ROM CRC check ready";

    return 0;
}

/*******************************************************
 *  ROM CRC 16bit K-Line ECUs
 ******************************************************/
int FlashEcuSubaruDensoMC68HC16Y5_02::check_romcrc_16bit_kline(const uint8_t *src, uint32_t start, uint32_t len, int *modified)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint32_t datalen = 0;
    uint16_t chksum;
    uint16_t chunko;
    uint16_t pagesize = ROMCRC_CHUNKSIZE_16BIT;
    uint32_t imgcrc;
    uint32_t romcrc;

    len = (len + ROMCRC_LENMASK_16BIT) & ~ROMCRC_LENMASK_16BIT;

    chunko = start / ROMCRC_CHUNKSIZE_16BIT;

    datalen = 8;

    for (uint32_t i = 0; i < len; i += ROMCRC_ITERSIZE_16BIT, chunko += ROMCRC_NUMCHUNKS_16BIT)
    {
        unsigned chunk_cnt;
        for (chunk_cnt = 0; chunk_cnt < ROMCRC_NUMCHUNKS_16BIT; chunk_cnt++) {
            if (kill_process)
                return STATUS_ERROR;

            output.clear();
            output.append((uint8_t)((SID_OE_KERNEL_START_COMM >> 8) & 0xFF));
            output.append((uint8_t)(SID_OE_KERNEL_START_COMM & 0xFF));
            output.append((uint8_t)((datalen + 1) >> 8) & 0xFF);
            output.append((uint8_t)(datalen + 1) & 0xFF);
            output.append((uint8_t)SID_OE_KERNEL_CRC & 0xFF);
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

            received = serial->write_serial_data_echo_check(output);
            //qDebug() << hex << chunk_cnt << output.length() << start << pagesize << parse_message_to_hex(output);

            delay(50);
            received = serial->read_serial_data(50, serial_read_long_timeout);
            delay(50);
            received = serial->read_serial_data(50, serial_read_long_timeout);

            imgcrc = crc32(src + start, pagesize);
            //romcrc = byte_to_int32(received);
            romcrc = ((received.at(0) & 0xFF) << 24) + ((received.at(1) & 0xFF) << 16) + ((received.at(2) & 0xFF) << 8) + (received.at(3) & 0xFF);

            //qDebug() << hex << chunk_cnt << received.length() << parse_message_to_hex(received) << "->" << hex << imgcrc;
            qDebug() << hex << chunk_cnt << received.length() << parse_message_to_hex(received) << romcrc << "->" << hex << imgcrc;

            start += ROMCRC_CHUNKSIZE_16BIT;
        }

        if (imgcrc == romcrc)
            continue;

        send_log_window_message("\tNO", false, true);

        //confirmed bad CRC, we can exit
        *modified = 1;

        return 0;
    }   //for

    send_log_window_message("\tYES", false, true);
    *modified = 0;
    serial->read_serial_data(100, serial_read_short_timeout);
    return 0;

badexit:
    serial->read_serial_data(100, serial_read_short_timeout);
    return -1;
}

static unsigned int crc_table[256] = {
   0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
   0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
   0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
   0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
   0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
   0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
   0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
   0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
   0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
   0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
   0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
   0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
   0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
   0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
   0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
   0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
   0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
   0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
   0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
   0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
   0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
   0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
   0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
   0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
   0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
   0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
   0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
   0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
   0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
   0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
   0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
   0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
   0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
   0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
   0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
   0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
   0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
   0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
   0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
   0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
   0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
   0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
   0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
   0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
   0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
   0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
   0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
   0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
   0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
   0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
   0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
   0x2d02ef8dL
};

unsigned int FlashEcuSubaruDensoMC68HC16Y5_02::crc32(const unsigned char *buf, unsigned int len)
{
    unsigned int crc = 0xFFFFFFFF;
    if (buf == NULL)
        return 0L;
    while (len--)
        crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);

    return crc ^ 0xFFFFFFFF;
}

QByteArray FlashEcuSubaruDensoMC68HC16Y5_02::request_kernel_init()
{
    QByteArray output;
    QByteArray received;

    send_log_window_message("No kernel init option in 16-bit denso yet", true, true);

    return received;
}

/*******************************************************
 *  Request kernel id
 ******************************************************/
QByteArray FlashEcuSubaruDensoMC68HC16Y5_02::request_kernel_id()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray kernelid;

    uint8_t chk_sum = 0;

    qDebug() << "Request kernel id";
    request_denso_kernel_id = true;

    output.clear();
    output.append((uint8_t)((SID_OE_KERNEL_START_COMM >> 8) & 0xFF));
    output.append((uint8_t)(SID_OE_KERNEL_START_COMM & 0xFF));
    output.append((uint8_t)0x00 & 0xFF);
    output.append((uint8_t)0x01 & 0xFF);
    output.append((uint8_t)(SID_OE_KERNEL_ID & 0xFF));

    chk_sum = calculate_checksum(output, false);
    output.append((uint8_t) chk_sum);
    received = serial->write_serial_data_echo_check(output);
    delay(200);
    received.clear();
    received = serial->read_serial_data(100, serial_read_short_timeout);
    kernelid = received;

    request_denso_kernel_id = false;

    //qDebug() << "kernel ID:" << parse_message_to_hex(kernelid);

    return kernelid;
}






/*
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray FlashEcuSubaruDensoMC68HC16Y5_02::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
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
uint8_t FlashEcuSubaruDensoMC68HC16Y5_02::calculate_checksum(QByteArray output, bool dec_0x100)
{
    uint8_t checksum = 0;

    for (uint16_t i = 0; i < output.length(); i++)
        checksum += (uint8_t)output.at(i);

    if (dec_0x100)
        checksum = (uint8_t) (0x100 - checksum);

    return checksum;
}

int FlashEcuSubaruDensoMC68HC16Y5_02::check_received_message(QByteArray msg, QByteArray received)
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
 * Countdown prior power on
 *
 * @return
 */
int FlashEcuSubaruDensoMC68HC16Y5_02::connect_bootloader_start_countdown(int timeout)
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
        delay(750);
        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}

/*
 * Parse QByteArray to readable form
 *
 * @return parsed message
 */
QString FlashEcuSubaruDensoMC68HC16Y5_02::parse_message_to_hex(QByteArray received)
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
int FlashEcuSubaruDensoMC68HC16Y5_02::send_log_window_message(QString message, bool timestamp, bool linefeed)
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

void FlashEcuSubaruDensoMC68HC16Y5_02::set_progressbar_value(int value)
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

void FlashEcuSubaruDensoMC68HC16Y5_02::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

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
            send_log_window_message("Connecting to Subaru 01-05 16-bit K-Line bootloader, please wait...", true, true);
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
                    send_log_window_message("Reading ROM from Subaru 01-05 16-bit using K-Line", true, true);
                    result = read_mem_subaru_denso_kline_16bit(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                }
                else if (cmd_type == "test_write" || cmd_type == "write")
                {
                    emit external_logger("Writing ROM, please wait...");
                    send_log_window_message("Writing ROM to Subaru 01-05 16-bit using K-Line", true, true);
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
    qDebug() << "Sent to bootloader: " + parse_message_to_hex(output);
    received = serial->read_serial_data(output.length(), serial_read_short_timeout);
    send_log_window_message("Response from bootloader: " + parse_message_to_hex(received), true, true);
    qDebug() << "Response from bootloader: " + parse_message_to_hex(received);

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

    for (int i = 0; i < 10; i++)
    {
        received = request_kernel_id();
        qDebug() << parse_message_to_hex(received);
        if (received.length() > 0)
        {
            send_log_window_message("Kernel already uploaded, requesting kernel ID", true, true);
            delay(100);

            kernel_alive = true;
            return STATUS_SUCCESS;
        }
        delay(500);
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

    delay(1500);
    serial->change_port_speed("39473");
    received.clear();
    while (received == "")
    {
        if (kill_process)
            return STATUS_ERROR;
        received = request_kernel_id();
        delay(500);
    }
    received.remove(0, 5);
    received.remove(received.length() - 1, 1);
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
        length = 0x028000;
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
                received.append((uint8_t)0xff);
            }
            mapdata.append(received);

            addr = flashdevices[mcu_type_index].rblocks->start + flashdevices[mcu_type_index].rblocks->len;
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

    set_progressbar_value(0);

    for (int i = 0; i < filedata.length(); i++)
    {
        data_array[i] = filedata.at(i);
    }

    send_log_window_message("--- Comparing ECU flash memory pages to image file after reflash ---", true, true);
    send_log_window_message("seg\tstart\tlen\tecu crc\timg crc\tsame?", true, true);

    if (get_changed_blocks_16bit_kline(data_array, block_modified))
    {
        send_log_window_message("Error in ROM compare", true, true);
        return STATUS_ERROR;
    }

    bcnt = 0;
    send_log_window_message("Different blocks: ", true, false);
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
        set_progressbar_value(100);

        send_log_window_message("--- Comparing ECU flash memory pages to image file after reflash ---", true, true);
        send_log_window_message("seg\tstart\tlen\tecu crc\timg crc\tsame?", true, true);

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
        if (check_romcrc_16bit_kline(&src[bs], bs, blen, &modified[blockno])) {
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
    uint16_t pagesize = len;
    uint32_t imgcrc32;
    uint32_t ecucrc32;

    datalen = 8;

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
    //delay(50);
    received = serial->write_serial_data_echo_check(output);
    delay(200);
    received = serial->read_serial_data(10, serial_read_long_timeout);
    qDebug() << "Received: " + parse_message_to_hex(received);
    if (received.length())
    {
        if (received.at(0) == 0x7f)
        {
            send_log_window_message("", false, true);
            send_log_window_message("Failed: Wrong answer from ECU", true, true);
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
        send_log_window_message("", false, true);
        send_log_window_message("Failed: No answer from ECU", true, true);
        return STATUS_ERROR;
    }

    ecucrc32 = 0;
    imgcrc32 = crc32(src, pagesize);
    if (received.length() > 3)
        ecucrc32 = ((uint8_t)received.at(0) << 24) | ((uint8_t)received.at(1) << 16) | ((uint8_t)received.at(2) << 8) | (uint8_t)received.at(3);
    msg.clear();
    msg.append(QString("ROM CRC: 0x%1 IMG CRC: 0x%2").arg(ecucrc32,8,16,QLatin1Char('0')).arg(imgcrc32,8,16,QLatin1Char('0')).toUtf8());
    qDebug() << msg;

    QString ecu_crc32 = QString("%1").arg((uint32_t)ecucrc32,8,16,QLatin1Char('0')).toUpper();
    QString img_crc32 = QString("%1").arg((uint32_t)imgcrc32,8,16,QLatin1Char('0')).toUpper();
    msg = QString("\t" + ecu_crc32 + "\t" + img_crc32).toUtf8();
    send_log_window_message(msg, false, false);
    if (ecucrc32 != imgcrc32)
    {
        send_log_window_message("\tNO", false, true);
        *modified = 1;
        serial->read_serial_data(100, serial_read_short_timeout);
        return 0;
    }

    send_log_window_message("\tYES", false, true);
    *modified = 0;
    serial->read_serial_data(100, serial_read_short_timeout);
    return 0;
}

unsigned int FlashEcuSubaruDensoMC68HC16Y5_02::crc32(const unsigned char *buf, unsigned int len)
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

void FlashEcuSubaruDensoMC68HC16Y5_02::init_crc32_tab(void) {
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

    qDebug() << "kernel ID:" << parse_message_to_hex(kernelid);

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

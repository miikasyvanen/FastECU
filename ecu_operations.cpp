#include "ecu_operations.h"

EcuOperations::EcuOperations(QWidget *ui, SerialPortActions *serial, QString mcu_type_string, int mcu_type_index)
{
    this->setParent(ui);
    this->setAttribute(Qt::WA_QuitOnClose, true);
    this->mcu_type_string = mcu_type_string;
    this->mcu_type_index = mcu_type_index;
    this->serial = serial;
    this->flash_window = ui;

    set_progressbar_value(0);
}

EcuOperations::~EcuOperations()
{

}

void EcuOperations::closeEvent(QCloseEvent *bar)
{

}

/*******************************************************
 *
 *  Denso ECU operations, modded NisProg kernels
 *
 ******************************************************/
/*******************************************************
 *  Request kernel init
 ******************************************************/
QByteArray EcuOperations::request_kernel_init()
{
    QByteArray output;
    QByteArray received;

    if (mcu_type_string == "68HC16Y5")
    {
        send_log_window_message("No kernel init option in 16-bit denso yet", true, true);
    }
    if (mcu_type_string == "SH7055" || mcu_type_string == "SH7058")
    {
        request_denso_kernel_init = true;

        output.clear();
        output.append(SID_KERNEL_INIT);
        received = serial->write_serial_data_echo_check(output);
        delay(500);
        received = serial->read_serial_data(100, serial_read_short_timeout);

        request_denso_kernel_init = false;
    }

    return received;
}

/*******************************************************
 *  Request kernel id
 ******************************************************/
QByteArray EcuOperations::request_kernel_id()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray kernelid;

    uint8_t chk_sum = 0;

    if (mcu_type_string == "MC68HC16Y5")
    {
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
    }
    if (mcu_type_string == "SH7055" || mcu_type_string == "SH7058")
    {
        request_denso_kernel_id = true;

        output.clear();
        if (serial->is_can_connection || serial->is_iso15765_connection)
        {
            output.append((uint8_t)0x00);
            output.append((uint8_t)0x0F);
            output.append((uint8_t)0xFF);
            output.append((uint8_t)0xFE);
            output.append((uint8_t)SID_START_COMM_CAN);
            output.append((uint8_t)0xA0);
        }
        else
            output.append(SID_RECUID);

        received = serial->write_serial_data_echo_check(output);
        delay(100);
        received = serial->read_serial_data(100, serial_read_short_timeout);

        received.remove(0, 1);
        if (serial->is_can_connection || serial->is_iso15765_connection)
            received.remove(0, 1);
        kernelid = received;

        while (received != "")
        {
            received = serial->read_serial_data(1, serial_read_short_timeout);
            received.remove(0, 1);
            if (serial->is_can_connection || serial->is_iso15765_connection)
                received.remove(0, 1);
            kernelid.append(received);
        }

        request_denso_kernel_id = false;
    }

    //qDebug() << "kernel ID:" << parse_message_to_hex(kernelid);

    return kernelid;
}

/*******************************************************
 *  Read ROM 16bit K-Line ECUs
 ******************************************************/
int EcuOperations::read_mem_16bit_kline(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t start_addr, uint32_t length)
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
 *  Read ROM 32bit K-Line ECUs
 ******************************************************/
int EcuOperations::read_mem_32bit_kline(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray mapdata;
    uint32_t cplen = 0;

    uint32_t skip_start = start_addr & (32 - 1); //if unaligned, we'll be receiving this many extra bytes
    uint32_t addr = start_addr - skip_start;
    uint32_t willget = (skip_start + length + 31) & ~(32 - 1);
    uint32_t len_done = 0;  //total data written to file

    #define NP10_MAXBLKS    32   //# of blocks to request per loop. Too high might flood us
    serial->is_packet_header = true;
    //serial->iso14230_checksum = true;

    output.append(SID_DUMP);
    output.append(SID_DUMP_ROM);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    timer.start();
    set_progressbar_value(0);

    mapdata.clear();
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

        uint32_t pagesize = numblocks * 32;
        pleft = (float)(addr - start_addr) / (float)length * 100.0f;
        set_progressbar_value(pleft);


        output[2] = numblocks >> 8;
        output[3] = numblocks >> 0;

        output[4] = curblock >> 8;
        output[5] = curblock >> 0;

        received = serial->write_serial_data_echo_check(output);
        //delay(10);
        // Receive map data, check and remove header // ADJUST THIS LATER //
        //received.clear();
        received = serial->read_serial_data(numblocks * (32 + 3), serial_read_short_timeout);
        qDebug() << "Read received:" << parse_message_to_hex(received);
        if (!received.length())
        {
            delay(500);
            received = serial->write_serial_data_echo_check(output);
            received = serial->read_serial_data(numblocks * (32 + 3), serial_read_short_timeout);
            qDebug() << "Read received:" << parse_message_to_hex(received);
            if (!received.length())
                return STATUS_ERROR;
        }
        for (uint32_t i = 0; i < numblocks; i++)
        {
            received.remove(0, 2);
            mapdata.append(received, 32);
            received.remove(0, 33);
        }
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
        send_log_window_message(msg, true, true);
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

/*******************************************************
 *  Read ROM 16bit CAN ECUs
 ******************************************************/
int EcuOperations::read_mem_32bit_can(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray pagedata;
    QByteArray mapdata;
    uint32_t cplen = 0;
    uint32_t timeout = 0;

    uint32_t pagesize = 0x400;

    uint32_t skip_start = start_addr & (pagesize - 1); //if unaligned, we'll be receiving this many extra bytes
    uint32_t addr = start_addr - skip_start;
    uint32_t willget = (skip_start + length + pagesize - 1) & ~(pagesize - 1);
    uint32_t len_done = 0;  //total data written to file

    // send 0xD8 command to kernel to dump the chunk from ROM
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    output.append((uint8_t)SID_START_COMM_CAN);
    output.append((uint8_t)(SID_DUMP_CAN + 0x06));
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

        uint32_t numblocks = length / pagesize;
        unsigned curspeed = 0, tleft;
        float pleft = 0;
        unsigned long chrono;

        //delay(1);
        numblocks = willget / pagesize;

        if (numblocks > NP10_MAXBLKS)
            numblocks = NP10_MAXBLKS;

        numblocks = 1;

        uint32_t curblock = (addr / pagesize);


        pleft = (float)(addr - start_addr) / (float)length * 100.0f;
        set_progressbar_value(pleft);

        //length = 256;

        output[6] = (uint8_t)((pagesize >> 24) & 0xFF);
        output[7] = (uint8_t)((pagesize >> 16) & 0xFF);
        output[8] = (uint8_t)((pagesize >> 8) & 0xFF);
        output[9] = (uint8_t)((addr >> 24) & 0xFF);
        output[10] = (uint8_t)((addr >> 16) & 0xFF);
        output[11] = (uint8_t)((addr >> 8) & 0xFF);
        serial->write_serial_data_echo_check(output);
        //qDebug() << "0xD8 message sent to kernel initiate dump";
        //delay(100);
        received = serial->read_serial_data(1, 10);
        //qDebug() << "Response to 0xD8 (dump mem) message:" << parse_message_to_hex(received);

        if ((uint8_t)received.at(0) != SID_START_COMM_CAN || ((uint8_t)received.at(1) & 0xF8) != SID_DUMP_CAN)
        {
            send_log_window_message("Page data request failed!", true, true);
            send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }

        timeout = 0;
        pagedata.clear();

        while ((uint32_t)pagedata.length() < pagesize && timeout < 1000)
        {
            received = serial->read_serial_data(1, 50);
            pagedata.append(received, 8);
            timeout++;
            //qDebug() << parse_message_to_hex(received);
        }
        if (timeout >= 1000)
        {
            send_log_window_message("Page data timeout!", true, true);
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
        send_log_window_message(msg, true, true);
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

    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*******************************************************
 *  Write ROM 16bit K-Line ECUs
 ******************************************************/
int EcuOperations::write_mem_16bit_kline(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
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
 *  Write ROM 32bit K-Line ECUs
 ******************************************************/
int EcuOperations::write_mem_32bit_kline(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{
    QByteArray filedata;

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

    if (get_changed_blocks_32bit_kline(data_array, block_modified))
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
                if (reflash_block_32bit_kline(&data_array[flashdevices[mcu_type_index].fblocks->start], &flashdevices[mcu_type_index], blockno, test_write))
                {
                    send_log_window_message("Block " + QString::number(blockno) + " reflash failed.", true, true);
                    return STATUS_ERROR;
                }
                else
                {
                    flashbytesindex += flashdevices[mcu_type_index].fblocks[blockno].len;
                    send_log_window_message("Block " + QString::number(blockno) + " reflash complete.", true, true);
                }
            }
        }

        send_log_window_message("--- comparing ECU flash memory pages to image file after reflash ---", true, true);
        send_log_window_message("seg\tstart\tlen\tsame?", true, true);

        if (get_changed_blocks_32bit_kline(data_array, block_modified))
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
 *  Write ROM 32bit CAN ECUs
 ******************************************************/
int EcuOperations::write_mem_32bit_can(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{
    QByteArray filedata;
    //QByteArray output;
    //QByteArray received;
    //QByteArray msg;
    //uint32_t start_addr = 0;
    //uint32_t length = 0;
    //uint32_t pagesize = 0x400;

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

    if (get_changed_blocks_32bit_can(data_array, block_modified))
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
                if (reflash_block_32bit_can(&data_array[flashdevices[mcu_type_index].fblocks->start], &flashdevices[mcu_type_index], blockno, test_write))
                {
                    send_log_window_message("Block " + QString::number(blockno) + " reflash failed.", true, true);
                    return STATUS_ERROR;
                }
                else
                {
                    flashbytesindex += flashdevices[mcu_type_index].fblocks[blockno].len;
                    send_log_window_message("Block " + QString::number(blockno) + " reflash complete.", true, true);
                }
            }
        }

        send_log_window_message("--- comparing ECU flash memory pages to image file after reflash ---", true, true);
        send_log_window_message("seg\tstart\tlen\tsame?", true, true);

        if (get_changed_blocks_32bit_can(data_array, block_modified))
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
int EcuOperations::get_changed_blocks_16bit_kline(const uint8_t *src, int *modified)
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
 *  Compare ROM 32bit K-Line ECUs
 ******************************************************/
int EcuOperations::get_changed_blocks_32bit_kline(const uint8_t *src, int *modified)
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
        if (check_romcrc_32bit_kline(&src[bs], bs, blen, &modified[blockno])) {
            return -1;
        }
    }
    return 0;
}

/*******************************************************
 *  Compare ROM 32bit CAN ECUs
 ******************************************************/
int EcuOperations::get_changed_blocks_32bit_can(const uint8_t *src, int *modified)
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
        if (check_romcrc_32bit_can(&src[bs], bs, blen, &modified[blockno])) {
            return -1;
        }
    }
    return 0;
}

/*******************************************************
 *  ROM CRC 16bit K-Line ECUs
 ******************************************************/
int EcuOperations::check_romcrc_16bit_kline(const uint8_t *src, uint32_t start, uint32_t len, int *modified)
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

/*******************************************************
 *  ROM CRC 32bit K-Line ECUs
 ******************************************************/
int EcuOperations::check_romcrc_32bit_kline(const uint8_t *src, uint32_t start, uint32_t len, int *modified)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint16_t chunko;

    len = (len + ROMCRC_LENMASK_32BIT) & ~ROMCRC_LENMASK_32BIT;

    chunko = start / ROMCRC_CHUNKSIZE_32BIT;

    //request format : <SID_CONF> <SID_CONF_CKS1> <CNH> <CNL> <CRC0H> <CRC0L> ...<CRC3H> <CRC3L>
    //verify if <CRCH:CRCL> hash is valid for n*256B chunk of the ROM (starting at <CNH:CNL> * 256)
    for (; len > 0; len -= ROMCRC_ITERSIZE_32BIT, chunko += ROMCRC_NUMCHUNKS_32BIT) {
        if (kill_process)
            return STATUS_ERROR;

        output.clear();
        output.append(SID_CONF);
        output.append(SID_CONF_CKS1);
        output.append(chunko >> 8);
        output.append(chunko & 0xFF);

        //fill the request with n*CRCs
        unsigned chunk_cnt;
        for (chunk_cnt = 0; chunk_cnt < ROMCRC_NUMCHUNKS_32BIT; chunk_cnt++) {
            uint16_t chunk_crc = crc16(src, ROMCRC_CHUNKSIZE_32BIT);
            src += ROMCRC_CHUNKSIZE_32BIT;
            output.append(chunk_crc >> 8);
            output.append(chunk_crc & 0xFF);
        }

        received = serial->write_serial_data_echo_check(output);

        //responses :	01 <SID_CONF+0x40> <cks> for good CRC
        //				03 7F <SID_CONF> <SID_CONF_CKS1_BADCKS> <cks> for bad CRC
        // anything else is an error that causes abort
        received = serial->read_serial_data(3, serial_read_short_timeout);

        if (received.at(1) == (char)(SID_CONF + 0x40))
        {
            continue;
        }

        received.append(serial->read_serial_data(2, serial_read_short_timeout));
        send_log_window_message("\tNO", false, true);

        if (received.at(2) != (char)(SID_CONF) && received.at(3) != (char)(SID_CONF_CKS1_BADCKS))
        {
            send_log_window_message(" ", false, true);
            send_log_window_message("got bad SID_FLASH_CKS1 response : ", true, true);
            goto badexit;
        }

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

/*******************************************************
 *  ROM CRC 32bit CAN ECUs
 ******************************************************/
int EcuOperations::check_romcrc_32bit_can(const uint8_t *src, uint32_t start_addr, uint32_t len, int *modified)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray pagedata;
    uint16_t chunko;
    uint32_t pagesize = ROMCRC_ITERSIZE_CAN;
    uint32_t byte_index = 0;

    len = (len + ROMCRC_LENMASK_CAN) & ~ROMCRC_LENMASK_CAN;

    chunko = start_addr / ROMCRC_CHUNKSIZE_CAN;

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    output.append((uint8_t)SID_START_COMM_CAN);
    output.append((uint8_t)(SID_CONF_CKS1_CAN + 0x06));
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    //request format : <SID_CONF> <SID_CONF_CKS1> <CNH> <CNL> <CRC0H> <CRC0L> ...<CRC3H> <CRC3L>
    //verify if <CRCH:CRCL> hash is valid for n*256B chunk of the ROM (starting at <CNH:CNL> * 256)
    for (; len > 0; len -= ROMCRC_ITERSIZE_CAN, chunko += ROMCRC_NUMCHUNKS_CAN) {
        if (kill_process)
            return STATUS_ERROR;

        output[6] = (uint8_t)((pagesize >> 24) & 0xFF);
        output[7] = (uint8_t)((pagesize >> 16) & 0xFF);
        output[8] = (uint8_t)((pagesize >> 8) & 0xFF);
        output[9] = (uint8_t)((start_addr >> 24) & 0xFF);
        output[10] = (uint8_t)((start_addr >> 16) & 0xFF);
        output[11] = (uint8_t)((start_addr >> 8) & 0xFF);
        //qDebug() << "Send req:" << parse_message_to_hex(output);
        start_addr += pagesize;

        received = serial->write_serial_data_echo_check(output);
        //delay(100);
        received = serial->read_serial_data(1, serial_read_short_timeout);
        //received.remove(0, 4);
        //qDebug() << "Received:" << parse_message_to_hex(received);

        uint16_t chk_sum = 0;
        for (uint32_t j = 0; j < pagesize; j++) {
            pagedata[j] = src[(byte_index * pagesize) + j];
            chk_sum += (pagedata[j] & 0xFF);
            chk_sum = ((chk_sum >> 8) & 0xFF) + (chk_sum & 0xFF);
        }
        byte_index++;

        //qDebug() << "Checksums: File =" << hex << chk_sum << "ROM =" << hex << (uint8_t)received.at(2);
        if ((uint8_t)received.at(0) != SID_START_COMM_CAN || ((uint8_t)received.at(1) & 0xF8) != SID_CONF_CKS1_CAN || chk_sum == (uint8_t)received.at(2))
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

/*******************************************************
 *  NPK Raw flash ROM 16bit K-Line ECUs
 ******************************************************/
int EcuOperations::npk_raw_flashblock_16bit_kline(const uint8_t *src, uint32_t start, uint32_t len)
{

    // program 128-byte chunks //
    uint32_t remain = len;
    uint32_t byteindex = flashbytesindex;
    uint8_t blocksize = 128;

    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray chksum_data;

    unsigned long chrono;
    unsigned curspeed, tleft;

    if ((len & (blocksize - 1)) ||
        (start & (blocksize - 1))) {
        send_log_window_message("error: misaligned start / length!", true, true);
        return -1;
    }


    timer.start();
    while (remain) {
        if (kill_process)
            return STATUS_ERROR;

        //delay(1);
        chksum_data.clear();
        output.clear();
        chksum_data.append(start >> 16);
        chksum_data.append(start >> 8);
        chksum_data.append(start >> 0);
        for (unsigned i = start; i < (start + blocksize); i++)
        {
            chksum_data.append(src[i]);
        }
        chksum_data.append(cks_add8(chksum_data, 131));
        output.append(SID_FLASH);
        output.append(SIDFL_WB);
        output.append(chksum_data);

        received = serial->write_serial_data_echo_check(output);

        received = serial->read_serial_data(3, serial_read_medium_timeout);
        if (received.length() <= 1) {
            send_log_window_message("npk_raw_flashblock: no response @ " + QString::number((unsigned) start), true, true);
            return -1;
        }
        if (received.length() < 3) {
            send_log_window_message("npk_raw_flashblock: incomplete response @ " + QString::number((unsigned) start), true, true);
            return -1;
        }

        if ((uint8_t)received.at(1) != (SID_FLASH + 0x40))
        {
            //maybe negative response, if so, get the remaining packet
            send_log_window_message("npk_raw_flashblock: bad response @ " + QString::number((unsigned) start), true, true);

            int needed = 1 + (uint8_t)received.at(0) - received.length();
            if (needed > 0) {
                received.append(serial->read_serial_data(needed, serial_read_medium_timeout));
            }
            send_log_window_message("npk_raw_flashblock: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }

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

        tleft = remain / curspeed;  //s
        if (tleft > 9999) {
            tleft = 9999;
        }
        tleft++;

        float pleft = (float)(byteindex - start) / (float)flashbytescount * 100.0f;
        set_progressbar_value(pleft);

        QString start_address = QString("%1").arg(start,8,16,QLatin1Char('0')).toUpper();
        msg = QString("writing chunk @ 0x%1 (%2\% - %3 B/s, ~ %4 s remaining)").arg(start_address).arg((unsigned) 100 * (len - remain) / len,1,10,QLatin1Char('0')).arg((uint32_t)curspeed,1,10,QLatin1Char('0')).arg(tleft,1,10,QLatin1Char('0')).toUtf8();
        send_log_window_message(msg, true, true);

    }   //while len

    send_log_window_message("npk_raw_flashblock: write complete.", true, true);

    received = serial->read_serial_data(100, serial_read_short_timeout);

    return 0;
}

/*******************************************************
 *  NPK Raw flash ROM 32bit K-Line ECUs
 ******************************************************/
int EcuOperations::npk_raw_flashblock_32bit_kline(const uint8_t *src, uint32_t start, uint32_t len)
{

    // program 128-byte chunks //
    uint32_t remain = len;
    uint32_t byteindex = flashbytesindex;
    uint8_t blocksize = 128;

    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray chksum_data;

    unsigned long chrono;
    unsigned curspeed, tleft;

    if ((len & (blocksize - 1)) ||
        (start & (blocksize - 1))) {
        send_log_window_message("error: misaligned start / length!", true, true);
        return -1;
    }


    timer.start();
    while (remain) {
        if (kill_process)
            return STATUS_ERROR;

        //delay(1);
        chksum_data.clear();
        output.clear();
        chksum_data.append(start >> 16);
        chksum_data.append(start >> 8);
        chksum_data.append(start >> 0);
        for (unsigned i = start; i < (start + blocksize); i++)
        {
            chksum_data.append(src[i]);
        }
        chksum_data.append(cks_add8(chksum_data, 131));
        output.append(SID_FLASH);
        output.append(SIDFL_WB);
        output.append(chksum_data);

        received = serial->write_serial_data_echo_check(output);

        received = serial->read_serial_data(3, serial_read_medium_timeout);
        if (received.length() <= 1) {
            send_log_window_message("npk_raw_flashblock: no response @ " + QString::number((unsigned) start), true, true);
            return -1;
        }
        if (received.length() < 3) {
            send_log_window_message("npk_raw_flashblock: incomplete response @ " + QString::number((unsigned) start), true, true);
            return -1;
        }

        if ((uint8_t)received.at(1) != (SID_FLASH + 0x40))
        {
            //maybe negative response, if so, get the remaining packet
            send_log_window_message("npk_raw_flashblock: bad response @ " + QString::number((unsigned) start), true, true);

            int needed = 1 + (uint8_t)received.at(0) - received.length();
            if (needed > 0) {
                received.append(serial->read_serial_data(needed, serial_read_medium_timeout));
            }
            send_log_window_message("npk_raw_flashblock: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }

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

        tleft = remain / curspeed;  //s
        if (tleft > 9999) {
            tleft = 9999;
        }
        tleft++;

        float pleft = (float)(byteindex - start) / (float)flashbytescount * 100.0f;
        set_progressbar_value(pleft);

        QString start_address = QString("%1").arg(start,8,16,QLatin1Char('0')).toUpper();
        msg = QString("writing chunk @ 0x%1 (%2\% - %3 B/s, ~ %4 s remaining)").arg(start_address).arg((unsigned) 100 * (len - remain) / len,1,10,QLatin1Char('0')).arg((uint32_t)curspeed,1,10,QLatin1Char('0')).arg(tleft,1,10,QLatin1Char('0')).toUtf8();
        send_log_window_message(msg, true, true);

    }   //while len

    send_log_window_message("npk_raw_flashblock: write complete.", true, true);

    received = serial->read_serial_data(100, serial_read_short_timeout);

    return 0;
}

/*******************************************************
 *  NPK Raw flash ROM 32bit CAN ECUs
 ******************************************************/
int EcuOperations::npk_raw_flashblock_32bit_can(const uint8_t *src, uint32_t start, uint32_t len)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    uint32_t remain = len;
    uint32_t block_start = start;
    uint32_t block_len = len;
    uint32_t byteindex = flashbytesindex;
    uint8_t blocksize = 128;
    uint16_t chk_sum;

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
            for (int k = 0; k < 8; k++){
                output[k + 4] = (uint8_t)(src[byte_index + k] & 0xFF);
                chk_sum += (output[k + 4] & 0xFF);
                chk_sum = ((chk_sum >> 8) & 0xFF) + (chk_sum & 0xFF);
            }
            byte_index += 8;
            received = serial->write_serial_data_echo_check(output);
            //qDebug() << "Flash data message sent to kernel for block (128 byte block, 8 byte block): " + QString::number(i) + "/" + QString::number(num_128_byte_blocks) + ", " + QString::number(j);
            //qDebug() << parse_message_to_hex(output);
            //delay(5);
        }

        // send 0xF8 command to check and flash 128 bytes
        output[4] = (uint8_t)SID_START_COMM_CAN;
        output[5] = (uint8_t)(SIDFL_WB_CAN + 0x03);
        output[6] = (uint8_t)((i >> 8) & 0xFF);
        output[7] = (uint8_t)(i & 0xFF);
        output[8] = (uint8_t)(chk_sum & 0xFF);
        received = serial->write_serial_data_echo_check(output);
        //qDebug() << "0xF8 command sent to kernel to check and flash 128 byte block";
        //qDebug() << parse_message_to_hex(output) << chk_sum;
        ////delay(50);

        // check for flash success or not
        received = serial->read_serial_data(3, serial_read_long_timeout);
        //qDebug() << parse_message_to_hex(received);
        if((uint8_t)received.at(0) != SID_START_COMM_CAN || ((uint8_t)received.at(1) & 0xF8) != SIDFL_WB_CAN)
        {
            qDebug() << "Flashing of 128 byte block unsuccessful, stopping";
            qDebug() << hex << num_128_byte_blocks << "/" << (i & 0xFFFF);
            //return STATUS_ERROR;
        }
        else
        {
            //qDebug() << "Flashing of 128 byte block successful, proceeding to next 128 byte block";
            //qDebug() << hex << num_128_byte_blocks << "/" << (i & 0xFFFF);
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

        tleft = remain / curspeed;  //s
        if (tleft > 9999) {
            tleft = 9999;
        }
        tleft++;

        float pleft = (float)(byteindex - start) / (float)flashbytescount * 100.0f;
        set_progressbar_value(pleft);

        QString start_address = QString("%1").arg(start,8,16,QLatin1Char('0'));
        msg = QString("writing chunk @ 0x%1 (%2\% - %3 B/s, ~ %4 s remaining)").arg(start_address).arg((unsigned) 100 * (len - remain) / len,1,10,QLatin1Char('0')).arg((uint32_t)curspeed,1,10,QLatin1Char('0')).arg(tleft,1,10,QLatin1Char('0')).toUtf8();
        send_log_window_message(msg, true, true);

    }

    return 0;
}

/*******************************************************
 *  Reflash ROM 16bit K-Line ECUs
 ******************************************************/
int EcuOperations::reflash_block_16bit_kline(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write)
{
    int errval;

    uint32_t start;
    uint32_t len;

    QByteArray output;
    QByteArray received;
    QByteArray msg;

    if (blockno >= fdt->numblocks) {
        send_log_window_message("block " + QString::number(blockno) + " out of range !", true, true);
        return -1;
    }

    start = fdt->fblocks[blockno].start;
    len = fdt->fblocks[blockno].len;

    QString start_addr = QString("%1").arg((uint32_t)start,8,16,QLatin1Char('0')).toUpper();
    QString length = QString("%1").arg((uint32_t)len,8,16,QLatin1Char('0')).toUpper();
    msg = QString("Flash block addr: 0x" + start_addr + " len: 0x" + length).toUtf8();
    send_log_window_message(msg, true, true);

    // 1- requestdownload //
    output.append(SID_FLREQ);
    //received.clear();
    received = serial->write_serial_data_echo_check(output);

    //received.clear();
    received = serial->read_serial_data(8, serial_read_short_timeout);
    if (!received.length())
    {
        send_log_window_message("no 'RequestDownload' response", true, true);
        return STATUS_ERROR;
    }
    if ((uint8_t)received.at(1) != (SID_FLREQ + 0x40))
    {
        send_log_window_message("got bad RequestDownload response", true, true);//;
        send_log_window_message("SID_FLREQ: " + parse_message_to_hex(received), true, true);
        output.clear();
        output.append(SID_CONF_LASTERR);
        received = serial->write_serial_data_echo_check(output);
        received = serial->read_serial_data(8, serial_read_short_timeout);
        send_log_window_message("SID_FLREQ failed with errcode", true, true);//;
        send_log_window_message("SID_CONF_LASTERR: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    // 2- Unprotect maybe //
    output.clear();
    if (!test_write)
    {
        output.append(SID_FLASH);
        output.append(SIDFL_UNPROTECT);
        output.append(~SIDFL_UNPROTECT);
        received = serial->write_serial_data_echo_check(output);

        //received.clear();
        received = serial->read_serial_data(3, serial_read_medium_timeout);
        if (!received.length())
        {
            send_log_window_message("no 'unprotect' response", true, true);
            return STATUS_ERROR;
        }
        if ((uint8_t)received.at(1) != (SID_FLASH + 0x40)) {
            send_log_window_message("got bad Unprotect response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
        send_log_window_message("SID_UNPROTECT: " + parse_message_to_hex(received), true, true);
    }
    else
        send_log_window_message("ECU write in test_write mode, no real write processed.", true, true);

    // 3- erase block //
    msg = QString("Erasing block %1 (0x%2-0x%3)...").arg((uint8_t)blockno,2,16,QLatin1Char('0')).arg((uint8_t)(unsigned) start,8,16,QLatin1Char('0')).arg((uint32_t)(unsigned) start + len - 1,8,16,QLatin1Char('0')).toUtf8();
    output.clear();
    output.append(SID_FLASH);
    output.append(SIDFL_EB);
    output.append(blockno);
    received = serial->write_serial_data_echo_check(output);
    //received = serial->read_serial_data(1, serial_read_short_timeout);

    send_log_window_message("Erase block: " + QString::number(blockno), true, true);

    received.clear();
    //delay(2000);
    //received = serial->read_serial_data(3, serial_read_short_timeout);

    QTime dieTime = QTime::currentTime().addMSecs(serial_read_extra_long_timeout);
    while ((uint32_t)received.length() < 3 && (QTime::currentTime() < dieTime))
    {
        received = serial->read_serial_data(3, serial_read_short_timeout);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        delay(100);
    }

    send_log_window_message("SID_FLASH | SIDFL_EB: " + parse_message_to_hex(received), true, true);
    if (!received.length())
    {
        send_log_window_message("no 'ERASE_BLOCK' response", true, true);
        return STATUS_ERROR;
    }
    if ((uint8_t)received.at(1) != (SID_FLASH + 0x40))
    {
        send_log_window_message("got bad ERASE_BLOCK response : ", true, true);
        send_log_window_message("SIDFL_EB: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    // 4- write //
    errval = npk_raw_flashblock_32bit_kline(newdata, start, len);
    if (errval) {
        send_log_window_message("Reflash error! Do not panic, do not reset the ECU immediately. The kernel is most likely still running and receiving commands!", true, true);
        return STATUS_ERROR;
    }

    send_log_window_message("Flash block ok", true, true);

    return STATUS_SUCCESS;

}

/*******************************************************
 *  Reflash ROM 32bit K-Line ECUs
 ******************************************************/
int EcuOperations::reflash_block_32bit_kline(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write)
{
    int errval;

    uint32_t start;
    uint32_t len;

    QByteArray output;
    QByteArray received;
    QByteArray msg;

    if (blockno >= fdt->numblocks) {
        send_log_window_message("block " + QString::number(blockno) + " out of range !", true, true);
        return -1;
    }

    start = fdt->fblocks[blockno].start;
    len = fdt->fblocks[blockno].len;

    QString start_addr = QString("%1").arg((uint32_t)start,8,16,QLatin1Char('0')).toUpper();
    QString length = QString("%1").arg((uint32_t)len,8,16,QLatin1Char('0')).toUpper();
    msg = QString("Flash block addr: 0x" + start_addr + " len: 0x" + length).toUtf8();
    send_log_window_message(msg, true, true);

    // 1- requestdownload //
    output.append(SID_FLREQ);
    //received.clear();
    received = serial->write_serial_data_echo_check(output);

    //received.clear();
    received = serial->read_serial_data(8, serial_read_short_timeout);
    if (!received.length())
    {
        send_log_window_message("no 'RequestDownload' response", true, true);
        return STATUS_ERROR;
    }
    if ((uint8_t)received.at(1) != (SID_FLREQ + 0x40))
    {
        send_log_window_message("got bad RequestDownload response", true, true);//;
        send_log_window_message("SID_FLREQ: " + parse_message_to_hex(received), true, true);
        output.clear();
        output.append(SID_CONF_LASTERR);
        received = serial->write_serial_data_echo_check(output);
        received = serial->read_serial_data(8, serial_read_short_timeout);
        send_log_window_message("SID_FLREQ failed with errcode", true, true);//;
        send_log_window_message("SID_CONF_LASTERR: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    // 2- Unprotect maybe //
    output.clear();
    if (!test_write)
    {
        output.append(SID_FLASH);
        output.append(SIDFL_UNPROTECT);
        output.append(~SIDFL_UNPROTECT);
        received = serial->write_serial_data_echo_check(output);

        //received.clear();
        received = serial->read_serial_data(3, serial_read_medium_timeout);
        if (!received.length())
        {
            send_log_window_message("no 'unprotect' response", true, true);
            return STATUS_ERROR;
        }
        if ((uint8_t)received.at(1) != (SID_FLASH + 0x40)) {
            send_log_window_message("got bad Unprotect response: " + parse_message_to_hex(received), true, true);
            return STATUS_ERROR;
        }
        send_log_window_message("SID_UNPROTECT: " + parse_message_to_hex(received), true, true);
    }
    else
        send_log_window_message("ECU write in test_write mode, no real write processed.", true, true);

    // 3- erase block //
    msg = QString("Erasing block %1 (0x%2-0x%3)...").arg((uint8_t)blockno,2,16,QLatin1Char('0')).arg((uint8_t)(unsigned) start,8,16,QLatin1Char('0')).arg((uint32_t)(unsigned) start + len - 1,8,16,QLatin1Char('0')).toUtf8();
    output.clear();
    output.append(SID_FLASH);
    output.append(SIDFL_EB);
    output.append(blockno);
    received = serial->write_serial_data_echo_check(output);
    //received = serial->read_serial_data(1, serial_read_short_timeout);

    send_log_window_message("Erase block: " + QString::number(blockno), true, true);

    received.clear();
    //delay(2000);
    //received = serial->read_serial_data(3, serial_read_short_timeout);

    QTime dieTime = QTime::currentTime().addMSecs(serial_read_extra_long_timeout);
    while ((uint32_t)received.length() < 3 && (QTime::currentTime() < dieTime))
    {
        received = serial->read_serial_data(3, serial_read_short_timeout);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        delay(100);
    }

    send_log_window_message("SID_FLASH | SIDFL_EB: " + parse_message_to_hex(received), true, true);
    if (!received.length())
    {
        send_log_window_message("no 'ERASE_BLOCK' response", true, true);
        return STATUS_ERROR;
    }
    if ((uint8_t)received.at(1) != (SID_FLASH + 0x40))
    {
        send_log_window_message("got bad ERASE_BLOCK response : ", true, true);
        send_log_window_message("SIDFL_EB: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    // 4- write //
    errval = npk_raw_flashblock_32bit_kline(newdata, start, len);
    if (errval) {
        send_log_window_message("Reflash error! Do not panic, do not reset the ECU immediately. The kernel is most likely still running and receiving commands!", true, true);
        return STATUS_ERROR;
    }

    send_log_window_message("Flash block ok", true, true);

    return STATUS_SUCCESS;

}

/*******************************************************
 *  Reflash ROM 32bit CAN ECUs
 ******************************************************/
int EcuOperations::reflash_block_32bit_can(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write)
{
    int errval;

    uint32_t block_start;
    uint32_t block_len;

    QByteArray output;
    QByteArray received;
    QByteArray msg;

    if (blockno >= fdt->numblocks) {
        send_log_window_message("block " + QString::number(blockno) + " out of range !", true, true);
        return -1;
    }

    block_start = fdt->fblocks[blockno].start;
    block_len = fdt->fblocks[blockno].len;

    QString start_addr = QString("%1").arg((uint32_t)block_start,8,16,QLatin1Char('0')).toUpper();
    QString length = QString("%1").arg((uint32_t)block_len,8,16,QLatin1Char('0')).toUpper();
    msg = QString("Flash block addr: 0x" + start_addr + " len: 0x" + length).toUtf8();
    send_log_window_message(msg, true, true);

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    output.append((uint8_t)SID_START_COMM_CAN);
    output.append((uint8_t)(SID_FLASH_CAN + 0x01));
    if (test_write)
        output.append((uint8_t)SIDFL_PROTECT_CAN);
    else
        output.append((uint8_t)SIDFL_UNPROTECT_CAN);
    output.append((uint8_t)(0x00));
    output.append((uint8_t)(0x00));
    output.append((uint8_t)(0x00));
    output.append((uint8_t)(0x00));
    output.append((uint8_t)(0x00));
    received = serial->write_serial_data_echo_check(output);
    qDebug() << parse_message_to_hex(output);
    qDebug() << "0xE0 message sent to kernel to initialize erasing / flashing microcodes";
    delay(200);
    received = serial->read_serial_data(3, serial_read_short_timeout);
    qDebug() << parse_message_to_hex(received);

    if((uint8_t)received.at(0) != SID_START_COMM_CAN || ((uint8_t)received.at(1) & 0xF8) != SID_FLASH_CAN)
    {
        qDebug() << "Initialize of erasing / flashing microcodes failed!";
        return STATUS_ERROR;
    }


    int num_128_byte_blocks = (block_len >> 7) & 0xFFFFFFFF;

    qDebug() << "Proceeding to attempt erase and flash of block number: " << blockno;
    output[5] = (uint8_t)(SIDFL_EB_CAN + 0x06);
    output[6] = (uint8_t)(blockno & 0xFF);
    output[7] = (uint8_t)((block_start >> 24) & 0xFF);
    output[8] = (uint8_t)((block_start >> 16) & 0xFF);
    output[9] = (uint8_t)((block_start >> 8) & 0xFF);
    output[10] = (uint8_t)((num_128_byte_blocks >> 8) & 0xFF);
    output[11] = (uint8_t)(num_128_byte_blocks & 0xFF);
    received = serial->write_serial_data_echo_check(output);
    qDebug() << parse_message_to_hex(output);
    //send_log_window_message("0xF0 message sent to kernel to erase block number: " + QString::number(blockno), true, true);
    qDebug() << "0xF0 message sent to kernel to erase block number: " << blockno;
    delay(500);

    QTime dieTime = QTime::currentTime().addMSecs(serial_read_extra_long_timeout);
    while ((uint32_t)received.length() < 3 && (QTime::currentTime() < dieTime))
    {
        received = serial->read_serial_data(3, serial_read_short_timeout);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        delay(100);
    }

    //send_log_window_message(parse_message_to_hex(received), true, true);
    qDebug() << parse_message_to_hex(received);

    if((uint8_t)received.at(0) != SID_START_COMM_CAN || ((uint8_t)received.at(1) & 0xF8) != SIDFL_EB_CAN)
    {
        qDebug() << "Not ready for 128byte block writing";
        return STATUS_ERROR;
    }

    errval = npk_raw_flashblock_32bit_can(newdata, block_start, block_len);
    if (errval) {
        send_log_window_message("Reflash error! Do not panic, do not reset the ECU immediately. The kernel is most likely still running and receiving commands!", true, true);
        return STATUS_ERROR;
    }


    return STATUS_SUCCESS;
}

/*******************************************************
 *
 *  Hitachi ECU operations
 *
 ******************************************************/
int EcuOperations::read_mem_uj20_30_40_70_kline(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t start_addr, uint32_t length)
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
    output.append((uint8_t)(SID_HITACHI_BLOCK_READ & 0xFF));
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

        qDebug() << "Received map data:" << parse_message_to_hex(received);
        if (received.startsWith("\x80\xf0"))
        {
            received.remove(0, 5);
            received.remove(received.length() - 1, 1);
            mapdata.append(received);
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
        msg = QString("ROM read addr:  0x%1  length:  0x%2,  %3  B/s  %4 s remaining").arg(start_address).arg(block_len).arg(curspeed, 6, 10, QLatin1Char(' ')).arg(tleft, 6, 10, QLatin1Char(' ')).toUtf8();
        send_log_window_message(msg, true, true);
        qDebug() << msg;
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
 *
 *  Generic subroutines
 *
 ******************************************************/
QString EcuOperations::parse_message_to_hex(QByteArray received)
{
    QByteArray msg;

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

void EcuOperations::send_log_window_message(QString message, bool timestamp, bool linefeed)
{
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']  ");

    if (timestamp)
        message = dateTimeString + message;
    if (linefeed)
        message = message + "\n";

    QTextEdit* textedit = flash_window->findChild<QTextEdit*>("text_edit");
    if (textedit)
    {
        textedit->insertPlainText(message);
        textedit->ensureCursorVisible();
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
}

void EcuOperations::set_progressbar_value(int value)
{
    QProgressBar* progressbar = flash_window->findChild<QProgressBar*>("progressbar");
    if (progressbar)
    {
        progressbar->setValue(value);
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void EcuOperations::delay(int n)
{
    QTime dieTime = QTime::currentTime().addMSecs(n);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

/* special checksum for reflash blocks:
 * "one's complement" checksum; if adding causes a carry, add 1 to sum. Slightly better than simple 8bit sum
 */
uint8_t EcuOperations::cks_add8(QByteArray chksum_data, unsigned len)
{
    uint16_t sum = 0;
    uint8_t data[chksum_data.length()];

    for (int i = 0; i < chksum_data.length(); i++)
    {
        data[i] = chksum_data.at(i);
    }

    for (unsigned i = 0; i < len; i++) {
        sum += data[i];
        if (sum & 0x100) {
            sum += 1;
        }
        sum = (uint8_t) sum;
    }
    return sum;
}

/*** CRC16 implementation adapted from Lammert Bies
 * https://www.lammertbies.nl/comm/info/crc-calculation.html
 *
 *
 */
#define NPK_CRC16   0xBAAD  //koopman, 2048bits (256B)
static bool crc_tab16_init = 0;
static uint16_t crc_tab16[256];

void EcuOperations::init_crc16_tab(void)
{
    uint32_t i, j;
    uint16_t crc, c;

    for (i=0; i<256; i++) {
        crc = 0;
        c   = (uint16_t) i;

        for (j=0; j<8; j++) {
            if ( (crc ^ c) & 0x0001 ) {
                crc = ( crc >> 1 ) ^ NPK_CRC16;
            } else {
                crc =   crc >> 1;
            }
            c = c >> 1;
        }
        crc_tab16[i] = crc;
    }

    crc_tab16_init = 1;

}

uint16_t EcuOperations::crc16(const uint8_t *data, uint32_t siz)
{
    uint16_t crc;

    if (!crc_tab16_init) {
        init_crc16_tab();
    }

    crc = 0;

    while (siz > 0) {
        uint16_t tmp;
        uint8_t nextval;

        nextval = *data++;
        tmp =  crc ^ nextval;
        crc = (crc >> 8) ^ crc_tab16[ tmp & 0xff ];
        siz -= 1;
    }
    return crc;
}

uint8_t EcuOperations::calculate_checksum(QByteArray output, bool dec_0x100)
{
    uint8_t checksum = 0;

    for (uint16_t i = 0; i < output.length(); i++)
    {
        checksum += (uint8_t)output.at(i);
    }
    if (dec_0x100)
        checksum = (uint8_t) (0x100 - checksum);

    return checksum;
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

unsigned int EcuOperations::crc32(const unsigned char *buf, unsigned int len)
{
    unsigned int crc = 0xFFFFFFFF;
    if (buf == NULL)
        return 0L;
    while (len--)
        crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);

    return crc ^ 0xFFFFFFFF;
}

int EcuOperations::byte_to_int32(unsigned char *data)
{
    return (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
}

int EcuOperations::byte_to_int24(unsigned char *data)
{
    return (data[0] << 16) + (data[1] << 8) + data[2];
}

int EcuOperations::byte_to_int16(unsigned char *data)
{
    return (data[0] << 8) + data[1];
}

void EcuOperations::int16_to_byte(unsigned char *data,int i)
{
    data[0] = i >> 8;
    data[1] = i & 0xFF;
}

void EcuOperations::int24_to_byte(unsigned char *data,int i)
{
    data[0] = i >> 16;
    data[1] = (i >> 8) & 0xFF;
    data[2] = i & 0xFF;
}

void EcuOperations::int32_to_byte(unsigned char *data,int i)
{
    data[0] = i >> 24;
    data[1] = (i >> 16) & 0xFF;
    data[2] = (i >> 8) & 0xFF;
    data[3] = i & 0xFF;
}


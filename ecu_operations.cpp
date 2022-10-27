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

QByteArray EcuOperations::request_kernel_id()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    uint8_t chk_sum = 0;

    if (mcu_type_string == "68HC16Y5")
    {
        request_denso_kernel_id = true;

        output.clear();
        output.append((uint8_t)((SID_OE_START_COMM >> 8) & 0xFF));
        output.append((uint8_t)(SID_OE_START_COMM & 0xFF));
        output.append((uint8_t)0x00 & 0xFF);
        output.append((uint8_t)0x01 & 0xFF);
        output.append((uint8_t)(SID_OE_RECUID & 0xFF));

        chk_sum = calculate_checksum(output, false);
        output.append((uint8_t) chk_sum);
        received = serial->write_serial_data_echo_check(output);
        delay(100);
        received = serial->read_serial_data(100, serial_read_short_timeout);

        request_denso_kernel_id = false;
    }
    if (mcu_type_string == "SH7055" || mcu_type_string == "SH7058")
    {
        request_denso_kernel_id = true;

        output.clear();
        output.append(SID_RECUID);
        serial->write_serial_data_echo_check(output);
        delay(100);
        received = serial->read_serial_data(100, serial_read_short_timeout);

        request_denso_kernel_id = false;
    }

    return received;
}

int EcuOperations::read_mem_16bit(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t start_addr, uint32_t length)
{

    return 0;
}

int EcuOperations::read_mem_32bit(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray mapdata;
    uint32_t cplen = 0;

    if (start_addr == 0 && length == 0)
    {
        start_addr = 0;
        length = 0x080000;
    }

    uint32_t skip_start = start_addr & (32 - 1); //if unaligned, we'll be receiving this many extra bytes
    uint32_t addr = start_addr - skip_start;
    uint32_t willget = (skip_start + length + 31) & ~(32 - 1);
    uint32_t len_done = 0;  //total data written to file

    #define NP10_MAXBLKS    32   //# of blocks to request per loop. Too high might flood us

    output.append(SID_DUMP);
    output.append(SID_DUMP_ROM);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    timer.start();

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

        if (numblocks > NP10_MAXBLKS) {
            numblocks = NP10_MAXBLKS;                           //ceil

        }

        uint32_t curblock = (addr / 32);

        uint32_t pagesize = numblocks * 32;
        pleft = (float)addr / (float)length * 100.0f;
        set_progressbar_value(pleft);


        output[2] = numblocks >> 8;
        output[3] = numblocks >> 0;

        output[4] = curblock >> 8;
        output[5] = curblock >> 0;

        received = serial->write_serial_data_echo_check(output);
        //delay(10);
        // Receive map data, check and remove header // ADJUST THIS LATER //
        received.clear();
        received = serial->read_serial_data(numblocks * (32 + 3), serial_read_short_timeout);
        if (!received.length())
            return STATUS_ERROR;
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

int EcuOperations::write_mem_32bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{
    //QString filename;
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

    if (get_changed_blocks(data_array, NULL, block_modified))
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
                if (reflash_block(&data_array[flashdevices[mcu_type_index].fblocks->start], &flashdevices[mcu_type_index], blockno, test_write))
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

        if (get_changed_blocks(data_array, NULL, block_modified))
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

int EcuOperations::get_changed_blocks(const uint8_t *src, const uint8_t *orig, int *modified)
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
        qDebug() << msg;
        send_log_window_message(msg, true, false);
        // do CRC comparison with ECU //
        if (check_romcrc(&src[bs], bs, blen, &modified[blockno])) {
            return -1;
        }
    }
    return 0;
}

#define ROMCRC_NUMCHUNKS 4
#define ROMCRC_CHUNKSIZE 256
#define ROMCRC_ITERSIZE (ROMCRC_NUMCHUNKS * ROMCRC_CHUNKSIZE)
#define ROMCRC_LENMASK ((ROMCRC_NUMCHUNKS * ROMCRC_CHUNKSIZE) - 1)  //should look like 0x3FF
int EcuOperations::check_romcrc(const uint8_t *src, uint32_t start, uint32_t len, int *modified)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint16_t chunko;

    len = (len + ROMCRC_LENMASK) & ~ROMCRC_LENMASK;

    chunko = start / ROMCRC_CHUNKSIZE;

    //request format : <SID_CONF> <SID_CONF_CKS1> <CNH> <CNL> <CRC0H> <CRC0L> ...<CRC3H> <CRC3L>
    //verify if <CRCH:CRCL> hash is valid for n*256B chunk of the ROM (starting at <CNH:CNL> * 256)
    for (; len > 0; len -= ROMCRC_ITERSIZE, chunko += ROMCRC_NUMCHUNKS) {
        if (kill_process)
            return STATUS_ERROR;

        output.clear();
        output.append(SID_CONF);
        output.append(SID_CONF_CKS1);
        output.append(chunko >> 8);
        output.append(chunko & 0xFF);

        //fill the request with n*CRCs
        unsigned chunk_cnt;
        for (chunk_cnt = 0; chunk_cnt < ROMCRC_NUMCHUNKS; chunk_cnt++) {
            uint16_t chunk_crc = crc16(src, ROMCRC_CHUNKSIZE);
            src += ROMCRC_CHUNKSIZE;
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

/* ret 0 if ok. For use by reflash_block(),
 * assumes parameters have been validated,
 * and appropriate block has been erased
 */

int EcuOperations::npk_raw_flashblock(const uint8_t *src, uint32_t start, uint32_t len) {

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

    if ((len & (blocksize - 1)) ||
        (start & (blocksize - 1))) {
        send_log_window_message("error: misaligned start / length!", true, true);
        return -1;
    }


    timer.start();
    while (remain) {
        if (kill_process)
            return STATUS_ERROR;

        unsigned curspeed, tleft;

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

        float pleft = (float)byteindex / (float)flashbytescount * 100.0f;
        set_progressbar_value(pleft);

        QString start_address = QString("0x%1").arg(start,8,16,QLatin1Char('0')).toUpper();
        msg = QString("writing chunk @ 0x%1 (%2\% - %3 B/s, ~ %4 s remaining)").arg(start_address).arg((unsigned) 100 * (len - remain) / len,1,10,QLatin1Char('0')).arg((uint32_t)curspeed,1,10,QLatin1Char('0')).arg(tleft,1,10,QLatin1Char('0')).toUtf8();
        send_log_window_message(msg, true, true);

    }   //while len
    send_log_window_message("npk_raw_flashblock: write complete.", true, true);

    received = serial->read_serial_data(100, serial_read_short_timeout);

    return 0;
}

int EcuOperations::reflash_block(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write) {
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
    errval = npk_raw_flashblock(newdata, start, len);
    if (errval) {
        send_log_window_message("Reflash error ! Do not panic, do not reset the ECU immediately. The kernel is most likely still running and receiving commands !", true, true);
        return STATUS_ERROR;
    }

    send_log_window_message("Flash block ok", true, true);

    return STATUS_SUCCESS;

}

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
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
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


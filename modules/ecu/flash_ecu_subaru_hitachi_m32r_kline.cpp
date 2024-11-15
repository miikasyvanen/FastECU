#include "flash_ecu_subaru_hitachi_m32r_kline.h"

FlashEcuSubaruHitachiM32rKline::FlashEcuSubaruHitachiM32rKline(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EcuOperationsWindow)
    , ecuCalDef(ecuCalDef)
    , cmd_type(cmd_type)
{
    ui->setupUi(this);

    connect(this, SIGNAL(LOG_E(QString,bool,bool)), parent, SLOT(logger(QString,bool,bool)));
    connect(this, SIGNAL(LOG_W(QString,bool,bool)), parent, SLOT(logger(QString,bool,bool)));
    connect(this, SIGNAL(LOG_I(QString,bool,bool)), parent, SLOT(logger(QString,bool,bool)));
    connect(this, SIGNAL(LOG_D(QString,bool,bool)), parent, SLOT(logger(QString,bool,bool)));

    if (cmd_type == "test_write")
        this->setWindowTitle("Test write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "write")
        this->setWindowTitle("Write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "read")
        this->setWindowTitle("Read ROM from TCU");

    this->serial = serial;
}

void FlashEcuSubaruHitachiM32rKline::run()
{
    this->show();

    int result = STATUS_ERROR;
    set_progressbar_value(0);

    //result = init_flash_hitachi_kline();

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

    serial->set_is_iso14230_connection(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(false);
    serial->set_is_29_bit_id(false);
    tester_id = 0xF0;
    target_id = 0x10;
    // Open serial port
    serial->open_serial_port();
    serial->change_port_speed("4800");
    serial->set_add_iso14230_header(false);

    int ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                   tr("Turn ignition ON and press OK to start initializing connection to ECU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);

    switch (ret)
    {
        case QMessageBox::Ok:
            if (cmd_type == "read")
            {
                emit external_logger("Reading ROM, please wait...");
                send_log_window_message("Reading ROM from ECU using K-Line", true, true);
                result = read_mem(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
            }
            else
            {
                send_log_window_message("Connecting to Subaru ECU Hitachi K-Line bootloader, please wait...", true, true);

                result = connect_bootloader_subaru_ecu_hitachi_kline();

                if (result == STATUS_SUCCESS)
                {
                    emit external_logger("Writing ROM, please wait...");
                    send_log_window_message("Writing ROM to ECU Subaru Hitachi using K-Line", true, true);
                    //result = write_mem_subaru_denso_can_02_32bit(test_write);
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

FlashEcuSubaruHitachiM32rKline::~FlashEcuSubaruHitachiM32rKline()
{

}

void FlashEcuSubaruHitachiM32rKline::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

/*
 * Connect to Subaru TCU Hitachi CAN bootloader
 *
 * @return success
 */
int FlashEcuSubaruHitachiM32rKline::connect_bootloader_subaru_ecu_hitachi_kline()
{
    QByteArray received;
    QByteArray seed;
    QByteArray seed_key;
    QByteArray output;

    QString msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    delay(100);

    send_log_window_message("Initializing k-line communications", true, true);

    received = send_subaru_sid_bf_ssm_init();

    if (received == "" || received.length() < 13)
        return STATUS_ERROR;

    received.remove(0, 8);
    received.remove(5, received.length() - 5);
    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
    }
    QString calid = msg;

    send_log_window_message("Init Success: CAL ID = " + calid, true, true);
    qDebug() << "CAL ID = " << calid;

    send_subaru_sid_b8_change_baudrate_38400();

    // Start communication
    send_log_window_message("Start communication ok", true, true);
    qDebug() << "Start communication ok";

    // Request timing parameters
    received = send_subaru_sid_81_start_communication();
    if (received == "" || (uint8_t)received.at(4) != 0xC1)
        return STATUS_ERROR;

    received = send_subaru_sid_83_request_timings();
    if (received == "" || (uint8_t)received.at(4) != 0xC3)
        return STATUS_ERROR;

    send_log_window_message("Request timing parameters ok", true, true);
    qDebug() << "Request timing parameters ok";

    // Request seed
    received = send_subaru_sid_27_request_seed();
    if (received == "" || (uint8_t)received.at(4) != 0x67)
        return STATUS_ERROR;

    send_log_window_message("Seed request ok", true, true);
    qDebug() << "Seed request ok";

    seed.append(received.at(6));
    seed.append(received.at(7));
    seed.append(received.at(8));
    seed.append(received.at(9));

    seed_key = generate_seed_key(seed);
    msg = parse_message_to_hex(seed_key);
    send_log_window_message("Calculated seed key: " + msg + ", sending to ECU", true, true);
    qDebug() << "Calculated seed key: " + msg + ", sending to ECU";
    send_log_window_message("Sending seed key", true, true);
    qDebug() << "Sending seed key";

    received = send_subaru_sid_27_send_seed_key(seed_key);
    send_log_window_message("SID_27_02 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0x67)
       return STATUS_ERROR;

    send_log_window_message("Seed key ok", true, true);
    qDebug() << "Seed key ok";

    // Start diagnostic session
    received = send_subaru_sid_10_start_diagnostic();
    if (received == "" || (uint8_t)received.at(4) != 0x50)
        return STATUS_ERROR;

    send_log_window_message("Start diagnostic session ok", true, true);
    qDebug() << "Start diagnostic session ok";

    return STATUS_SUCCESS;
}


/*
 *
 * For reading a ROM using a0 command
 *
 */
int FlashEcuSubaruHitachiM32rKline::read_mem(uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;

    QByteArray output;
    QByteArray received;
    QString msg;
    QByteArray mapdata;

    uint32_t pagesize = 0;
    uint32_t end_addr = 0;
    uint32_t datalen = 0;
    uint32_t cplen = 0;

    uint8_t chk_sum = 0;

    if (!serial->is_serial_port_open())
    {
        LOG_E("ERROR: Serial port is not open.", true, true);
        //send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    // SSM init
    received = send_subaru_sid_bf_ssm_init();
    if (received == "" || (uint8_t)received.at(4) != 0xff)
        return STATUS_ERROR;

    if (received.length() < 13)
        return STATUS_ERROR;

    received.remove(0, 8);
    received.remove(5, received.length() - 5);

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
    }
    QString ecuid = msg;
    LOG_I("ECU ID = " + ecuid, true, true);
    //send_log_window_message("ECU ID = " + ecuid, true, true);

    received = send_subaru_sid_b8_change_baudrate_38400();
    LOG_I("0xB8 response: " + parse_message_to_hex(received), true, true);
    //send_log_window_message("0xB8 response: " + parse_message_to_hex(received), true, true);
    //qDebug() << "0xB8 response:" << parse_message_to_hex(received);
    if (received == "" || (uint8_t)received.at(4) != 0xf8)
        return STATUS_ERROR;

    serial->change_port_speed("38400");

    // Checking connection after baudrate change with SSM Init
    received = send_subaru_sid_bf_ssm_init();
    if (received == "" || (uint8_t)received.at(4) != 0xff)
        return STATUS_ERROR;

    start_addr += 0x00100000;
    datalen = 6;
    pagesize = 0x80;
    if (start_addr == 0 && length == 0)
    {
        start_addr = 0;
        length = 0x040000;
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
    output.append((uint8_t)0xA0);
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

        //qDebug() << "Received map data:" << parse_message_to_hex(received);
        if (received.startsWith("\x80\xf0"))
        {
            received.remove(0, 5);
            received.remove(received.length() - 1, 1);
            mapdata.append(received);
        }
        else
        {
            LOG_E("ERROR IN DATA RECEIVE! " + parse_message_to_hex(received), true, true);
            //qDebug() << "ERROR IN DATA RECEIVE!" << hex << addr << parse_message_to_hex(received);
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
        LOG_I(msg, true, true);
        send_log_window_message(msg, true, true);
        //qDebug() << msg;
        //delay(1);

        len_done += cplen;
        addr += (numblocks * pagesize);
        willget -= pagesize;
    }

    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

int FlashEcuSubaruHitachiM32rKline::write_mem(bool test_write)
{
    QByteArray received;
    QByteArray filedata;

    filedata = ecuCalDef->FullRomData;

    uint8_t data_array[filedata.length()];

    int block_modified[16] = {1};   // assume all blocks are modified

    unsigned bcnt;
    unsigned blockno;

    //encrypt the data
    filedata = encrypt_32bit_payload(filedata, filedata.length());

    for (int i = 0; i < filedata.length(); i++)
    {
        data_array[i] = filedata.at(i);
    }

    bcnt = 0;
    send_log_window_message("Blocks to flash : ", true, false);
    for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++) {
        if (block_modified[blockno]) {
            send_log_window_message(QString::number(blockno) + ", ", false, false);
            bcnt += 1;
        }
    }
    send_log_window_message(" (total: " + QString::number(bcnt) + ")", false, true);

    if (bcnt)
    {
//        send_log_window_message("--- erasing TCU flash memory ---", true, true);
//        if (erase_subaru_ecu_hitachi_can())
//        {
//            send_log_window_message("--- erasing did not complete successfully ---", true, true);
//            return STATUS_ERROR;
//        }
//        received = send_subaru_ecu_sid_83_request_timings();

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
        for (blockno = 0; blockno < flashdevices[mcu_type_index].numblocks; blockno++)  // hack so that only 1 flash loop done for the entire ROM above 0x8000
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

    }
    else
    {
        send_log_window_message("*** No blocks require flash! ***", true, true);
    }

    return STATUS_SUCCESS;
}

/*
 * SSM Block Write (K-Line)
 *
 * @return received response
 */

int FlashEcuSubaruHitachiM32rKline::reflash_block(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write)
{

    int errval;

    uint32_t start_address, end_addr;
    uint32_t pl_len;
    uint16_t maxblocks;
    uint16_t blockctr;
    uint32_t blockaddr;

    QByteArray output;
    QByteArray received;
    QByteArray msg;
//    QByteArray buf;
//    uint8_t chk_sum = 0;

    set_progressbar_value(0);

    if (blockno >= fdt->numblocks) {
        send_log_window_message("block " + QString::number(blockno) + " out of range !", true, true);
        return -1;
    }

    start_address = fdt->fblocks[blockno].start;
    pl_len = fdt->fblocks[blockno].len;
    maxblocks = pl_len / 128;
    end_addr = (start_address + (maxblocks * 128)) & 0xFFFFFFFF;
    uint32_t data_len = end_addr - start_address;

    QString start_addr = QString("%1").arg((uint32_t)start_address,8,16,QLatin1Char('0')).toUpper();
    QString length = QString("%1").arg((uint32_t)pl_len,8,16,QLatin1Char('0')).toUpper();
    msg = QString("Flash block addr: 0x" + start_addr + " len: 0x" + length).toUtf8();
    send_log_window_message(msg, true, true);

    send_log_window_message("Settting flash start & length...", true, true);
    qDebug() << "Settting flash start & length...";

//    delay(200);
    serial->change_port_speed("15625");

    output.clear();
    output.append((uint8_t)0x34);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x04);
    output.append((uint8_t)0x08);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(200);

    received = serial->read_serial_data(20, 200);
    send_log_window_message("Response: " + parse_message_to_hex(received), true, true);

    if (received == "" || (uint8_t)received.at(4) != 0x74)
    return STATUS_ERROR;

    delay(200);

    QMessageBox::information(this, tr("Init was OK?"), "Press OK to continue");

    send_log_window_message("Erasing...", true, true);

    output.clear();
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFF);

    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    delay(200);
    //received = serial->read_serial_data(20, 200);
    //send_log_window_message("Response: " + parse_message_to_hex(received), true, true);

    send_log_window_message("", true, false);
    received.clear();
    for (int i = 0; i < 20; i++)
    {
        received.append(serial->read_serial_data(8, receive_timeout));
        send_log_window_message(".", false, false);
        qDebug() << ".";
        if (received.length() > 5)
        {
            if ((uint8_t)received.at(4) == 0x71 && (uint8_t)received.at(5) == 0x02)
            {
                send_log_window_message("", false, true);
                send_log_window_message("Flash erase in progress, please wait...", true, true);
                qDebug() << "Flash erase in progress, please wait...";
                break;
            }
            else
            {
                send_log_window_message("", false, true);
                send_log_window_message("Flash erase failed!", true, true);
                qDebug() << "Flash erase failed!";
                send_log_window_message("Received: " + parse_message_to_hex(received), true, true);
                qDebug() << "Received: " + parse_message_to_hex(received);
                return STATUS_ERROR;
            }
        }
        delay(500);
    }
    if (received == "")
    {
        send_log_window_message("", false, true);
        send_log_window_message("Flash erase failed, no answer from ECU!", true, true);
        qDebug() << "Flash erase failed, no answer from ECU!";
        send_log_window_message("Received: " + parse_message_to_hex(received), true, true);
        qDebug() << "Received: " + parse_message_to_hex(received);

        return STATUS_ERROR;
    }
    send_log_window_message("", true, false);

    delay(500);

    send_log_window_message("Flash erased, starting ROM write...", true, true);
    qDebug() << "Flash erased, starting ROM write...";

    int data_bytes_sent = 0;
    for (blockctr = 0; blockctr < maxblocks; blockctr++)
    {
        if (kill_process)
            return 0;

        blockaddr = start_address + blockctr * 128;
        output.clear();
        output.append((uint8_t)0x36);
        output.append((uint8_t)(blockaddr >> 16) & 0xFF);
        output.append((uint8_t)(blockaddr >> 8) & 0xFF);
        output.append((uint8_t)blockaddr & 0xFF);

        for (int i = 0; i < 128; i++)
        {
//            output[i + 8] = (uint8_t)(newdata[i + blockaddr] & 0xFF);
            output[i + 4] = (uint8_t)(newdata[i + blockaddr] & 0xFF);
            data_bytes_sent++;
        }
        data_len -= 128;

        serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
        send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);

    received = serial->read_serial_data(20, 200);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);

        float pleft = (float)blockctr / (float)maxblocks * 100;
        set_progressbar_value(pleft);

    }

    set_progressbar_value(100);

//    return STATUS_SUCCESS;

    send_log_window_message("Verifying checksum...", true, true);
    qDebug() << "Verifying checksum...";

//    connected = false;
//    try_count = 0;
    delay(300);
    output.clear();
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x02);

//    while (try_count < 20 && connected == false)
    {
        serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
//        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        delay(50);
        received = serial->read_serial_data(20, 200);
        if (received != "")
        {
//            connected = true;
            send_log_window_message(": 0x31 response: " + parse_message_to_hex(received), true, true);
        }
//        try_count++;
        //delay(try_timeout);
    }
    if (received == "" || (uint8_t)received.at(4) != 0x71 || (uint8_t)received.at(5) != 0x01 || (uint8_t)received.at(6) != 0x02)
        send_log_window_message("No or bad response received", true, true);
        //return STATUS_ERROR;

    send_log_window_message("Checksum verified...", true, true);
    qDebug() << "Checksum verified...";

    return STATUS_SUCCESS;

}

/*
 * SSM Block Read (K-Line)
 *
 * @return received response
 */
QByteArray FlashEcuSubaruHitachiM32rKline::send_subaru_sid_a0_block_read(uint32_t dataaddr, uint32_t datalen)
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
    received = serial->read_serial_data(datalen + 2 + 3, 200);
    send_log_window_message("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Generate denso kline seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashEcuSubaruHitachiM32rKline::generate_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex[]={
        0x3275, 0x6AD8, 0x1062, 0x512B,
        0xD695, 0x7640, 0x25F6, 0xAC45,
        0x6803, 0xE5DA, 0xC821, 0x36BF,
        0xA433, 0x3F41, 0x842C, 0x05D9
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
QByteArray FlashEcuSubaruHitachiM32rKline::calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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

QByteArray FlashEcuSubaruHitachiM32rKline::encrypt_32bit_payload(QByteArray buf, uint32_t len)
{
    QByteArray encrypted;

    const uint16_t keytogenerateindex[]={
        0x78F1, 0x2962, 0x9312, 0x7c03
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    encrypted = calculate_32bit_payload(buf, len, keytogenerateindex, indextransformation);

    return encrypted;
}

QByteArray FlashEcuSubaruHitachiM32rKline::decrypt_32bit_payload(QByteArray buf, uint32_t len)
{
    QByteArray decrypt;

    const uint16_t keytogenerateindex[]={
//	0x6587, 0x4492, 0xa8b4, 0x7bf2
      0x7c03, 0x9312, 0x2962, 0x78F1
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    decrypt = calculate_32bit_payload(buf, len, keytogenerateindex, indextransformation);

    return decrypt;
}

QByteArray FlashEcuSubaruHitachiM32rKline::calculate_32bit_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
 * ECU init
 *
 * @return ECU ID and capabilities
 */
QByteArray FlashEcuSubaruHitachiM32rKline::send_subaru_sid_bf_ssm_init()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0xBF);

    while (received == "" && loop_cnt < 1)
    {
        if (kill_process)
            break;

        send_log_window_message("SSM init", true, true);
        serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
        send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);

        delay(200);
        received = serial->read_serial_data(100, receive_timeout);
        send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
        loop_cnt++;
    }

    return received;
}

/*
 * Start diagnostic connection
 *
 * @return received response
 */
QByteArray FlashEcuSubaruHitachiM32rKline::send_subaru_sid_81_start_communication()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    output.clear();
    output.append((uint8_t)0x81);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Start diagnostic connection
 *
 * @return received response
 */
QByteArray FlashEcuSubaruHitachiM32rKline::send_subaru_sid_83_request_timings()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    output.clear();
    output.append((uint8_t)0x83);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Request seed
 *
 * @return seed (4 bytes)
 */
QByteArray FlashEcuSubaruHitachiM32rKline::send_subaru_sid_27_request_seed()
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
    send_log_window_message("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Send seed key
 *
 * @return received response
 */
QByteArray FlashEcuSubaruHitachiM32rKline::send_subaru_sid_27_send_seed_key(QByteArray seed_key)
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
    send_log_window_message("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

/*
 * Request start diagnostic
 *
 * @return received response
 */
QByteArray FlashEcuSubaruHitachiM32rKline::send_subaru_sid_10_start_diagnostic()
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
    send_log_window_message("Response: " + parse_message_to_hex(received), true, true);

    return received;
}

QByteArray FlashEcuSubaruHitachiM32rKline::send_subaru_sid_b8_change_baudrate_38400()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start B8";
    output.clear();
    output.append((uint8_t)0xB8);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x75);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    send_log_window_message("Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false)), true, true);
    qDebug() << "Sent: " + parse_message_to_hex(add_ssm_header(output, tester_id, target_id, false));
    delay(comm_try_timeout);
    received = serial->read_serial_data(8, receive_timeout);
    send_log_window_message("Response: " + parse_message_to_hex(received), true, true);
    qDebug() << "Response: " + parse_message_to_hex(received);

    return received;
}


/*
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray FlashEcuSubaruHitachiM32rKline::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
{
    QByteArray received;
    QByteArray msg;

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
uint8_t FlashEcuSubaruHitachiM32rKline::calculate_checksum(QByteArray output, bool dec_0x100)
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
QString FlashEcuSubaruHitachiM32rKline::parse_message_to_hex(QByteArray received)
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
int FlashEcuSubaruHitachiM32rKline::send_log_window_message(QString message, bool timestamp, bool linefeed)
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

void FlashEcuSubaruHitachiM32rKline::set_progressbar_value(int value)
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

void FlashEcuSubaruHitachiM32rKline::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

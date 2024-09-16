#include "flash_ecu_subaru_denso_sh72531_can.h"

FlashEcuSubaruDensoSH72531Can::FlashEcuSubaruDensoSH72531Can(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
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

FlashEcuSubaruDensoSH72531Can::~FlashEcuSubaruDensoSH72531Can()
{

}

void FlashEcuSubaruDensoSH72531Can::run()
{
    this->show();

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
    serial->set_add_iso14230_header(false);
    serial->set_is_can_connection(false);
    serial->set_is_iso15765_connection(true);
    serial->set_is_29_bit_id(false);
    serial->set_can_speed("500000");
    serial->set_iso15765_source_address(0x7E0);
    serial->set_iso15765_destination_address(0x7E8);
    // Open serial port
    serial->open_serial_port();


    int ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                   tr("Turn ignition ON and press OK to start initializing connection to ECU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);

    switch (ret)
    {
        case QMessageBox::Ok:
            send_log_window_message("Connecting to ECU Hitachi SH72531 CAN bootloader, please wait...", true, true);
            result = connect_bootloader();

            if (result == STATUS_SUCCESS)
            {
                if (cmd_type == "read")
                {
                    emit external_logger("Reading ROM, please wait...");
                    send_log_window_message("Not yet implemented: Reading ROM from ECU, Hitachi SH72531 using CAN", true, true);
                    result = read_memory(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                }
                else if (cmd_type == "test_write" || cmd_type == "write")
                {
                    emit external_logger("Writing ROM, please wait...");
                    send_log_window_message("Not yet implemented: Writing ROM to ECU, Hitachi SH72531 using CAN", true, true);
                    result = write_memory(test_write);
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

void FlashEcuSubaruDensoSH72531Can::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

/*
 * Connect to Subaru TCU Hitachi CAN bootloader
 *
 * @return success
 */
int FlashEcuSubaruDensoSH72531Can::connect_bootloader()
{
    QByteArray output;
    QByteArray received;
    QByteArray seed;
    QByteArray seed_key;
    QByteArray response;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    bool req_10_03_connected = false;
    bool req_10_43_connected = false;

    bool connected = false;
    int try_count = 0;

    connected = false;
    try_count = 0;

    send_log_window_message("Trying 0x09 0x04...", true, true);
    qDebug() << "Trying 0x09 0x04...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x09);
    output.append((uint8_t)0x04);
/*
    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        received = serial->read_serial_data(20, 200);
        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x09 0x04 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x09 0x04 response:" << parse_message_to_hex(received) << received;
        }
        try_count++;
    }
*/
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x49 && (uint8_t)received.at(5) == 0x04)
        {
            response.remove(0, 7);
            response.remove(8, 8);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);

            qDebug() << "Response:" << parse_message_to_hex(received);
        }
        else
        {
            send_log_window_message("Wrong response from ECU... (" + parse_message_to_hex(received) + ")", true, true);
            return STATUS_ERROR;
        }
    }

    QString ecuid = QString::fromUtf8(response);
    send_log_window_message("Init Success: ECU ID = " + ecuid, true, true);

    send_log_window_message("Initializing bootloader...", true, true);
    qDebug() << "Initializing bootloader...";

    connected = false;
    output[4] = ((uint8_t)0x10);
    output[5] = ((uint8_t)0x03);

    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x50 && (uint8_t)received.at(5) == 0x03)
        {
            req_10_03_connected = true;
            QByteArray response = received;
            response.remove(0, 6);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);

            qDebug() << "Response:" << parse_message_to_hex(received);
        }
        else
        {
            send_log_window_message("Wrong response from ECU... (" + parse_message_to_hex(received) + ")", true, true);
            return STATUS_ERROR;
        }
    }

    output[4] = ((uint8_t)0x10);
    output[5] = ((uint8_t)0x43);

    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x50 && (uint8_t)received.at(5) == 0x43)
        {
            req_10_43_connected = true;
            QByteArray response = received;
            response.remove(0, 6);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);

            qDebug() << "Response:" << parse_message_to_hex(received);
        }
        else
        {
            send_log_window_message("Wrong response from ECU... (" + parse_message_to_hex(received) + ")", true, true);
            return STATUS_ERROR;
        }
    }

    send_log_window_message("Starting seed request...", true, true);
    qDebug() << "Starting seed request...";

    output[4] = (uint8_t)0x27;
    output[5] = (uint8_t)0x61;
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x67 && (uint8_t)received.at(5) == 0x61)
        {
            QByteArray response = received;
            response.remove(0, 6);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);

            qDebug() << "Response:" << parse_message_to_hex(received);
        }
        else
        {
            send_log_window_message("Wrong response from ECU... (" + parse_message_to_hex(received) + ")", true, true);
            return STATUS_ERROR;
        }
    }

    send_log_window_message("Seed request ok", true, true);
    qDebug() << "Seed request ok";

    seed.append(received.at(6));
    seed.append(received.at(7));
    seed.append(received.at(8));
    seed.append(received.at(9));

    seed_key = generate_can_seed_key(seed);

    send_log_window_message("Sending seed key...", true, true);
    qDebug() << "Sending seed key...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x27);
    output.append((uint8_t)0x62);
    output.append(seed_key);
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x67 && (uint8_t)received.at(5) == 0x61)
        {
            QByteArray response = received;
            response.remove(0, 6);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);

            qDebug() << "Response:" << parse_message_to_hex(received);
        }
        else
        {
            send_log_window_message("Wrong response from ECU... (" + parse_message_to_hex(received) + ")", true, true);
            return STATUS_ERROR;
        }
    }

    send_log_window_message("Seed key ok", true, true);
    qDebug() << "Seed key ok";

    send_log_window_message("Jumping to onboad kernel...", true, true);
    qDebug() << "Jumping to onboad kernel...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x10);
    if (req_10_03_connected)
        output.append((uint8_t)0x02);
    if (req_10_43_connected)
        output.append((uint8_t)0x42);

    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(50);
    received = serial->read_serial_data(20, serial_read_timeout);
    if (received.length() > 5)
    {
        if ((uint8_t)received.at(4) == 0x50 && (uint8_t)received.at(5) == 0x02)
        {
            QByteArray response = received;
            response.remove(0, 6);
            QString msg;
            msg.clear();
            for (int i = 0; i < response.length(); i++)
                msg.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
            send_log_window_message("Response: " + parse_message_to_hex(received), true, true);

            qDebug() << "Response:" << parse_message_to_hex(received);
        }
        else
        {
            send_log_window_message("Wrong response from ECU... (" + parse_message_to_hex(received) + ")", true, true);
            return STATUS_ERROR;
        }
    }

    return STATUS_SUCCESS;
}

/*
 * Read memory from Subaru TCU Hitachi CAN bootloader
 *
 * @return success
 */
int FlashEcuSubaruDensoSH72531Can::read_memory(uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray pagedata;
    QByteArray mapdata;
    uint32_t cplen = 0;
    uint32_t timeout = 0;

    uint32_t pagesize = 0x100;

    start_addr = 0x8000;

    length = 0x137F00;    // hack for testing

    uint32_t skip_start = start_addr & (pagesize - 1); //if unaligned, we'll be receiving this many extra bytes
    uint32_t addr = start_addr - skip_start;
    uint32_t willget = (skip_start + length + pagesize - 1) & ~(pagesize - 1);
    uint32_t len_done = 0;  //total data written to file

    send_log_window_message("Settting dump start & length...", true, true);
    qDebug() << "Settting dump start & length...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x34);
    output.append((uint8_t)0x04);
    output.append((uint8_t)0x44);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x80);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x13);
    output.append((uint8_t)0x7F);
    output.append((uint8_t)0x00);


    send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);
    serial->write_serial_data_echo_check(output);
    delay(200);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
    if ((uint8_t)received.at(4) != 0x74 || (uint8_t)received.at(5) != 0x20 || (uint8_t)received.at(6) != 0x01 || (uint8_t)received.at(7) != 0x05)
    {
        send_log_window_message("Bad response to setting dump start & length", true, true);

        //return STATUS_ERROR;
    }

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x35);
    output.append((uint8_t)0x04);
    output.append((uint8_t)0x44);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x80);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x13);
    output.append((uint8_t)0x7F);
    output.append((uint8_t)0x00);


    send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);
    serial->write_serial_data_echo_check(output);
    delay(200);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
    if ((uint8_t)received.at(4) != 0x75 || (uint8_t)received.at(5) != 0x20 || (uint8_t)received.at(6) != 0x01 || (uint8_t)received.at(7) != 0x01)
    {
        send_log_window_message("Bad response to setting dump start & length", true, true);

        //return STATUS_ERROR;
    }

    send_log_window_message("Start reading ROM, please wait...", true, true);
    qDebug() << "Start reading ROM, please wait...";

    // send 0xB7 command to kernel to dump from ROM
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0xB7);
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

        output[6] = (uint8_t)((addr >> 16) & 0xFF);
        output[7] = (uint8_t)((addr >> 8) & 0xFF);
        output[8] = (uint8_t)(addr & 0xFF);
        send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);
        serial->write_serial_data_echo_check(output);

        received = serial->read_serial_data(270, 2000);
        send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);

        if ((uint8_t)received.at(4) != 0xF7)
        {
            send_log_window_message("Page data request failed!", true, true);
            return STATUS_ERROR;
        }

        pagedata.clear();
        pagedata = received.remove(0, 5);

        send_log_window_message("Received pagedata: " + parse_message_to_hex(pagedata), true, true);
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

    send_log_window_message("ROM read complete", true, true);
    qDebug() << "ROM read complete";

    send_log_window_message("Sending stop command...", true, true);
    qDebug() << "Sending stop command...";

    bool connected = false;
    int try_count = 0;
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x37);

    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        delay(200);
        received = serial->read_serial_data(270, 500);
        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x37 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x37 response:" << parse_message_to_hex(received);
        }
        try_count++;
        //delay(try_timeout);
    }

    mapdata = decrypt_payload(mapdata, mapdata.length());

    // need to pad out first 0x8000 bytes with 0xFF
    int i;
    QByteArray padBytes;
    for (i = 0; i < 0x8000; i++)
    {
        padBytes[i] = (uint8_t)0xFF;
    }
    mapdata = mapdata.insert(0, padBytes);

    int k;
    QByteArray endBytes;
    for (k = 0; k < 0x100; k++)
    {
        endBytes[k] = (uint8_t)0xFF;
    }
    mapdata = mapdata.insert(0x13FF00, endBytes);

    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*
 * Write memory to Subaru Hitachi CAN 32bit TCUs, on board kernel
 *
 * @return success
 */

int FlashEcuSubaruDensoSH72531Can::write_memory(bool test_write)
{
    QByteArray filedata;

    filedata = ecuCalDef->FullRomData;

    uint8_t data_array[filedata.length()];

    int block_modified[16] = {0,1};   // assume blocks after 0x8000 are modified

    unsigned bcnt;  // 13 blocks in M32R_512kB, but kernelmemorymodels.h has 11. Of these 11, the first 3 are not flashed by OBK
    unsigned blockno;

    //encrypt the data
    filedata = encrypt_payload(filedata, filedata.length());

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
        send_log_window_message("--- erasing ECU flash memory ---", true, true);
        if (erase_memory())
        {
            send_log_window_message("--- erasing did not complete successfully ---", true, true);
            return STATUS_ERROR;
        }

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
 * Reflash ROM 32bit CAN (iso15765) ECUs
 *
 * @return success
 */
int FlashEcuSubaruDensoSH72531Can::reflash_block(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write)
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

    set_progressbar_value(0);

    if (blockno >= fdt->numblocks) {
        send_log_window_message("block " + QString::number(blockno) + " out of range !", true, true);
        return -1;
    }

    start_address = fdt->fblocks[blockno].start;
    pl_len = fdt->fblocks[blockno].len;
    maxblocks = pl_len / 256;
    end_addr = (start_address + (maxblocks * 256)) & 0xFFFFFFFF;
    uint32_t data_len = end_addr - start_address;

    QString start_addr = QString("%1").arg((uint32_t)start_address,8,16,QLatin1Char('0')).toUpper();
    QString length = QString("%1").arg((uint32_t)pl_len,8,16,QLatin1Char('0')).toUpper();
    msg = QString("Flash block addr: 0x" + start_addr + " len: 0x" + length).toUtf8();
    send_log_window_message(msg, true, true);

    int data_bytes_sent = 0;
    for (blockctr = 0; blockctr < maxblocks; blockctr++)
    {
        if (kill_process)
            return 0;

        blockaddr = start_address + blockctr * 256;
        output.clear();
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x07);
        output.append((uint8_t)0xE0);
        output.append((uint8_t)0xB6);
        output.append((uint8_t)0x00);
        output.append((uint8_t)(blockaddr >> 16) & 0xFF);
        output.append((uint8_t)(blockaddr >> 8) & 0xFF);
        output.append((uint8_t)blockaddr & 0xFF);
        //qDebug() << "Data header:" << parse_message_to_hex(output);

        for (int i = 0; i < 256; i++)
        {
            output[i + 9] = (uint8_t)(newdata[i + blockaddr] & 0xFF);
            data_bytes_sent++;
        }
        data_len -= 256;

        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        //qDebug() << "Kernel data:" << parse_message_to_hex(output);
        //delay(20);
        received = serial->read_serial_data(5, receive_timeout);
        send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);

        if (received == "" || (uint8_t)received.at(3) != 0xE8 || (uint8_t)received.at(4) != 0xF6)
            return STATUS_ERROR;

        float pleft = (float)blockctr / (float)maxblocks * 100;
        set_progressbar_value(pleft);
    }
    qDebug() << "Data bytes sent:" << hex << data_bytes_sent;

    send_log_window_message("Closing out Flashing of this block...", true, true);
    qDebug() << "Closing out Flashing of this block...";

    bool connected = false;
    int try_count = 0;

    connected = false;
    try_count = 0;
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x37);

    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        //delay(50);
        delay(200);
        received = serial->read_serial_data(20, 200);
        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x37 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x37 response:" << parse_message_to_hex(received);
        }
        try_count++;
        //delay(try_timeout);
    }
    if (received == "" || (uint8_t)received.at(4) != 0x77)
        send_log_window_message("No or bad response received", true, true);
    //return STATUS_ERROR;

    delay(100);

    send_log_window_message("Verifying checksum...", true, true);
    qDebug() << "Verifying checksum...";

    connected = false;
    try_count = 0;
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x01);

    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    delay(200);
    received = serial->read_serial_data(20, 500);
    send_log_window_message(QString::number(try_count) + ": 0x31 response: " + parse_message_to_hex(received), true, true);
    delay(200);
    received = serial->read_serial_data(20, 500);
    send_log_window_message(QString::number(try_count) + ": 0x31 response: " + parse_message_to_hex(received), true, true);

    if (received == "" || (uint8_t)received.at(4) != 0x71 || (uint8_t)received.at(5) != 0x01 || (uint8_t)received.at(6) != 0x02)
        send_log_window_message("No or bad response received", true, true);
    //return STATUS_ERROR;

    send_log_window_message("Checksum verified...", true, true);
    qDebug() << "Checksum verified...";

    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*
 * Erase Subaru Hitachi TCU CAN (iso15765)
 *
 * @return success
 */
int FlashEcuSubaruDensoSH72531Can::erase_memory()
{
    QByteArray output;
    QByteArray received;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    send_log_window_message("Settting flash start & length...", true, true);
    qDebug() << "Settting flash start & length...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x34);
    output.append((uint8_t)0x04);
    output.append((uint8_t)0x44);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x80);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x13);
    output.append((uint8_t)0x7F);
    output.append((uint8_t)0x00);


    send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);
    serial->write_serial_data_echo_check(output);
    delay(200);
    received = serial->read_serial_data(20, 500);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
    if ((uint8_t)received.at(4) != 0x74 || (uint8_t)received.at(5) != 0x20 || (uint8_t)received.at(6) != 0x01 || (uint8_t)received.at(7) != 0x05)
    {
        send_log_window_message("Bad response to setting dump start & length", true, true);

        //return STATUS_ERROR;
    }

    send_log_window_message("Erasing ECU ROM...", true, true);
    qDebug() << "Erasing ECU ROM...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0xff);
    output.append((uint8_t)0xff);
    output.append((uint8_t)0xff);
    output.append((uint8_t)0xff);

    send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);
    serial->write_serial_data_echo_check(output);
    delay(500);

    received = serial->read_serial_data(20, 500);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);

    delay(200);

    while ((uint8_t)received.at(4) != 0x71 || (uint8_t)received.at(5) != 0x01 || (uint8_t)received.at(6) != 0x02)
    {
        received = serial->read_serial_data(20, 500);
        send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
        delay(100);
    }

    send_log_window_message("Erased! Starting Writing! Do Not Power Off!", true, true);

    return STATUS_SUCCESS;
}

/*
 * Generate seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashEcuSubaruDensoSH72531Can::generate_can_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex[]={
        0x78B1, 0x4625, 0x201C, 0x9EA5,
        0xAD6B, 0x35F4, 0xFD21, 0x5E71,
        0xB046, 0x7F4A, 0x4B75, 0x93F9,
        0x1895, 0x8961, 0x3ECC, 0x862B
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
 * Calculate seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashEcuSubaruDensoSH72531Can::calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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

QByteArray FlashEcuSubaruDensoSH72531Can::encrypt_payload(QByteArray buf, uint32_t len)
{
    QByteArray encrypted;

    const uint16_t keytogenerateindex[]={
        0xC85B, 0x32C0, 0xE282, 0x92A0
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

QByteArray FlashEcuSubaruDensoSH72531Can::decrypt_payload(QByteArray buf, uint32_t len)
{
    QByteArray decrypt;

    const uint16_t keytogenerateindex[]={
        0x92A0, 0xE282, 0x32C0, 0xC85B
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    decrypt = calculate_payload(buf, len, keytogenerateindex, indextransformation);

    return decrypt;
}

QByteArray FlashEcuSubaruDensoSH72531Can::calculate_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
 * Parse QByteArray to readable form
 *
 * @return parsed message
 */
QString FlashEcuSubaruDensoSH72531Can::parse_message_to_hex(QByteArray received)
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
int FlashEcuSubaruDensoSH72531Can::send_log_window_message(QString message, bool timestamp, bool linefeed)
{
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']  ");

    if (timestamp)
        message = dateTimeString + message;
    if (linefeed)
        message = message + "\n";

    QString filename = "log.txt";
    QFile file(filename);
    //QFileInfo fileInfo(file.fileName());
    //QString file_name_str = fileInfo.fileName();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Append ))
    {
        //qDebug() << "Unable to open file for writing";
        QMessageBox::warning(this, tr("Ecu calibration file"), "Unable to open file for writing");
        return NULL;
    }

    file.write(message.toUtf8());
    file.close();

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

void FlashEcuSubaruDensoSH72531Can::set_progressbar_value(int value)
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

void FlashEcuSubaruDensoSH72531Can::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}


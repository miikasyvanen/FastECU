#include "flash_tcu_cvt_subaru_hitachi_m32r_can.h"

//QT_CHARTS_USE_NAMESPACE

FlashTcuCvtSubaruHitachiM32rCan::FlashTcuCvtSubaruHitachiM32rCan(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
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
        this->setWindowTitle("Read ROM from TCU");

    this->serial = serial;
}

void FlashTcuCvtSubaruHitachiM32rCan::run()
{
    this->show();

    int result = STATUS_ERROR;
    set_progressbar_value(0);

    //result = init_flash_hitachi_can();

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
    serial->set_iso15765_source_address(0x7E1);
    serial->set_iso15765_destination_address(0x7E9);
    // Open serial port
    serial->open_serial_port();


    int ret = QMessageBox::warning(this, tr("Connecting to TCU"),
                                   tr("Turn ignition ON and press OK to start initializing connection to TCU"),
                                   QMessageBox::Ok | QMessageBox::Cancel,
                                   QMessageBox::Ok);

    switch (ret)
    {
        case QMessageBox::Ok:
            send_log_window_message("Connecting to Subaru TCU Hitachi CAN bootloader, please wait...", true, true);
            //result = connect_bootloader_subaru_tcu_hitachi_can();
            result = hack_words();

            if (result == STATUS_SUCCESS)
            {
                if (cmd_type == "read")
                {
                    emit external_logger("Reading ROM, please wait...");
                    send_log_window_message("Not yet implemented: Reading ROM from TCU Subaru Hitachi using CAN", true, true);
                    result = read_mem_subaru_tcu_hitachi_can(flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
                }
                else if (cmd_type == "test_write" || cmd_type == "write")
                {
                    emit external_logger("Writing ROM, please wait...");
                    send_log_window_message("Not yet implemented: Writing ROM to TCU Subaru Hitachi using CAN", true, true);
                    result = write_mem_subaru_tcu_hitachi_can(test_write);
                }
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

FlashTcuCvtSubaruHitachiM32rCan::~FlashTcuCvtSubaruHitachiM32rCan()
{

}

void FlashTcuCvtSubaruHitachiM32rCan::closeEvent(QCloseEvent *event)
{
    kill_process = true;
}

/*
 * Connect to Subaru TCU Hitachi CAN bootloader
 *
 * @return success
 */
int FlashTcuCvtSubaruHitachiM32rCan::connect_bootloader_subaru_tcu_hitachi_can()
{
    QByteArray output;
    QByteArray received;
    QByteArray seed;
    QByteArray seed_key;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    send_log_window_message("Checking if kernel is already running...", true, true);
    qDebug() << "Checking if kernel is already running...";

    // Check if kernel already alive
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x01);
    send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);
    serial->write_serial_data_echo_check(output);
    delay(200);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
    if ((uint8_t)received.at(4) == 0x71 && (uint8_t)received.at(5) == 0x02 && (uint8_t)received.at(6) == 0x02 && (uint8_t)received.at(7) == 0x03)
    {
        send_log_window_message("Kernel already running", true, true);

        kernel_alive = true;
        return STATUS_SUCCESS;
    }

    send_log_window_message("Trying TCU Init...", true, true);
    qDebug() << "Trying TCU Init...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0xAA);

    bool connected = false;
    int try_count = 0;

    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        //delay(50);
        received = serial->read_serial_data(20, 200);
        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0xAA response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0xAA response:" << parse_message_to_hex(received) << received;
        }
        try_count++;
        //delay(try_timeout);
    }

    QByteArray response = received;
    response.remove(0, 8);
    QString calid;
    for (int i = 0; i < 5; i++)
        calid.append(QString("%1").arg((uint8_t)response.at(i),2,16,QLatin1Char('0')).toUpper());
    send_log_window_message("Init Success: CAL ID = " + calid, true, true);

    connected = false;
    try_count = 0;

    send_log_window_message("Trying 0x09 0x04...", true, true);
    qDebug() << "Trying 0x09 0x04...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x09);
    output.append((uint8_t)0x04);

    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        //delay(50);
        received = serial->read_serial_data(20, 200);
        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x09 0x04 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x09 0x04 response:" << parse_message_to_hex(received) << received;
        }
        try_count++;
        //delay(try_timeout);
    }

    response = received;
    response.remove(0, 7);
    response.remove(8, 8);
    QString tcuid = QString::fromUtf8(response);
    send_log_window_message("Init Success: TCU ID = " + tcuid, true, true);

    send_log_window_message("Initializing bootloader...", true, true);
    qDebug() << "Initializing bootloader...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x10);
    output.append((uint8_t)0x03);
    send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);
    serial->write_serial_data_echo_check(output);
    delay(200);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);

    if ((uint8_t)received.at(4) != 0x50 || (uint8_t)received.at(5) != 0x03)
    {
        send_log_window_message("Failed to initialise bootloader", true, true);

        //return STATUS_ERROR;
    }

    send_log_window_message("Starting seed request...", true, true);
    qDebug() << "Starting seed request...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x27);
    output.append((uint8_t)0x01);
    send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);
    serial->write_serial_data_echo_check(output);
    delay(200);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
    if ((uint8_t)received.at(4) != 0x67 || (uint8_t)received.at(5) != 0x01)
    {
        send_log_window_message("Bad response to seed request", true, true);

        //return STATUS_ERROR;
    }

    send_log_window_message("Seed request ok", true, true);
    qDebug() << "Seed request ok";

    seed.append(received.at(6));
    seed.append(received.at(7));
    seed.append(received.at(8));
    seed.append(received.at(9));

    seed_key = subaru_tcu_hitachi_generate_can_seed_key(seed);

    send_log_window_message("Sending seed key...", true, true);
    qDebug() << "Sending seed key...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x27);
    output.append((uint8_t)0x02);
    output.append(seed_key);
    send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);
    serial->write_serial_data_echo_check(output);
    delay(200);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
    if ((uint8_t)received.at(4) != 0x67 || (uint8_t)received.at(5) != 0x02)
    {
        send_log_window_message("Bad response to seed request", true, true);

        //return STATUS_ERROR;
    }

    send_log_window_message("Seed key ok", true, true);
    qDebug() << "Seed key ok";

    send_log_window_message("Jumping to onboad kernel...", true, true);
    qDebug() << "Jumping to onboad kernel...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x10);
    output.append((uint8_t)0x02);
    send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);
    serial->write_serial_data_echo_check(output);
    delay(200);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
    if ((uint8_t)received.at(4) != 0x50 || (uint8_t)received.at(5) != 0x02)
    {
        send_log_window_message("Bad response to jumping to onboard kernel", true, true);

        return STATUS_ERROR;
    }

    send_log_window_message("Jump to kernel ok", true, true);
    qDebug() << "Jump to kernel ok";

    send_log_window_message("Checking if jump successful and kernel alive...", true, true);
    qDebug() << "Checking if jump successful and kernel alive...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x01);
    send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);
    serial->write_serial_data_echo_check(output);
    delay(200);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
    if ((uint8_t)received.at(4) == 0x71 && (uint8_t)received.at(5) == 0x02 && (uint8_t)received.at(6) == 0x02 && (uint8_t)received.at(7) == 0x03)
    {
        send_log_window_message("Kernel verified to be running", true, true);

        kernel_alive = true;
        //return STATUS_SUCCESS;
    }

    send_log_window_message("Test script complete", true, true);
    return STATUS_SUCCESS;
}

/*
 *
 * Hack encryption words
 *
 */
int FlashTcuCvtSubaruHitachiM32rCan::hack_words()
{
    QByteArray pagedata, romdata;
    char indata[0x100];
    uint16_t i, j, k, l, m;

    pagedata = QByteArray::fromHex("082130ce27bbf3091f96fb444d5568947d98bdbd7d98bdbd28b4cb2801c872d47d98bdbd3a2126c77d98bdbd4cd5f99796c04d3e7d98bdbd7d98bdbd2bed7d8a164ae44523ab68e87d98bdbd7d98bdbd7d98bdbd7d98bdbd7d98bdbd7d98bdbd7d98bdbd7d98bdbd7d98bdbd7d98bdbd7d98bdbd7d98bdbd7d98bdbd2a810a7c7d98bdbd878611f70e1d2bf79e14ec64488d31557d285047d1952a6a6df5709f6e2cb12f5cb39a30bb287a381b2bd4314e943d9875e2bb391a0ac0d31f66e710e734f7aa8aa9e9687202926105e6775afcd1154f9d28e6e61283b231a815935a9310a5456007cc252cb53c7136276f9836276f9836276f9836276f9836276f98");

    send_log_window_message("Loading TCU ROM from default.bin...", true, true);
    QString filename = "default.bin";
    QFile file(filename);
    QDataStream inStream(&file);
    //QFileInfo fileInfo(file.fileName());
    //QString file_name_str = fileInfo.fileName();

    if (!file.open(QIODevice::ReadOnly ))
    {
        //qDebug() << "Unable to open file for reading";
        QMessageBox::warning(this, tr("Ecu calibration file"), "Unable to open file for reading");
        return 0;
    }

    inStream.skipRawData(0x8000);
    inStream.readRawData(indata, 0x100);
    file.close();

    for(i = 0; i < 0x100; i++) romdata.append(indata[i]);

    send_log_window_message("Send msg: " + parse_message_to_hex(pagedata), true, true);
    send_log_window_message("Send msg: " + parse_message_to_hex(romdata), true, true);

    QByteArray decrypt, testpage, testrom;
    uint16_t keytogenerateindex[4];
    //const uint16_t keytogenerateindex[]={
    //    0x1075, 0x9E51, 0x8BEF, 0x3B61
    //};
    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    for(i = 0; i < 0xf; i++)
    {
        for(j = 0; j < 0xf; j++)
        {
            for(k = 0; k < 0xf; k++)
            {
                for(l = 0; l < 0xf; l++)
                {
                    keytogenerateindex[0] = i;
                    keytogenerateindex[1] = j;
                    keytogenerateindex[2] = k;
                    keytogenerateindex[3] = l;


                    for(m = 4; m <= 0x10; m += 4)
                    {
                        testpage = pagedata.chopped(0x100 - m);
                        testrom = pagedata.chopped(0x100 - m);
                        send_log_window_message("Attempt: " + QString::number(i) + " " + QString::number(j) + " " + QString::number(k) + " " + QString::number(l) + " " + QString::number(m), true, true);
                        decrypt = subaru_tcu_hitachi_calculate_32bit_payload(testpage, m, keytogenerateindex, indextransformation);
                        send_log_window_message("Decrypted: " + parse_message_to_hex(decrypt), true, true);
                        if (decrypt != testrom) break;
                    }
                }
            }
        }
    }





    return STATUS_ERROR;
}


/*
 * Read memory from Subaru TCU Hitachi CAN bootloader
 *
 * @return success
 */
int FlashTcuCvtSubaruHitachiM32rCan::read_mem_subaru_tcu_hitachi_can(uint32_t start_addr, uint32_t length)
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


    start_addr = start_addr - 0x00100000;          // manual adjustment for starting address
    if (start_addr < 0x8000)                       // TCU code does not allow dumping below 0x8000
    {
        length = length - (0x8000 - start_addr);
        start_addr = 0x8000;
    }

    //length = 0x400;    // hack for testing

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
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x34);
    output.append((uint8_t)0x04);
    output.append((uint8_t)0x33);
    output.append((uint8_t)((addr >> 16) & 0xFF));
    output.append((uint8_t)((addr >> 8) & 0xFF));
    output.append((uint8_t)(addr & 0xFF));
    output.append((uint8_t)((willget >> 16) & 0xFF));
    output.append((uint8_t)((willget >> 8) & 0xFF));
    output.append((uint8_t)(willget & 0xFF));


    send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);
    serial->write_serial_data_echo_check(output);
    delay(200);
    received = serial->read_serial_data(20, 200);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
    if ((uint8_t)received.at(4) != 0x74 || (uint8_t)received.at(5) != 0x20 || (uint8_t)received.at(6) != 0x01 || (uint8_t)received.at(7) != 0x04)
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
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0xB7);
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

        output[5] = (uint8_t)((addr >> 16) & 0xFF);
        output[6] = (uint8_t)((addr >> 8) & 0xFF);
        output[7] = (uint8_t)(addr & 0xFF);
        send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);
        serial->write_serial_data_echo_check(output);
        //qDebug() << "0xB7 message sent to kernel to dump 256 bytes";
        delay(200);
        received = serial->read_serial_data(270, 200);
        //qDebug() << "Response to 0xB7 (dump mem) message:" << parse_message_to_hex(received);
        send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
        if ((uint8_t)received.at(4) != 0xF7)
        {
            send_log_window_message("Page data request failed!", true, true);
            //return STATUS_ERROR;
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
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x37);

    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        delay(200);
        received = serial->read_serial_data(270, 200);
        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x37 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x37 response:" << parse_message_to_hex(received);
        }
        try_count++;
        //delay(try_timeout);
    }

    // now decrypt the data obtained
    mapdata = subaru_tcu_hitachi_decrypt_32bit_payload(mapdata, mapdata.length());

    // need to pad out first 0x8000 bytes with 0x00
    int i;
    QByteArray padBytes;
    for (i = 0; i < 0x8000; i++)
    {
        padBytes[i] = (uint8_t)0x00;
    }
    mapdata = mapdata.insert(0, padBytes);
    //send_log_window_message("Received mapdata: " + parse_message_to_hex(mapdata), true, true);

    ecuCalDef->FullRomData = mapdata;
    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*
 * Write memory to Subaru Hitachi CAN 32bit TCUs, on board kernel
 *
 * @return success
 */

int FlashTcuCvtSubaruHitachiM32rCan::write_mem_subaru_tcu_hitachi_can(bool test_write)
{
    QByteArray filedata;

    filedata = ecuCalDef->FullRomData;

    uint8_t data_array[filedata.length()];

    int block_modified[16] = {0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0};   // assume blocks after 0x8000 are modified

    unsigned bcnt;  // 13 blocks in M32R_512kB, but kernelmemorymodels.h has 11. Of these 11, the first 3 are not flashed by OBK
    unsigned blockno;

    //encrypt the data
    filedata = subaru_tcu_hitachi_encrypt_32bit_payload(filedata, filedata.length());

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
        send_log_window_message("--- erasing TCU flash memory ---", true, true);
        if (erase_subaru_tcu_hitachi_can())
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
                if (reflash_block_subaru_tcu_hitachi_can(&data_array[flashdevices[mcu_type_index].fblocks->start], &flashdevices[mcu_type_index], blockno, test_write))
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
 * Upload kernel to Subaru Denso CAN (iso15765) 32bit ECUs
 *
 * @return success
 */
int FlashTcuCvtSubaruHitachiM32rCan::reflash_block_subaru_tcu_hitachi_can(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write)
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
    maxblocks = pl_len / 128;
    end_addr = (start_address + (maxblocks * 128)) & 0xFFFFFFFF;
    uint32_t data_len = end_addr - start_address;

    QString start_addr = QString("%1").arg((uint32_t)start_address,8,16,QLatin1Char('0')).toUpper();
    QString length = QString("%1").arg((uint32_t)pl_len,8,16,QLatin1Char('0')).toUpper();
    msg = QString("Flash block addr: 0x" + start_addr + " len: 0x" + length).toUtf8();
    send_log_window_message(msg, true, true);

    send_log_window_message("Settting flash start & length...", true, true);
    qDebug() << "Settting flash start & length...";

    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x34);
    output.append((uint8_t)0x04);
    output.append((uint8_t)0x33);
    output.append((uint8_t)((start_address >> 16) & 0xFF));
    output.append((uint8_t)((start_address >> 8) & 0xFF));
    output.append((uint8_t)(start_address & 0xFF));
    output.append((uint8_t)((data_len >> 16) & 0xFF));
    output.append((uint8_t)((data_len >> 8) & 0xFF));
    output.append((uint8_t)(data_len & 0xFF));

    bool connected = false;
    int try_count = 0;
    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        delay(200);
        received = serial->read_serial_data(20, 200);
        if (received != "")
        {
            connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x34 0x04 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x34 0x04 response:" << parse_message_to_hex(received);
        }
        try_count++;
        //delay(try_timeout);
    }
    if (received == "" || (uint8_t)received.at(4) != 0x74)
        send_log_window_message("No or bad response received", true, true);
        //return STATUS_ERROR;

    int data_bytes_sent = 0;
    for (blockctr = 0; blockctr < maxblocks; blockctr++)
    {
        if (kill_process)
            return 0;

        blockaddr = start_address + blockctr * 128;
        output.clear();
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x00);
        output.append((uint8_t)0x07);
        output.append((uint8_t)0xE1);
        output.append((uint8_t)0xB6);
        output.append((uint8_t)(blockaddr >> 16) & 0xFF);
        output.append((uint8_t)(blockaddr >> 8) & 0xFF);
        output.append((uint8_t)blockaddr & 0xFF);
        //qDebug() << "Data header:" << parse_message_to_hex(output);

        for (int i = 0; i < 128; i++)
        {
            output.append((uint8_t)(newdata[i + blockaddr] & 0xFF));
            //output[i + 8] = (uint8_t)(newdata[i + blockaddr] & 0xFF);
            data_bytes_sent++;
        }
        data_len -= 128;

        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        //qDebug() << "Kernel data:" << parse_message_to_hex(output);
        //delay(20);
        delay(200);
        received = serial->read_serial_data(5, receive_timeout);

        float pleft = (float)blockctr / (float)maxblocks * 100;
        set_progressbar_value(pleft);
    }
    qDebug() << "Data bytes sent:" << hex << data_bytes_sent;

    send_log_window_message("Closing out Flashing of this block...", true, true);
    qDebug() << "Closing out Flashing of this block...";

    connected = false;
    try_count = 0;
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
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
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x01);

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
            send_log_window_message(QString::number(try_count) + ": 0x31 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x31 response:" << parse_message_to_hex(received);
        }
        try_count++;
        //delay(try_timeout);
    }
    if (received == "" || (uint8_t)received.at(4) != 0x71 || (uint8_t)received.at(5) != 0x02 || (uint8_t)received.at(6) != 0x02)
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
int FlashTcuCvtSubaruHitachiM32rCan::erase_subaru_tcu_hitachi_can()
{
    QByteArray output;
    QByteArray received;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    send_log_window_message("Erasing TCU ROM...", true, true);
    qDebug() << "Erasing TCU ROM...";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE1);
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0xff);
    output.append((uint8_t)0xff);
    output.append((uint8_t)0xff);
    output.append((uint8_t)0xff);

    send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);
    serial->write_serial_data_echo_check(output);
    delay(500);

    // should there be a QTime delay to wait for erasing process?

    received = serial->read_serial_data(20, 200);
    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
    if ((uint8_t)received.at(4) != 0x31 || (uint8_t)received.at(5) != 0x02 || (uint8_t)received.at(6) != 0x01)
    {
        send_log_window_message("Erasing error! Do not panic, do not reset the TCU immediately. The kernel is most likely still running and receiving commands!", true, true);
        //return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
}

// CVT version
//
//
//

/*
 * Generate cvt tcu hitachi can seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashTcuCvtSubaruHitachiM32rCan::subaru_tcu_hitachi_generate_can_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex[]={
        0x9E99, 0x685C, 0x874D, 0xF11E,
        0x27D4, 0xA967, 0xB63B, 0x7A37,
        0xE23B, 0xA8D0, 0x9B82, 0xAC43,
        0xE874, 0x7FC5, 0x7141, 0x8B44
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = subaru_tcu_hitachi_calculate_seed_key(requested_seed, keytogenerateindex, indextransformation);

    return key;
}

/*
 * Calculate subaru tcu hitachi seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashTcuCvtSubaruHitachiM32rCan::subaru_tcu_hitachi_calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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


//
//
//
// end CVT version


/*
 * Encrypt upload data
 *
 * @return encrypted data
 */

QByteArray FlashTcuCvtSubaruHitachiM32rCan::subaru_tcu_hitachi_encrypt_32bit_payload(QByteArray buf, uint32_t len)
{
    QByteArray encrypted;

    const uint16_t keytogenerateindex[]={
        0x3B61, 0x8BEF, 0x9E51, 0x1075
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    encrypted = subaru_tcu_hitachi_calculate_32bit_payload(buf, len, keytogenerateindex, indextransformation);

    return encrypted;
}

QByteArray FlashTcuCvtSubaruHitachiM32rCan::subaru_tcu_hitachi_decrypt_32bit_payload(QByteArray buf, uint32_t len)
{
    QByteArray decrypt;

    const uint16_t keytogenerateindex[]={
        0x1075, 0x9E51, 0x8BEF, 0x3B61
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    decrypt = subaru_tcu_hitachi_calculate_32bit_payload(buf, len, keytogenerateindex, indextransformation);

    return decrypt;
}

QByteArray FlashTcuCvtSubaruHitachiM32rCan::subaru_tcu_hitachi_calculate_32bit_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
QString FlashTcuCvtSubaruHitachiM32rCan::parse_message_to_hex(QByteArray received)
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
int FlashTcuCvtSubaruHitachiM32rCan::send_log_window_message(QString message, bool timestamp, bool linefeed)
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

void FlashTcuCvtSubaruHitachiM32rCan::set_progressbar_value(int value)
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

void FlashTcuCvtSubaruHitachiM32rCan::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}


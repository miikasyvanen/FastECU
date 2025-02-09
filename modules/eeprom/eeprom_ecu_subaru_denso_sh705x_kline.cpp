#include "eeprom_ecu_subaru_denso_sh705x_kline.h"

EepromEcuSubaruDensoSH705xKline::EepromEcuSubaruDensoSH705xKline(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::EcuOperationsWindow)
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

void EepromEcuSubaruDensoSH705xKline::run()
{
    this->show();

    int result = STATUS_ERROR;
    set_progressbar_value(0);

    EEPROM_MODE = 2;

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

    int ret = QMessageBox::information(this, tr("Connecting to ECU"),
                                       tr("Downloading EEPROM content. There is 3 different option depends on "
                                          "ECU. All 3 option shows content on screen and you can save it when "
                                          "it looks ok.\n\n"
                                          "Turn ignition ON and press OK to start initializing connection to ECU"),
                                       QMessageBox::Ok | QMessageBox::Cancel,
                                       QMessageBox::Ok);

    switch (ret)
    {
    case QMessageBox::Ok:
        for (int i = 2; i < 5; i++)
        {
            bool save_and_exit = false;

            send_log_window_message("Connecting to Subaru 07+ 32-bit CAN bootloader, please wait...", true, true);
            result = connect_bootloader_subaru_denso_kline_04_32bit();

            if (result == STATUS_SUCCESS && !kernel_alive)
            {
                emit external_logger("Preparing, please wait...");
                send_log_window_message("Initializing Subaru 07+ 32-bit CAN kernel upload, please wait...", true, true);
                result = upload_kernel_subaru_denso_kline_04_32bit(kernel, ecuCalDef->KernelStartAddr.toUInt(&ok, 16));
            }
            if (result == STATUS_SUCCESS)
            {
                if (cmd_type == "read")
                {
                    emit external_logger("Reading ROM, please wait...");
                    send_log_window_message("Reading EEPROM from Subaru 07+ 32-bit using CAN", true, true);
                    qDebug() << "Reading EEPROM start at:" << flashdevices[mcu_type_index].eblocks[0].start << "and size of" << flashdevices[mcu_type_index].eblocks[0].len;
                    result = read_mem_subaru_denso_kline_32bit(flashdevices[mcu_type_index].eblocks[0].start, flashdevices[mcu_type_index].eblocks[0].len);
                }
                else if (cmd_type == "test_write" || cmd_type == "write")
                {
                    emit external_logger("Writing ROM, please wait...");
                    send_log_window_message("Writing ROM to Subaru 07+ 32-bit using CAN", true, true);
                    //result = write_mem(ecuCalDef, test_write);
                }
            }
            emit external_logger("Finished");

            if (result == STATUS_SUCCESS)
            {
                int ret = QMessageBox::information(this, tr("Downloaded EEPROM content"),
                                                   tr("If downloaded content looks ok, click 'Save' to save content and exit, otherwise click 'discard' and continue with next method."),
                                                   QMessageBox::Save | QMessageBox::Ignore,
                                                   QMessageBox::Save);

                switch (ret)
                {
                case QMessageBox::Save:
                    save_and_exit = true;
                    break;
                case QMessageBox::Ignore: {
                    result = STATUS_ERROR;
                    ecuCalDef->FullRomData.clear();
                    EEPROM_MODE++;
                    int ret = QMessageBox::warning(this, tr("Connecting to ECU"),
                                                   tr("Turn ignition OFF and back ON and press OK to start initializing connection to ECU"),
                                                   QMessageBox::Ok | QMessageBox::Cancel,
                                                   QMessageBox::Ok);

                    switch (ret)
                    {
                    case QMessageBox::Ok:

                        break;
                    case QMessageBox::Cancel:
                        save_and_exit = true;
                        break;
                    default:
                        // should never be reached
                        break;
                    }
                    break;
                }
                default:
                    // should never be reached
                    break;
                }
            }
            if (save_and_exit)
                break;
        }
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
    if (result != STATUS_SUCCESS)
    {
        QMessageBox::warning(this, tr("ECU Operation"), "ECU operation failed, press OK to exit and try again");
    }
}

EepromEcuSubaruDensoSH705xKline::~EepromEcuSubaruDensoSH705xKline()
{
    delete ui;
}

void EepromEcuSubaruDensoSH705xKline::closeEvent(QCloseEvent *bar)
{
    kill_process = true;
}

/*
 * Connect to Subaru Denso K-Line bootloader 32bit ECUs
 *
 * @return success
 */
int EepromEcuSubaruDensoSH705xKline::connect_bootloader_subaru_denso_kline_04_32bit()
{
    QByteArray output;
    QByteArray received;
    QByteArray seed;
    QByteArray seed_key;

    QString msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    //if (connect_bootloader_start_countdown(bootloader_start_countdown))
    //    return STATUS_ERROR;

    serial->change_port_speed("62500");
    serial->set_add_iso14230_header(true);
    serial->set_iso14230_startbyte(0x80);
    serial->set_iso14230_tester_id(0xFC);
    serial->set_iso14230_target_id(0x10);

    delay(100);

    send_log_window_message("Checking if kernel is already running...", true, true);
    qDebug() << "Checking if kernel is already running...";

    received = request_kernel_init();
    if (received.length())
    {
        if ((uint8_t)received.at(1) == SID_KERNEL_INIT + 0x40)
        {
            send_log_window_message("Kernel already running, requesting kernel ID", true, true);
            delay(100);
            delay(100);

            received = request_kernel_id();
            if (received == "")
                return STATUS_ERROR;

            received.remove(0, 2);
            send_log_window_message("Request kernel id ok: " + received, true, true);
            kernel_alive = true;
            return STATUS_SUCCESS;
        }
    }
    send_log_window_message("Kernel NOT running, requesting access", true, true);

    serial->change_port_speed("4800");
    serial->set_add_iso14230_header(false);
    delay(100);

    send_log_window_message("Initializing bootloader", true, true);
    qDebug() << "Initializing bootloader";

    // SSM init
    received = send_sid_bf_ssm_init();
    if (received == "")
        return STATUS_ERROR;
    //qDebug() << "SID_BF received:" << parse_message_to_hex(received);
    received.remove(0, 8);
    received.remove(5, received.length() - 5);
    //qDebug() << "Received length:" << received.length();
    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
    }
    QString ecuid = msg;
    //QString ecuid = QString("%1%2%3%4%5").arg(received.at(0),2,16,QLatin1Char('0')).toUpper().arg(received.at(1),2,16,QLatin1Char('0')).arg(received.at(2),2,16,QLatin1Char('0')).arg(received.at(3),2,16,QLatin1Char('0')).arg(received.at(4),2,16,QLatin1Char('0'));

    send_log_window_message("ECU ID = " + ecuid, true, true);
    qDebug() << "ECU ID = " << ecuid;

    // Start communication
    received = send_sid_81_start_communication();
    //send_log_window_message("SID_81 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0xC1)
        return STATUS_ERROR;

    send_log_window_message("Start communication ok", true, true);
    qDebug() << "Start communication ok";

    // Request timing parameters
    received = send_sid_83_request_timings();
    //send_log_window_message("SID_83 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0xC3)
        return STATUS_ERROR;

    send_log_window_message("Request timing parameters ok", true, true);
    qDebug() << "Request timing parameters ok";

    // Request seed
    received = send_sid_27_request_seed();
    //send_log_window_message("SID_27_01 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0x67)
        return STATUS_ERROR;

    send_log_window_message("Seed request ok", true, true);
    qDebug() << "Seed request ok";

    seed.append(received.at(6));
    seed.append(received.at(7));
    seed.append(received.at(8));
    seed.append(received.at(9));

    if (flash_method.endsWith("_ecutek"))
        seed_key = subaru_denso_generate_ecutek_kline_seed_key(seed);
    else
        seed_key = subaru_denso_generate_kline_seed_key(seed);
    msg = parse_message_to_hex(seed_key);
    //qDebug() << "Calculated seed key: " + msg + ", sending to ECU";
    //send_log_window_message("Calculated seed key: " + msg + ", sending to ECU", true, true);
    send_log_window_message("Sending seed key", true, true);
    qDebug() << "Sending seed key";

    received = send_sid_27_send_seed_key(seed_key);
    //send_log_window_message("SID_27_02 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0x67)
        return STATUS_ERROR;

    send_log_window_message("Seed key ok", true, true);
    qDebug() << "Seed key ok";

    // Start diagnostic session
    received = send_sid_10_start_diagnostic();
    if (received == "" || (uint8_t)received.at(4) != 0x50)
        return STATUS_ERROR;

    send_log_window_message("Start diagnostic session ok", true, true);
    qDebug() << "Start diagnostic session ok";

    return STATUS_SUCCESS;
}

/*
 * Upload kernel to Subaru Denso K-Line 32bit ECUs
 *
 * @return success
 */
int EepromEcuSubaruDensoSH705xKline::upload_kernel_subaru_denso_kline_04_32bit(QString kernel, uint32_t kernel_start_addr)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray payload;
    QByteArray received;
    QByteArray msg;
    QByteArray pl_encr;
    uint32_t file_len = 0;
    uint32_t pl_len = 0;
    uint32_t start_address = 0;
    uint32_t len = 0;
    QByteArray cks_bypass;
    uint8_t chk_sum = 0;

    QString mcu_name;

    start_address = kernel_start_addr;//flashdevices[mcu_type_index].kblocks->start;
    qDebug() << "Start address to upload kernel:" << hex << start_address;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->set_add_iso14230_header(false);

    // Check kernel file
    if (!file.open(QIODevice::ReadOnly ))
    {
        send_log_window_message("Unable to open kernel file for reading", true, true);
        return STATUS_ERROR;
    }
    file_len = file.size();
    pl_len = (file_len + 3) & ~3;
    pl_encr = file.readAll();
    len = pl_len &= ~3;

    if (file_len != pl_len)
    {
        //send_log_window_message("Using " + QString::number(file_len) + " byte payload, padding with garbage to " + QString::number(pl_len) + " (" + QString::number(file_len) + ") bytes.\n", true, true);
    }
    else
    {
        //send_log_window_message("Using " + QString::number(file_len) + " (" + QString::number(file_len) + ") byte payload.", true, true);
    }
    if ((uint32_t)pl_encr.size() != file_len)
    {
        send_log_window_message("File size error (" + QString::number(file_len) + "/" + QString::number(pl_encr.size()) + ")", true, true);
        return STATUS_ERROR;
    }

    // Change port speed to upload kernel
    qDebug() << "Change port speed to 15625";
    if (serial->change_port_speed("15625"))
        return STATUS_ERROR;

    // Request upload
    //send_log_window_message("Send 'sid34_request_upload'", true, true);
    //qDebug() << "Send 'sid34_request_upload'";
    received = send_sid_34_request_upload(start_address, len);
    if (received == "" || (uint8_t)received.at(4) != 0x74)
        return STATUS_ERROR;

    send_log_window_message("Kernel upload request ok, uploading now, please wait...", true, true);
    qDebug() << "Kernel upload request ok, uploading now, please wait...";

    //qDebug() << "Encrypt kernel data before upload";
    //pl_encr = sub_encrypt_buf(pl_encr, (uint32_t) pl_len);
    pl_encr = encrypt_payload(pl_encr, pl_len);

    //send_log_window_message("Send 'sid36_transfer_data'", true, true);
    //qDebug() << "Send 'sid36_transfer_data'";
    received = send_sid_36_transferdata(start_address, pl_encr, len);
    if (received == "" || (uint8_t)received.at(4) != 0x76)
        return STATUS_ERROR;

    send_log_window_message("Kernel uploaded", true, true);
    qDebug() << "Kernel uploaded";

    /* sid34 request upload - checksum bypass put just after payload */
    //send_log_window_message("Send 'sid34_transfer_data'", true, true);
    //qDebug() << "Send 'sid34_transfer_data' for chksum bypass";
    received = send_sid_34_request_upload(start_address + len, 4);
    if (received == "" || (uint8_t)received.at(4) != 0x74)
        return STATUS_ERROR;

    cks_bypass.append((uint8_t)0x00);
    cks_bypass.append((uint8_t)0x00);
    cks_bypass.append((uint8_t)0x5A);
    cks_bypass.append((uint8_t)0xA5);

    cks_bypass = encrypt_payload(cks_bypass, (uint32_t) 4);

    /* sid36 transferData for checksum bypass */
    //qDebug() << "Send 'sid36_transfer_data' for chksum bypass";
    received = send_sid_36_transferdata(start_address + len, cks_bypass, 4);
    if (received == "" || (uint8_t)received.at(4) != 0x76)
        return STATUS_ERROR;

    send_log_window_message("Checksum bypass ok", true, true);
    qDebug() << "Checksum bypass ok";

    /* SID 37 TransferExit does not exist on all Subaru ROMs */

    /* RAMjump ! */
    //qDebug() << "Send 'sid31_transfer_data' to jump to kernel";
    received = send_sid_31_start_routine();
    if (received == "" || (uint8_t)received.at(4) != 0x71)
        return STATUS_ERROR;

    send_log_window_message("Kernel started, initializing...", true, true);
    qDebug() << "Kernel started, initializing...";

    serial->change_port_speed("62500");
    serial->set_add_iso14230_header(true);
    serial->set_iso14230_startbyte(0x80);
    serial->set_iso14230_tester_id(0xFC);
    serial->set_iso14230_target_id(0x10);

    delay(100);

    received = request_kernel_init();
    if (received == "")
    {
        send_log_window_message("Kernel init NOK! No response from kernel" + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }
    if ((uint8_t)received.at(1) != SID_KERNEL_INIT + 0x40)
    {
        send_log_window_message("Kernel init NOK! Got bad startcomm response from kernel" + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }
    else
    {
        send_log_window_message("Kernel init OK", true, true);
        qDebug() << "Kernel init OK";
    }

    delay(100);

    send_log_window_message("Requesting kernel ID", true, true);
    qDebug() << "Requesting kernel ID";

    received = request_kernel_id();
    if (received == "")
        return STATUS_ERROR;

    send_log_window_message("Kernel ID: " + received, true, true);
    qDebug() << "Kernel ID: " << parse_message_to_hex(received);

    return STATUS_SUCCESS;
}

/*
 * Read memory from Subaru Denso K-Line 32bit ECUs, nisprog kernel
 *
 * @return success
 */
int EepromEcuSubaruDensoSH705xKline::read_mem_subaru_denso_kline_32bit(uint32_t start_addr, uint32_t length)
{
    QElapsedTimer timer;
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray pagedata;
    QByteArray mapdata;
    uint32_t cplen = 0;
    uint32_t timeout = 0;

    uint32_t skip_start = 0;
    uint32_t addr = 0;
    uint32_t willget = 0;
    uint32_t len_done = 0;  //total data written to file

    skip_start = start_addr & (32 - 1); //if unaligned, we'll be receiving this many extra bytes
    addr = start_addr - skip_start;
    willget = (skip_start + length + 31) & ~(32 - 1);
    len_done = 0;  //total data written to file

    qDebug() << "Read EEPROM start at:" << start_addr << "and size of" << length;

    #define NP10_MAXBLKS    32   //# of blocks to request per loop. Too high might flood us
    serial->set_add_iso14230_header(true);

    output.clear();
    output.append((uint8_t)SID_DUMP);
    output.append((uint8_t)EEPROM_MODE);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    timer.start();
    set_progressbar_value(0);

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

        uint32_t pagesize = numblocks * 32 + numblocks * 3;
        pleft = (float)(addr - start_addr) / (float)length * 100.0f;
        set_progressbar_value(pleft);

        //qDebug() << "Read" << numblocks * 32 << "bytes starting at" << curblock *8;
        output[2] = numblocks >> 8;
        output[3] = numblocks >> 0;

        output[4] = curblock >> 8;
        output[5] = curblock >> 0;

        received = serial->write_serial_data_echo_check(output);
        delay(500);
        // Receive map data, check and remove header // ADJUST THIS LATER //
        //received.clear();
        //received = serial->read_serial_data(pagesize, serial_read_timeout);
        //qDebug() << "Read received:" << parse_message_to_hex(received);
        timeout = 0;
        pagedata.clear();
        while ((uint32_t)pagedata.length() < pagesize && timeout < 5)
        {
            if (kill_process)
                return STATUS_ERROR;
            delay(100);
            //received = serial->write_serial_data_echo_check(output);
            received = serial->read_serial_data(pagesize / numblocks, serial_read_timeout);
            //qDebug() << "Read received:" << parse_message_to_hex(received);
            if (received.length()) {
                pagedata.append(received);
            }
            else
                timeout++;

            //qDebug() << "Pagedata length:" << pagedata.length() << "/" << pagesize;
            received.clear();
        }
        for (uint32_t i = 0; i < numblocks; i++)
        {
            pagedata.remove(i * 32, 2);
            //mapdata.append(received, 32);
            pagedata.remove((i + 1) * 32, 1);
            send_log_window_message(parse_message_to_hex(pagedata.mid(i * 32, 32)), false, true);
            qDebug() << parse_message_to_hex(pagedata.mid(i * 32, 32));
        }
        //qDebug() << "Read received:" << parse_message_to_hex(pagedata);
        if (timeout >= 1000)
        {
            send_log_window_message("Page data timeout!", true, true);
            qDebug() << "Page data timeout!";
            return STATUS_ERROR;
        }
        mapdata.append(pagedata);

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















/*
 * 8bit checksum
 *
 * @return
 */
uint8_t EepromEcuSubaruDensoSH705xKline::cks_add8(QByteArray chksum_data, unsigned len)
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
 * CRC16 implementation adapted from Lammert Bies
 *
 * @return
 */
#define NPK_CRC16   0xBAAD  //koopman, 2048bits (256B)
static bool crc_tab16_init = 0;
static uint16_t crc_tab16[256];
void EepromEcuSubaruDensoSH705xKline::init_crc16_tab(void)
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

uint16_t EepromEcuSubaruDensoSH705xKline::crc16(const uint8_t *data, uint32_t siz)
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











/*
 * ECU init
 *
 * @return ECU ID and capabilities
 */
QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_bf_ssm_init()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;


    output.append((uint8_t)0xBF);

    while (received == "" && loop_cnt < 5)
    {
        if (kill_process)
            break;

        send_log_window_message("SSM init", true, true);
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
QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_81_start_communication()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

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
QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_83_request_timings()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

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
QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_27_request_seed()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

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
QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_27_send_seed_key(QByteArray seed_key)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

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
QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_10_start_diagnostic()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

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
QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_34_request_upload(uint32_t dataaddr, uint32_t datalen)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

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
QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_36_transferdata(uint32_t dataaddr, QByteArray buf, uint32_t len)
{
    QByteArray output;
    QByteArray received;
    uint32_t blockaddr = 0;
    uint16_t blockno = 0;
    uint16_t maxblocks = 0;
    uint8_t loop_cnt = 0;

    len &= ~0x03;
    if (!buf.length() || !len) {
        send_log_window_message("Error in kernel data length!", true, true);
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

QByteArray EepromEcuSubaruDensoSH705xKline::send_sid_31_start_routine()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x01);
    serial->write_serial_data_echo_check(add_ssm_header(output, tester_id, target_id, false));
    received = serial->read_serial_data(8, receive_timeout);

    return received;
}

/*
 * Generate denso kline seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray EepromEcuSubaruDensoSH705xKline::subaru_denso_generate_kline_seed_key(QByteArray requested_seed)
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
QByteArray EepromEcuSubaruDensoSH705xKline::subaru_denso_generate_ecutek_kline_seed_key(QByteArray requested_seed)
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
QByteArray EepromEcuSubaruDensoSH705xKline::calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
QByteArray EepromEcuSubaruDensoSH705xKline::encrypt_payload(QByteArray buf, uint32_t len)
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

QByteArray EepromEcuSubaruDensoSH705xKline::decrypt_payload(QByteArray buf, uint32_t len)
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

QByteArray EepromEcuSubaruDensoSH705xKline::calculate_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
QByteArray EepromEcuSubaruDensoSH705xKline::request_kernel_init()
{
    QByteArray output;
    QByteArray received;

    request_denso_kernel_init = true;

    output.clear();
    output.append(SID_KERNEL_INIT);
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
QByteArray EepromEcuSubaruDensoSH705xKline::request_kernel_id()
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
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray EepromEcuSubaruDensoSH705xKline::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
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
uint8_t EepromEcuSubaruDensoSH705xKline::calculate_checksum(QByteArray output, bool dec_0x100)
{
    uint8_t checksum = 0;

    for (uint16_t i = 0; i < output.length(); i++)
        checksum += (uint8_t)output.at(i);

    if (dec_0x100)
        checksum = (uint8_t) (0x100 - checksum);

    return checksum;
}

/*
 * Countdown prior power on
 *
 * @return
 */
int EepromEcuSubaruDensoSH705xKline::connect_bootloader_start_countdown(int timeout)
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
QString EepromEcuSubaruDensoSH705xKline::parse_message_to_hex(QByteArray received)
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
int EepromEcuSubaruDensoSH705xKline::send_log_window_message(QString message, bool timestamp, bool linefeed)
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

void EepromEcuSubaruDensoSH705xKline::set_progressbar_value(int value)
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

void EepromEcuSubaruDensoSH705xKline::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

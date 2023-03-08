#include <modules/flash_denso_kline_04.h>
#include <ui_ecu_operations.h>

FlashDensoKline04::FlashDensoKline04(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
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
    this->show();

    int result = 0;
    set_progressbar_value(0);

    init_flash_sti04(ecuCalDef, cmd_type);

    if (result == STATUS_SUCCESS)
    {
        QMessageBox::information(this, tr("ECU Operation"), "ECU operation was succesful, press OK to exit");
        this->close();
    }
    else
    {
        QMessageBox::warning(this, tr("ECU Operation"), "ECU operation failed, press OK to exit and try again");
    }
}

FlashDensoKline04::~FlashDensoKline04()
{

}

void FlashDensoKline04::closeEvent(QCloseEvent *bar)
{
    kill_process = true;
}

int FlashDensoKline04::init_flash_sti04(FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type)
{
    mcu_type_string = "SH7055";
    mcu_type_index = 0;
    tester_id = 0xF0;
    target_id = 0x10;

    while (flashdevices[mcu_type_index].name != 0)
    {
        if (flashdevices[mcu_type_index].name == mcu_type_string)
            break;
        mcu_type_index++;
    }
    QString mcu_name = flashdevices[mcu_type_index].name;
    //send_log_window_message("MCU type: " + mcu_name + " and index: " + mcu_type_index, true, true);
    //qDebug() << "MCU type:" << mcu_name << "and index:" << mcu_type_index;

    int result = STATUS_ERROR;

    flash_method = ecuCalDef->RomInfo.at(FileActions::FlashMethod);
    kernel = ecuCalDef->Kernel;

    if (cmd_type == "read")
    {
        send_log_window_message("Read memory with flashmethod " + flash_method + " and kernel " + ecuCalDef->Kernel, true, true);
        //qDebug() << "Read memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
    }
    else if (cmd_type == "test_write")
    {
        test_write = true;
        send_log_window_message("Test write memory with flashmethod " + flash_method + " and kernel " + ecuCalDef->Kernel, true, true);
        //qDebug() << "Test write memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
    }
    else if (cmd_type == "write")
    {
        test_write = false;
        send_log_window_message("Write memory with flashmethod " + flash_method + " and kernel " + ecuCalDef->Kernel, true, true);
        //qDebug() << "Write memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;
    }

    QMessageBox::information(this, tr("Connecting to ECU"), "Press OK to start write countdown!");

    if (serial->is_can_connection)
    {
        serial->is_can_connection = true;
        serial->is_iso15765_connection = false;
        serial->is_29_bit_id = true;
        serial->can_speed = "500000";
        serial->can_source_address = 0xFFFFE;
        serial->can_destination_address = 0x21;
        serial->open_serial_port();

        send_log_window_message("Connecting to Subaru 04 32-bit CAN bootloader, please wait...", true, true);
        result = connect_bootloader_subaru_denso_can_04_32bit();
    }
    else
    {
        serial->is_can_connection = false;
        serial->is_iso15765_connection = false;
        serial->is_iso14230_connection = true;
        serial->open_serial_port();

        send_log_window_message("Connecting to Subaru 04 32-bit K-line bootloader, please wait...", true, true);
        result = connect_bootloader_subaru_denso_kline_04_32bit();
    }
    if (result == STATUS_SUCCESS && !kernel_alive)
    {
        if (serial->is_can_connection)
        {
            send_log_window_message("Initializing Subaru 04 32-bit CAN kernel upload, please wait...", true, true);
            result = upload_kernel_subaru_denso_can_04_32bit(kernel);
        }
        else
        {
            send_log_window_message("Initializing Subaru 04 32-bit K-Line kernel upload, please wait...", true, true);
            result = upload_kernel_subaru_denso_kline_04_32bit(kernel);
        }
    }
    if (result == STATUS_SUCCESS)
    {
        if (cmd_type == "read")
        {
            if (serial->is_can_connection)
            {
                send_log_window_message("Reading ROM from Subaru 04 32-bit using CAN", true, true);
                result = read_mem_subaru_denso_can_32bit(ecuCalDef, flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
            }
            else
            {
                send_log_window_message("Reading ROM from Subaru 04 32-bit using K-Line", true, true);
                result = read_mem_subaru_denso_kline_32bit(ecuCalDef, flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);
            }
        }
        else if (cmd_type == "test_write" || cmd_type == "write")
        {
            if (serial->is_can_connection)
            {
                send_log_window_message("Writing ROM to Subaru 04 32-bit using CAN", true, true);
                result = write_mem_subaru_denso_can_32bit(ecuCalDef, test_write);
            }
            else
            {
                send_log_window_message("Writing ROM to Subaru 04 32-bit using K-Line", true, true);
                result = write_mem_subaru_denso_kline_32bit(ecuCalDef, test_write);
            }
        }
    }
    return result;
}

/*
 * Connect to Subaru Denso K-Line bootloader 32bit ECUs
 *
 * @return success
 */
int FlashDensoKline04::connect_bootloader_subaru_denso_kline_04_32bit()
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

    if (connect_bootloader_start_countdown(bootloader_start_countdown))
        return STATUS_ERROR;

    serial->change_port_speed("62500");
    serial->is_packet_header = true;
    serial->packet_header_startbyte = 0x80;
    serial->packet_header_source_id = 0xFC;
    serial->packet_header_destination_id = 0x10;

    delay(100);

    received = request_kernel_init();
    if (received.length() > 0)
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

    serial->change_port_speed("4800");
    serial->is_packet_header = false;
    delay(100);

    // SSM init
    received = send_subaru_sid_bf_ssm_init();
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

    // Start communication
    received = send_subaru_denso_sid_81_start_communication();
    //send_log_window_message("SID_81 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0xC1)
        return STATUS_ERROR;

    send_log_window_message("Start communication ok", true, true);

    // Request timing parameters
    received = send_subaru_denso_sid_83_request_timings();
    //send_log_window_message("SID_83 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0xC3)
        return STATUS_ERROR;

    send_log_window_message("Request timing parameters ok", true, true);

    // Request seed
    received = send_subaru_denso_sid_27_request_seed();
    //send_log_window_message("SID_27_01 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0x67)
        return STATUS_ERROR;

    send_log_window_message("Seed request ok", true, true);

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

    received = send_subaru_denso_sid_27_send_seed_key(seed_key);
    //send_log_window_message("SID_27_02 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0x67)
        return STATUS_ERROR;

    send_log_window_message("Seed key ok", true, true);

    // Start diagnostic session
    received = send_subaru_denso_sid_10_start_diagnostic();
    if (received == "" || (uint8_t)received.at(4) != 0x50)
        return STATUS_ERROR;

    send_log_window_message("Start diagnostic session ok", true, true);

    return STATUS_SUCCESS;
}

/*
 * Upload kernel to Subaru Denso K-Line 32bit ECUs
 *
 * @return success
 */
int FlashDensoKline04::upload_kernel_subaru_denso_kline_04_32bit(QString kernel)
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

    start_address = flashdevices[mcu_type_index].rblocks->start;
    //qDebug() << "Start address to upload kernel:" << hex << start_address;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->is_packet_header = false;

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
    //qDebug() << "Change port speed to 15625";
    if (serial->change_port_speed("15625"))
        return STATUS_ERROR;

    // Request upload
    //qDebug() << "Send 'sid34_request upload'";
    received = send_subaru_denso_sid_34_request_upload(start_address, len);
    if (received == "" || (uint8_t)received.at(4) != 0x74)
        return STATUS_ERROR;

    send_log_window_message("Kernel upload request ok, uploading now, please wait...", true, true);

    //qDebug() << "Encrypt kernel data before upload";
    //pl_encr = sub_encrypt_buf(pl_encr, (uint32_t) pl_len);
    pl_encr = subaru_denso_transform_32bit_payload(pl_encr, (uint32_t) pl_len);

    //qDebug() << "Send 'sid36_transfer_data'";
    received = send_subaru_denso_sid_36_transferdata(start_address, pl_encr, len);
    if (received == "" || (uint8_t)received.at(4) != 0x76)
        return STATUS_ERROR;

    send_log_window_message("Kernel uploaded", true, true);

    /* sid34 requestDownload - checksum bypass put just after payload */
    //qDebug() << "Send 'sid34_transfer_data' for chksum bypass";
    received = send_subaru_denso_sid_34_request_upload(start_address + len, 4);
    if (received == "" || (uint8_t)received.at(4) != 0x74)
        return STATUS_ERROR;

    //send_log_window_message("sid_34 checksum bypass ok", true, true);

    cks_bypass.append((uint8_t)0x00);
    cks_bypass.append((uint8_t)0x00);
    cks_bypass.append((uint8_t)0x5A);
    cks_bypass.append((uint8_t)0xA5);

    //qDebug() << parse_message_to_hex(sub_transform_denso_04_32bit_kernel(cks_bypass, (uint32_t) 4));
    //qDebug() << parse_message_to_hex(sub_encrypt_buf(cks_bypass, (uint32_t) 4));
    //cks_bypass = sub_encrypt_buf(cks_bypass, (uint32_t) 4);
    cks_bypass = subaru_denso_transform_32bit_payload(cks_bypass, (uint32_t) 4);

    /* sid36 transferData for checksum bypass */
    //qDebug() << "Send 'sid36_transfer_data' for chksum bypass";
    received = send_subaru_denso_sid_36_transferdata(start_address + len, cks_bypass, 4);
    if (received == "" || (uint8_t)received.at(4) != 0x76)
        return STATUS_ERROR;

    //send_log_window_message("sid_36 checksum bypass ok", true, true);

    /* SID 37 TransferExit does not exist on all Subaru ROMs */

    /* RAMjump ! */
    //qDebug() << "Send 'sid31_transfer_data' to jump to kernel";
    received = send_subaru_denso_sid_31_start_routine();
    if (received == "" || (uint8_t)received.at(4) != 0x71)
        return STATUS_ERROR;

    send_log_window_message("Kernel started, initializing...", true, true);

    serial->change_port_speed("62500");
    serial->is_packet_header = true;
    serial->packet_header_startbyte = 0x80;
    serial->packet_header_source_id = 0xFC;
    serial->packet_header_destination_id = 0x10;

    //serial->reset_connection();
    //serial->open_serial_port();

    delay(100);

    received = request_kernel_init();
    //qDebug() << "Kernel init response:" << parse_message_to_hex(received);
    if (received == "")
    {
        //qDebug() << "Kernel init NOK! No response from kernel" + parse_message_to_hex(received);
        send_log_window_message("Kernel init NOK! No response from kernel" + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }
    if ((uint8_t)received.at(1) != SID_KERNEL_INIT + 0x40)
    {
        //qDebug() << "Kernel init NOK! Got bad startcomm response from kernel" + parse_message_to_hex(received);
        send_log_window_message("Kernel init NOK! Got bad startcomm response from kernel" + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }
    else
    {
        //send_log_window_message("Kernel init OK", true, true);
    }

    //send_log_window_message("Requesting kernel ID", true, true);

    delay(100);

    received = request_kernel_id();
    if (received == "")
        return STATUS_ERROR;

    send_log_window_message("Kernel ID: " + received, true, true);

    return STATUS_SUCCESS;
}

/*
 * Read memory from Subaru Denso K-Line 32bit ECUs, nisprog kernel
 *
 * @return success
 */
int FlashDensoKline04::read_mem_subaru_denso_kline_32bit(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t start_addr, uint32_t length)
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

/*
 * Write memory to Subaru Denso K-Line 32bit ECUs, nisprog kernel
 *
 * @return success
 */
int FlashDensoKline04::write_mem_subaru_denso_kline_32bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
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

    if (get_changed_blocks_kline_32bit(data_array, block_modified))
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
                if (reflash_block_kline_32bit(&data_array[flashdevices[mcu_type_index].fblocks->start], &flashdevices[mcu_type_index], blockno, test_write))
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

        if (get_changed_blocks_kline_32bit(data_array, block_modified))
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

/*
 * Compare ROM 32bit K-Line ECUs, nisprog kernel
 *
 * @return
 */
int FlashDensoKline04::get_changed_blocks_kline_32bit(const uint8_t *src, int *modified)
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
        if (check_romcrc_kline_32bit(&src[bs], bs, blen, &modified[blockno])) {
            return -1;
        }
    }
    return 0;
}

/*
 * ROM CRC 32bit K-Line ECUs, nisprog kernel
 *
 * @return
 */
int FlashDensoKline04::check_romcrc_kline_32bit(const uint8_t *src, uint32_t start, uint32_t len, int *modified)
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
            serial->read_serial_data(100, serial_read_short_timeout);
            return -1;
        }

        //confirmed bad CRC, we can exit
        *modified = 1;

        return 0;
    }   //for

    send_log_window_message("\tYES", false, true);
    *modified = 0;
    serial->read_serial_data(100, serial_read_short_timeout);
    return 0;
}

/*
 *  Reflash ROM 32bit K-Line ECUs
 *
 *  @return
 */
int FlashDensoKline04::reflash_block_kline_32bit(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write)
{
    int errval;

    uint32_t start;
    uint32_t len;

    QByteArray output;
    QByteArray received;
    QByteArray msg;

    set_progressbar_value(0);

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
    errval = flash_block_kline_32bit(newdata, start, len);
    if (errval) {
        send_log_window_message("Reflash error! Do not panic, do not reset the ECU immediately. The kernel is most likely still running and receiving commands!", true, true);
        return STATUS_ERROR;
    }

    send_log_window_message("Flash block ok", true, true);

    return STATUS_SUCCESS;
}

/*
 * Flash block 32bit K-Line ECUs, nisprog kernel
 *
 * @return
 */
int FlashDensoKline04::flash_block_kline_32bit(const uint8_t *src, uint32_t start, uint32_t len)
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

        float pleft = (float)byteindex / (float)flashbytescount * 100.0f;
        set_progressbar_value(pleft);

        QString start_address = QString("%1").arg(start,8,16,QLatin1Char('0')).toUpper();
        msg = QString("writing chunk @ 0x%1 (%2\% - %3 B/s, ~ %4 s remaining)").arg(start_address).arg((unsigned) 100 * (len - remain) / len,1,10,QLatin1Char('0')).arg((uint32_t)curspeed,1,10,QLatin1Char('0')).arg(tleft,1,10,QLatin1Char('0')).toUtf8();
        send_log_window_message(msg, true, true);

    }   //while len

    send_log_window_message("npk_raw_flashblock: write complete.", true, true);
    received = serial->read_serial_data(100, serial_read_short_timeout);
    return 0;
}


/*
 * Connect to Subaru Denso CAN bootloader 32bit ECUs
 *
 * @return success
 */
int FlashDensoKline04::connect_bootloader_subaru_denso_can_04_32bit()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->is_packet_header = false;

    if (connect_bootloader_start_countdown(bootloader_start_countdown))
        return STATUS_ERROR;

    // Check if kernel already alive
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    output.append((uint8_t)(SID_START_COMM_CAN & 0xFF));
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(output);
    //send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    //qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 10);
    //qDebug() << "0x7A 0x00 response:" << parse_message_to_hex(received);
    //send_log_window_message("0x7A 0x00 response: " + parse_message_to_hex(received), true, true);
    if ((uint8_t)received.at(0) == 0x7F && (uint8_t)received.at(2) == 0x34)
    {
        send_log_window_message("Kernel already running", true, true);

        kernel_alive = true;
        return STATUS_SUCCESS;
    }

    output[4] = (uint8_t)((SID_ENTER_BL_CAN >> 8) & 0xFF);
    output[5] = (uint8_t)(SID_ENTER_BL_CAN & 0xFF);
    output[6] = (uint8_t)0x00;
    output[7] = (uint8_t)0x00;
    output[8] = (uint8_t)0x00;
    output[9] = (uint8_t)0x00;
    output[10] = (uint8_t)0x00;
    output[11] = (uint8_t)0x00;
    serial->write_serial_data_echo_check(output);
    //send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    //qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 10);
    //send_log_window_message("0xFF 0x86 response: " + parse_message_to_hex(received), true, true);
    //qDebug() << "0xFF 0x86 response:" << parse_message_to_hex(received);

    output[4] = (uint8_t)SID_START_COMM_CAN;
    output[5] = (uint8_t)(SID_CHECK_COMM_BL_CAN & 0xFF);
    output[6] = (uint8_t)0x00;
    output[7] = (uint8_t)0x00;
    output[8] = (uint8_t)0x00;
    output[9] = (uint8_t)0x00;
    output[10] = (uint8_t)0x00;
    output[11] = (uint8_t)0x00;
    serial->write_serial_data_echo_check(output);
    //send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    //qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 10);
    //send_log_window_message("0x7A 0x90 response: " + parse_message_to_hex(received), true, true);
    //qDebug() << "0x7A 0x90 response:" << parse_message_to_hex(received);
    if ((uint8_t)(received.at(1) & 0xF8) == 0x90)
    {
        send_log_window_message("Connected to bootloader, start kernel upload", true, true);
        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}

/*
 * Upload kernel to Subaru Denso CAN 32bit ECUs
 *
 * @return success
 */
int FlashDensoKline04::upload_kernel_subaru_denso_can_04_32bit(QString kernel)
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
    uint32_t end_address = 0;
    uint32_t len = 0;
    QByteArray cks_bypass;
    uint8_t chk_sum = 0;
    int maxblocks = 0;
    int byte_counter = 0;

    QString mcu_name;

    start_address = flashdevices[mcu_type_index].rblocks->start;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->is_packet_header = false;

    // Check kernel file
    if (!file.open(QIODevice::ReadOnly ))
    {
        send_log_window_message("Unable to open kernel file for reading", true, true);
        return STATUS_ERROR;
    }
    file_len = file.size();
    pl_len = file_len + 6;
    pl_encr = file.readAll();
    maxblocks = file_len / 6;
    if((file_len % 6) != 0) maxblocks++;
    end_address = (start_address + (maxblocks * 6)) & 0xFFFFFFFF;

    send_log_window_message("Start sending kernel... please wait...", true, true);
    // Send kernel address
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    output.append((uint8_t)SID_START_COMM_CAN);
    output.append((uint8_t)(SID_KERNEL_ADDRESS + 0x04));
    output.append((uint8_t)((start_address >> 24) & 0xFF));
    output.append((uint8_t)((start_address >> 16) & 0xFF));
    output.append((uint8_t)((start_address >> 8) & 0xFF));
    output.append((uint8_t)(start_address & 0xFF));
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    received = serial->write_serial_data_echo_check(output);
    qDebug() << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 10);
    //qDebug() << "0x7A 0x9C response:" << parse_message_to_hex(received);

    //qDebug() << "Kernel start address:" << hex << start_address << ", blocks:" << hex << maxblocks << ", end address:" << hex << end_address;

    output[5] = (uint8_t)(0xA8 + 0x06);

    set_progressbar_value(0);

    for(int blockno = 0; blockno < maxblocks; blockno++)
    {
        for(int j = 0; j < 6; j++){

            output[6 + j] = pl_encr.at(byte_counter + j);
            chk_sum += (pl_encr.at(byte_counter + j) & 0xFF);
            chk_sum = ((chk_sum >> 8) & 0xFF) + (chk_sum & 0xFF);

        }

        byte_counter += 6;
        received = serial->write_serial_data_echo_check(output);
        //qDebug() << "0xA8 message sent to bootloader to load kernel for block:" << i;

        delay(5);

        float pleft = (float)blockno / (float)maxblocks * 100;
        set_progressbar_value(pleft);

    }

    set_progressbar_value(100);

    //qDebug() << "All 0xA8 messages sent, checksum:" << hex << chk_sum;

    // send 0xB0 command to check checksum
    output[5] = (uint8_t)(SID_KERNEL_CHECKSUM + 0x04);
    output[6] = (uint8_t)(((end_address + 1) >> 24) & 0xFF);
    output[7] = (uint8_t)(((end_address + 1) >> 16) & 0xFF);
    output[8] = (uint8_t)(((end_address + 1) >> 8) & 0xFF);
    output[9] = (uint8_t)((end_address + 1) & 0xFF);
    output[10] = (uint8_t)0x00;
    output[11] = (uint8_t)0x00;
    serial->write_serial_data_echo_check(output);
    //qDebug() << parse_message_to_hex(output);
    //qDebug() << "0xB0 message sent to bootloader to check checksum, waiting for response...";
    delay(200);
    received = serial->read_serial_data(20, 10);
    //qDebug() << "Response to 0xB0 message:" << parse_message_to_hex(received);

    send_log_window_message("Kernel uploaded, starting kernel...", true, true);

    // send 0xA0 command to jump into kernel
    output[5] = (uint8_t)(SID_KERNEL_JUMP + 0x04);
    output[6] = (uint8_t)((end_address >> 24) & 0xFF);
    output[7] = (uint8_t)((end_address >> 16) & 0xFF);
    output[8] = (uint8_t)((end_address >> 8) & 0xFF);
    output[9] = (uint8_t)(end_address & 0xFF);
    output[10] = (uint8_t)0x00;
    output[11] = (uint8_t)0x00;
    serial->write_serial_data_echo_check(output);
    //qDebug() << parse_message_to_hex(output);
    //qDebug() << "0xA0 message sent to bootloader to jump into kernel";

    //qDebug() << "ECU should now be running from kernel";

    received.clear();
    received = request_kernel_id();
    if (received == "")
        return STATUS_ERROR;

    //qDebug() << "Request kernel ID OK: " << parse_message_to_hex(received);
    send_log_window_message("Kernel ID: " + received, true, true);

    return STATUS_SUCCESS;
}

/*
 * Read memory from Subaru Denso CAN 32bit ECUs, nisprog kernel
 *
 * @return success
 */
int FlashDensoKline04::read_mem_subaru_denso_can_32bit(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t start_addr, uint32_t length)
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

/*
 * Write memory to Subaru Denso CAN 32bit ECUs, nisprog kernel
 *
 * @return success
 */
int FlashDensoKline04::write_mem_subaru_denso_can_32bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{
    QByteArray filedata;

    filedata = ecuCalDef->FullRomData;

    uint8_t data_array[filedata.length()];

    int block_modified[16] = {0};

    unsigned bcnt = 0;
    unsigned blockno;

    for (int i = 0; i < filedata.length(); i++)
    {
        data_array[i] = filedata.at(i);
    }

    send_log_window_message("--- comparing ECU flash memory pages to image file ---", true, true);
    send_log_window_message("seg\tstart\tlen\tsame?", true, true);

    if (get_changed_blocks_can_32bit(data_array, block_modified))
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
                if (reflash_block_can_32bit(&data_array[flashdevices[mcu_type_index].fblocks->start], &flashdevices[mcu_type_index], blockno, test_write))
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

        if (get_changed_blocks_can_32bit(data_array, block_modified))
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

/*
 * Compare ROM 32bit CAN ECUs
 *
 * @return
 */
int FlashDensoKline04::get_changed_blocks_can_32bit(const uint8_t *src, int *modified)
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

/*
 * ROM CRC 32bit CAN ECUs
 *
 * @return
 */
int FlashDensoKline04::check_romcrc_32bit_can(const uint8_t *src, uint32_t start_addr, uint32_t len, int *modified)
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
}

/*
 * Reflash ROM 32bit CAN ECUs
 *
 * @return success
 */
int FlashDensoKline04::reflash_block_can_32bit(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool test_write)
{
    int errval;

    uint32_t block_start;
    uint32_t block_len;

    QByteArray output;
    QByteArray received;
    QByteArray msg;

    set_progressbar_value(0);

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

    errval = flash_block_can_32bit(newdata, block_start, block_len);
    if (errval) {
        send_log_window_message("Reflash error! Do not panic, do not reset the ECU immediately. The kernel is most likely still running and receiving commands!", true, true);
        return STATUS_ERROR;
    }

    set_progressbar_value(100);

    return STATUS_SUCCESS;
}

/*
 * Flash block 32bit CAN ECUs
 *
 * @return success
 */
int FlashDensoKline04::flash_block_can_32bit(const uint8_t *src, uint32_t start, uint32_t len)
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

    qDebug() << "flashbytesindex" << flashbytesindex;
    qDebug() << "flashbytescount" << flashbytescount;

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
        }

        output[4] = (uint8_t)SID_START_COMM_CAN;
        output[5] = (uint8_t)(SIDFL_WB_CAN + 0x03);
        output[6] = (uint8_t)((i >> 8) & 0xFF);
        output[7] = (uint8_t)(i & 0xFF);
        output[8] = (uint8_t)(chk_sum & 0xFF);
        received = serial->write_serial_data_echo_check(output);

        received = serial->read_serial_data(3, serial_read_long_timeout);
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

        float pleft = (float)byteindex / (float)flashbytescount * 100.0f;
        set_progressbar_value(pleft);

        QString start_address = QString("%1").arg(start,8,16,QLatin1Char('0'));
        msg = QString("writing chunk @ 0x%1 (%2\% - %3 B/s, ~ %4 s remaining)").arg(start_address).arg((unsigned) 100 * (len - remain) / len,1,10,QLatin1Char('0')).arg((uint32_t)curspeed,1,10,QLatin1Char('0')).arg(tleft,1,10,QLatin1Char('0')).toUtf8();
        send_log_window_message(msg, true, true);

    }

    return STATUS_SUCCESS;
}

/*
 * 8bit checksum
 *
 * @return
 */
uint8_t FlashDensoKline04::cks_add8(QByteArray chksum_data, unsigned len)
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
void FlashDensoKline04::init_crc16_tab(void)
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

uint16_t FlashDensoKline04::crc16(const uint8_t *data, uint32_t siz)
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
QByteArray FlashDensoKline04::send_subaru_sid_bf_ssm_init()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;


    output.append((uint8_t)0xBF);

    while (received == "" && loop_cnt < 5)
    {
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
QByteArray FlashDensoKline04::send_subaru_denso_sid_81_start_communication()
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
QByteArray FlashDensoKline04::send_subaru_denso_sid_83_request_timings()
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
QByteArray FlashDensoKline04::send_subaru_denso_sid_27_request_seed()
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
QByteArray FlashDensoKline04::send_subaru_denso_sid_27_send_seed_key(QByteArray seed_key)
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
QByteArray FlashDensoKline04::send_subaru_denso_sid_10_start_diagnostic()
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
QByteArray FlashDensoKline04::send_subaru_denso_sid_34_request_upload(uint32_t dataaddr, uint32_t datalen)
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
QByteArray FlashDensoKline04::send_subaru_denso_sid_36_transferdata(uint32_t dataaddr, QByteArray buf, uint32_t len)
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

QByteArray FlashDensoKline04::send_subaru_denso_sid_31_start_routine()
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
QByteArray FlashDensoKline04::subaru_denso_generate_kline_seed_key(QByteArray requested_seed)
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

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

/*
 * Generate denso kline ecutek seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashDensoKline04::subaru_denso_generate_ecutek_kline_seed_key(QByteArray requested_seed)
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

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

/*
 * Generate denso can seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashDensoKline04::subaru_denso_generate_can_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex_1[]={
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

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

/*
 * Generate denso can ecutek seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashDensoKline04::subaru_denso_generate_ecutek_can_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex_1[]={
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

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

/*
 * Calculate denso seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray FlashDensoKline04::subaru_denso_calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
QByteArray FlashDensoKline04::subaru_denso_transform_32bit_payload(QByteArray buf, uint32_t len)
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

    encrypted = subaru_denso_calculate_32bit_payload(buf, len, keytogenerateindex, indextransformation);

    return encrypted;
}

QByteArray FlashDensoKline04::subaru_denso_calculate_32bit_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
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
QByteArray FlashDensoKline04::request_kernel_init()
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
QByteArray FlashDensoKline04::request_kernel_id()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray kernelid;

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

    return kernelid;
}
























/*
 * Add SSM header to message
 *
 * @return parsed message
 */
QByteArray FlashDensoKline04::add_ssm_header(QByteArray output, uint8_t tester_id, uint8_t target_id, bool dec_0x100)
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
uint8_t FlashDensoKline04::calculate_checksum(QByteArray output, bool dec_0x100)
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
int FlashDensoKline04::connect_bootloader_start_countdown(int timeout)
{
    for (int i = timeout; i > 0; i--)
    {
        if (kill_process)
            break;
        send_log_window_message("Start in " + QString::number(i), true, true);
        //qDebug() << "Countdown:" << i;
        delay(1000);
    }
    if (!kill_process)
    {
        send_log_window_message("Turn ignition on NOW!", true, true);
        delay(500);
        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}

/*
 * Parse QByteArray to readable form
 *
 * @return parsed message
 */
QString FlashDensoKline04::parse_message_to_hex(QByteArray received)
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
int FlashDensoKline04::send_log_window_message(QString message, bool timestamp, bool linefeed)
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

void FlashDensoKline04::set_progressbar_value(int value)
{
    if (ui->progressbar)
        ui->progressbar->setValue(value);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void FlashDensoKline04::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
}

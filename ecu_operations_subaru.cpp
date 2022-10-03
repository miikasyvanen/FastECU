#include "ecu_operations_subaru.h"
#include <ui_ecuoperationswindow.h>

EcuOperationsSubaru::EcuOperationsSubaru(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
    : QWidget(parent),
      ui(new Ui::EcuOperationsWindow)
{
    ui->setupUi(this);
    this->setParent(parent);
    this->setAttribute(Qt::WA_DeleteOnClose);
    //this->setAttribute(Qt::WA_QuitOnClose);
    if (cmd_type == "test_write")
        this->setWindowTitle("Test write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "write")
        this->setWindowTitle("Write ROM " + ecuCalDef->FileName + " to ECU");
    else
        this->setWindowTitle("Read ROM from ECU");
    this->show();
    this->serial = serial;

    int result = 0;

    //qDebug() << "Start ECU operations window";

    ui->progressbar->setValue(0);
    //serial = new SerialPortActions();

    result = ecu_functions(ecuCalDef, cmd_type);
    if (result == STATUS_SUCCESS)
        QMessageBox::information(this, tr("ECU Operation"), "ECU operation was succesful, press OK to exit");
    else
        QMessageBox::warning(this, tr("ECU Operation"), "ECU operation failed, press OK to exit and try again");
}

EcuOperationsSubaru::~EcuOperationsSubaru()
{

    //free(ecuOperations);

    this->close();
}

void EcuOperationsSubaru::check_mcu_type(QString flash_method)
{
    if (flash_method == "wrx02")
        mcu_type_string = "MC68HC16Y5";
    if (flash_method == "fxt02" || flash_method == "fxt02can")
        mcu_type_string = "SH7055";
    if (flash_method == "sti04" || flash_method == "sti04can")
        mcu_type_string = "SH7055";
    if (flash_method == "sti05" || flash_method == "sti05can")
        mcu_type_string = "SH7058";
    if (flash_method == "subarucan")
        mcu_type_string = "SH7058";

    mcu_type_index = 0;

    while (flashdevices[mcu_type_index].name != 0)
    {
        //qDebug() << flashdevices[mcu_type_index].name;
        if (flashdevices[mcu_type_index].name == mcu_type_string)
            break;
        mcu_type_index++;
    }

/*
    qDebug() << "MCU type:" << flashdevices[mcu_type_index].name;
    qDebug() << "MCU ROM size:" << flashdevices[mcu_type_index].romsize;
    qDebug() << "MCU block count:" << flashdevices[mcu_type_index].numblocks;
    for (unsigned i = 0; i < flashdevices[mcu_type_index].numblocks; i++)
        qDebug() << "Block start:" << flashdevices[mcu_type_index].fblocks[i].start << "| length:" << flashdevices[mcu_type_index].fblocks[i].len;
*/
}

int EcuOperationsSubaru::ecu_functions(FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type)
{
    QByteArray romdata;
    QString flash_method;
    QString kernel;
    int test_write = 0;
    int result;

    if (cmd_type == "read")
    {
        QMessageBox::information(this, tr("Read ROM"), "Press OK to start read countdown");
        send_log_window_message("Read memory with flashmethod " + flash_method + " and kernel " + ecuCalDef->Kernel, true, true);
    }
    else if (cmd_type == "test_write")
    {
        test_write = 1;
        QMessageBox::information(this, tr("Test write ROM"), "Press OK to start test write countdown");
        send_log_window_message("Test write memory with flashmethod " + flash_method + " and kernel " + ecuCalDef->Kernel, true, true);
    }
    else if (cmd_type == "write")
    {
        test_write = 0;
        QMessageBox::information(this, tr("Write ROM"), "Press OK to start write countdown!");
        send_log_window_message("Write memory with flashmethod " + flash_method + " and kernel " + ecuCalDef->Kernel, true, true);
    }

    if (cmd_type == "test_write" || cmd_type == "write")
    {
        romdata = ecuCalDef->FullRomData;
        flash_method = ecuCalDef->RomInfo.at(FlashMethod);
        kernel = ecuCalDef->Kernel;
    }
    if (cmd_type == "read")
    {
        flash_method = ecuCalDef->RomInfo.at(FlashMethod);
        kernel = ecuCalDef->Kernel;
    }

    check_mcu_type(flash_method);

    ecuOperations = new EcuOperations(this, serial, mcu_type_string, mcu_type_index);
    kernel_alive = false;

    if (flash_method == "wrx02")
    {
        result = connect_bootloader_subaru_kline_02_16bit();
        if (result == STATUS_SUCCESS)
            result = upload_kernel_subaru_kline_02_16bit(kernel);
        if (result == STATUS_SUCCESS)
        {
            if (cmd_type == "read")
                result = read_rom_subaru_kline_02_16bit(ecuCalDef);
            else if (cmd_type == "test_write" || cmd_type == "write")
                result = write_rom_subaru_kline_02_16bit(ecuCalDef, test_write);
        }
        return result;
    }
    else if (flash_method == "wrx04")
    {
        result = connect_bootloader_subaru_kline_04_16bit();
        if (result == STATUS_SUCCESS)
            result = upload_kernel_subaru_kline_04_16bit(kernel);
        if (result == STATUS_SUCCESS)
        {
            if (cmd_type == "read")
                result = read_rom_subaru_kline_04_16bit(ecuCalDef);
            else if (cmd_type == "test_write" || cmd_type == "write")
                result = write_rom_subaru_kline_04_16bit(ecuCalDef, test_write);
        }
        return result;
    }
    else if (flash_method == "fxt02")
    {
        result = connect_bootloader_subaru_kline_02_32bit();
        if (result == STATUS_SUCCESS)
            result = upload_kernel_subaru_kline_02_32bit(kernel);
        if (result == STATUS_SUCCESS)
        {
            if (cmd_type == "read")
                result = read_rom_subaru_kline_02_32bit(ecuCalDef);
            else if (cmd_type == "test_write" || cmd_type == "write")
                result = write_rom_subaru_kline_02_32bit(ecuCalDef, test_write);
        }
        return result;
    }
    else if (flash_method == "sti04")
    {
        result = connect_bootloader_subaru_kline_04_32bit();
        if (result == STATUS_SUCCESS && !kernel_alive)
            result = upload_kernel_subaru_kline_04_32bit(kernel);
        if (result == STATUS_SUCCESS)
        {
            if (cmd_type == "read")
                result = read_rom_subaru_kline_04_32bit(ecuCalDef);
            else if (cmd_type == "test_write" || cmd_type == "write")
                result = write_rom_subaru_kline_04_32bit(ecuCalDef, test_write);
        }
        return result;
    }
    else if (flash_method == "sti05")
    {
        result = connect_bootloader_subaru_kline_05_32bit();
        if (result == STATUS_SUCCESS)
            result = upload_kernel_subaru_kline_05_32bit(kernel);
        if (result == STATUS_SUCCESS)
        {
            if (cmd_type == "read")
                result = read_rom_subaru_kline_05_32bit(ecuCalDef);
            else if (cmd_type == "test_write" || cmd_type == "write")
                result = write_rom_subaru_kline_05_32bit(ecuCalDef, test_write);
        }
        return result;
    }
    else if (flash_method == "subarucan")
    {
        result = connect_bootloader_subaru_can_05_32bit();
        if (result == STATUS_SUCCESS)
            result = upload_kernel_subaru_can_05_32bit(kernel);
        if (result == STATUS_SUCCESS)
        {
            if (cmd_type == "read")
                result = read_rom_subaru_can_05_32bit(ecuCalDef);
            else if (cmd_type == "test_write" || cmd_type == "write")
                result = write_rom_subaru_can_05_32bit(ecuCalDef, test_write);
        }
        return result;
    }
    else
    {
        send_log_window_message("Unknown flashmethod " + flash_method, true, true);
        return 0;
    }

    return 0;
}

int EcuOperationsSubaru::connect_bootloader_subaru_kline_02_16bit()
{
    QByteArray output;
    QByteArray received;

    send_log_window_message("Connecting to Subaru 02 16-bit bootloader, please wait...", true, true);

    // Change serial speed and set 'line end checks' to low level
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }
    serial->change_port_speed("9600");
    serial->set_lec_lines(0, 0);

    delay(10);
    // Start countdown
    connect_bootloader_start_countdown(5);

    // Connect to bootloader
    delay(1000);
    serial->pulse_lec_2_line(200);
    output.clear();
    for (uint8_t i = 0; i < subaru_02_16bit_bootloader_init.length(); i++)
    {
        output.append(subaru_02_16bit_bootloader_init[i]);
    }
    received = serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent to bootloader: " + parse_message_to_hex(received), true, true);
    received = serial->read_serial_data(output.length(), serial_read_short_timeout);
    send_log_window_message("Response from bootloader: " + parse_message_to_hex(received), true, true);

    if (received.length() != 3 || !check_received_message(subaru_02_16bit_bootloader_init_ok, received))
    {
        send_log_window_message("Bad response from bootloader", true, true);
        return STATUS_ERROR;
    }
    else
    {
        send_log_window_message("Connected to bootloader", true, true);
        return STATUS_SUCCESS;
    }

    delay(10);

    return STATUS_ERROR;
}

int EcuOperationsSubaru::connect_bootloader_subaru_kline_04_16bit()
{
    QByteArray output;
    QByteArray received;

    send_log_window_message("Connecting to Subaru 04 16-bit k-line bootloader, please wait...", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR! Serial port is not open!", true, true);
        return STATUS_ERROR;
    }

    return STATUS_ERROR;
}

int EcuOperationsSubaru::connect_bootloader_subaru_kline_02_32bit()
{
    QByteArray output;
    QByteArray received;

    send_log_window_message("Connecting to Subaru 02 32-bit k-line bootloader, please wait...", true, true);

    // Change serial speed and set 'line end checks' to low level
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }
    serial->change_port_speed("9600");
    serial->set_lec_lines(0, 0);

    delay(10);
    // Start countdown
    connect_bootloader_start_countdown(5);

    // Connect to bootloader
    delay(1000);
    serial->pulse_lec_2_line(200);
    output.clear();
    for (uint8_t i = 0; i < subaru_02_32bit_bootloader_init.length(); i++)
    {
        output.append(subaru_02_32bit_bootloader_init[i]);
    }
    received = serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent to bootloader: " + parse_message_to_hex(received), true, true);

    received = serial->read_serial_data(output.length(), serial_read_short_timeout);
    send_log_window_message("Response from bootloader: " + parse_message_to_hex(received), true, true);

    if (received.length() != 3 || !check_received_message(subaru_02_32bit_bootloader_init_ok, received))
    {
        send_log_window_message("Bad response from bootloader", true, true);
        return STATUS_ERROR;
    }
    else
    {
        send_log_window_message("Connected to bootloader", true, true);
        return STATUS_SUCCESS;
    }

    delay(10);

    return STATUS_ERROR;
}

int EcuOperationsSubaru::connect_bootloader_subaru_kline_04_32bit()
{
    QByteArray output;
    QByteArray received;
    QByteArray seed;
    QByteArray seed_key;

    QString msg;

    send_log_window_message("Connecting to Subaru 04 32-bit k-line bootloader, please wait...", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    connect_bootloader_start_countdown(2);

    serial->change_port_speed("62500");
    serial->serialport_protocol_14230 = true;

    delay(100);

    received = ecuOperations->request_kernel_init();
    if (received.length() > 0)
    {
        if ((uint8_t)received.at(1) == SID_KERNEL_INIT + 0x40)
        {
            send_log_window_message("Kernel init ok", true, true);
            send_log_window_message("Requesting kernel ID", true, true);

            delay(100);

            received = ecuOperations->request_kernel_id();
            if (received == "")
                return STATUS_ERROR;

            received.remove(0, 2);
            send_log_window_message("Request kernel id ok" + received, true, true);
            kernel_alive = true;
            return STATUS_SUCCESS;
        }
    }

    serial->serialport_protocol_14230 = false;

    if (serial->use_openport2_adapter)
        serial->reset_connection();
    serial->change_port_speed("4800");
    if (serial->use_openport2_adapter)
        serial->init_j2534_connection();

    delay(100);
/*
    received = sub_sid_a8_read_mem();
    if (received == "" || (uint8_t)received.at(4) != 0xE8)
        return STATUS_ERROR;
    received.remove(0,5);
    received.remove(received.length() - 1, 1);
    send_log_window_message("ECU ID = " + parse_message_to_hex(received), true, true);
*/

    // SSM init
    received = sub_sid_bf_ssm_init();
    //send_log_window_message("SID BF = " + parse_message_to_hex(received), true, true);
    if (received == "")
        return STATUS_ERROR;
    received.remove(0, 8);
    received.remove(5, received.length() - 5);
    QString ecuid = QString("%1%2%3%4%5").arg(received.at(0),2,16,QLatin1Char('0')).toUpper().arg(received.at(1),2,16,QLatin1Char('0')).arg(received.at(2),2,16,QLatin1Char('0')).arg(received.at(3),2,16,QLatin1Char('0')).arg(received.at(4),2,16,QLatin1Char('0'));

    send_log_window_message("ECU ID = " + ecuid, true, true);

    // Start communication
    received = sub_sid_81_start_communication();
    //send_log_window_message("SID_81 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0xC1)
        return STATUS_ERROR;

    send_log_window_message("Start communication ok", true, true);

    // Request timing parameters
    received = sub_sid_83_request_timings();
    //send_log_window_message("SID_83 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0xC3)
        return STATUS_ERROR;

    send_log_window_message("Request timing parameters ok", true, true);

    // Request seed
    received = sub_sid_27_request_seed();
    //send_log_window_message("SID_27_01 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0x67)
        return STATUS_ERROR;

    send_log_window_message("Seed request ok", true, true);

    seed.append(received.at(6));
    seed.append(received.at(7));
    seed.append(received.at(8));
    seed.append(received.at(9));

    seed_key = sub_generate_seed_key(seed);
    msg = parse_message_to_hex(seed_key);
    //qDebug() << "Calculated seed key: " + msg + ", sending to ECU";
    //send_log_window_message("Calculated seed key: " + msg + ", sending to ECU", true, true);
    send_log_window_message("Sending seed key", true, true);

    received = sub_sid_27_send_seed_key(seed_key);
    //send_log_window_message("SID_27_02 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0x67)
        return STATUS_ERROR;

    send_log_window_message("Seed key ok", true, true);

    // Start diagnostic session
    received = sub_sid_10_start_diagnostic();
    if (received == "" || (uint8_t)received.at(4) != 0x50)
        return STATUS_ERROR;

    //send_log_window_message("Starting diagnostic session", true, true);

    //delay(500);

    return STATUS_SUCCESS;
}

int EcuOperationsSubaru::connect_bootloader_subaru_kline_05_32bit()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    send_log_window_message("Connecting to Subaru 05 32-bit k-line bootloader, please wait...", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    return STATUS_ERROR;
}

int EcuOperationsSubaru::connect_bootloader_subaru_can_05_32bit()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    send_log_window_message("Connecting to Subaru 05 32-bit can bootloader, please wait...", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    return STATUS_ERROR;
}

int EcuOperationsSubaru::upload_kernel_subaru_kline_02_16bit(QString kernel)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray pl_encr;
    uint32_t file_len = 0;
    uint32_t pl_len = 0;
    uint32_t len = 0;
    uint8_t chk_sum = 0;

    send_log_window_message("Initializing Subaru K-Line 02 16-bit kernel upload, please wait...", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

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
    output.append((uint8_t)(flashdevices[mcu_type_index].rblocks->start >> 16) & 0xFF);
    output.append((uint8_t)(flashdevices[mcu_type_index].rblocks->start >> 8) & 0xFF);
    output.append((uint8_t)(len >> 16) & 0xFF);
    output.append((uint8_t)(len >> 8) & 0xFF);
    output.append((uint8_t)len & 0xFF);

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
        send_log_window_message("Message received " + QString::number(received.length()) + " bytes '" + msg + "'", true, true);
    else
        send_log_window_message("Kernel uploaded succesfully", true, true);

    delay(200);

    serial->change_port_speed("39473");

    return STATUS_ERROR;
}

int EcuOperationsSubaru::upload_kernel_subaru_kline_04_16bit(QString kernel)
{
    send_log_window_message("Initializing Subaru K-Line 04 16-bit kernel upload, please wait...", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    return STATUS_ERROR;
}

int EcuOperationsSubaru::upload_kernel_subaru_kline_02_32bit(QString kernel)
{
    send_log_window_message("Uploading Subaru K-Line 02 32-bit kernel upload, please wait...", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    return STATUS_ERROR;
}

int EcuOperationsSubaru::upload_kernel_subaru_kline_04_32bit(QString kernel)
{
    //qDebug() << "Kernel:" << kernel;
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

    start_address = flashdevices[mcu_type_index].rblocks->start + 4;

    send_log_window_message("Initializing Subaru K-Line 04 32-bit kernel upload, please wait...", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

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
        send_log_window_message("Using " + QString::number(file_len) + " byte payload, padding with garbage to " + QString::number(pl_len) + " (" + QString::number(file_len) + ") bytes.\n", true, true);
    }
    else
    {
        send_log_window_message("Using " + QString::number(file_len) + " (" + QString::number(file_len) + ") byte payload.", true, true);
    }
    if ((uint32_t)pl_encr.size() != file_len)
    {
        send_log_window_message("File size error (" + QString::number(file_len) + "/" + QString::number(pl_encr.size()) + ")", true, true);
        return STATUS_ERROR;
    }

    // Change port speed to upload kernel
    if (serial->change_port_speed("15625"))
        return STATUS_ERROR;

    // Request upload
    received = sub_sid_34_request_upload(start_address, len);
    if (received == "" || (uint8_t)received.at(4) != 0x74)
        return STATUS_ERROR;

    send_log_window_message("Kernel upload request ok, uploading now, please wait...", true, true);

    pl_encr = sub_encrypt_buf(pl_encr, (uint32_t) pl_len);

    received = sub_sid_36_transferdata(start_address, pl_encr, len);
    if (received == "" || (uint8_t)received.at(4) != 0x76)
        return STATUS_ERROR;

    send_log_window_message("Kernel uploaded", true, true);

    /* sid34 requestDownload - checksum bypass put just after payload */
    received = sub_sid_34_request_upload(start_address + len, 4);
    if (received == "" || (uint8_t)received.at(4) != 0x74)
        return STATUS_ERROR;

    //send_log_window_message("sid_34 checksum bypass ok", true, true);

    cks_bypass.append((uint8_t)0x00);
    cks_bypass.append((uint8_t)0x00);
    cks_bypass.append((uint8_t)0x5A);
    cks_bypass.append((uint8_t)0xA5);

    cks_bypass = sub_encrypt_buf(cks_bypass, (uint32_t) 4);

    /* sid36 transferData for checksum bypass */
    received = sub_sid_36_transferdata(start_address + len, cks_bypass, 4);
    if (received == "" || (uint8_t)received.at(4) != 0x76)
        return STATUS_ERROR;

    //send_log_window_message("sid_36 checksum bypass ok", true, true);

    /* SID 37 TransferExit does not exist on all Subaru ROMs */

    /* RAMjump ! */
    received = sub_sid_31_start_routine();
    if (received == "" || (uint8_t)received.at(4) != 0x71)
        return STATUS_ERROR;

    send_log_window_message("Kernel started, initializing...", true, true);

    serial->change_port_speed("62500");
    serial->serialport_protocol_14230 = true;

    delay(100);

    received = ecuOperations->request_kernel_init();
    if (received == "")
    {
        qDebug() << "Kernel init NOK! No response from kernel. " + parse_message_to_hex(received);
        send_log_window_message("Kernel init NOK! No response from kernel. " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }
    if ((uint8_t)received.at(1) != SID_KERNEL_INIT + 0x40)
    {
        qDebug() << "Kernel init NOK! Got bad startcomm response from kernel. " + parse_message_to_hex(received);
        send_log_window_message("Kernel init NOK! Got bad startcomm response from kernel. " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }
    else
    {
        send_log_window_message("Kernel init OK", true, true);
    }

    send_log_window_message("Requesting kernel ID", true, true);

    delay(100);

    received = ecuOperations->request_kernel_id();
    if (received == "")
        return STATUS_ERROR;

    received.remove(0, 2);
    send_log_window_message("Request kernel ID OK" + received, true, true);

    return STATUS_SUCCESS;
}

int EcuOperationsSubaru::upload_kernel_subaru_kline_05_32bit(QString kernel)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray payload;
    QByteArray received;
    QByteArray msg;
    QByteArray pl_encr;
    uint32_t file_len = 0;
    uint32_t pl_len = 0;
    uint32_t len = 0;
    QByteArray cks_bypass;
    uint8_t chk_sum = 0;

    send_log_window_message("Initializing Subaru K-Line 05 32-bit kernel upload, please wait...", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    return STATUS_ERROR;
}

int EcuOperationsSubaru::upload_kernel_subaru_can_05_32bit(QString kernel)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray payload;
    QByteArray received;
    QByteArray msg;
    QByteArray pl_encr;
    uint32_t file_len = 0;
    uint32_t pl_len = 0;
    uint32_t len = 0;
    QByteArray cks_bypass;
    uint8_t chk_sum = 0;

    send_log_window_message("Initializing Subaru CAN 05 32-bit kernel upload, please wait...", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    return STATUS_ERROR;
}


int EcuOperationsSubaru::read_rom_subaru_kline_02_16bit(FileActions::EcuCalDefStructure *ecuCalDef)
{
    send_log_window_message("Reading ROM from Subaru K-Line 02 16-bit", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    bool success = ecuOperations->read_mem_16bit(ecuCalDef, flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);

    return success;
}

int EcuOperationsSubaru::read_rom_subaru_kline_04_16bit(FileActions::EcuCalDefStructure *ecuCalDef)
{
    send_log_window_message("Reading ROM from Subaru K-Line 04 16-bit", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    return STATUS_ERROR;
}

int EcuOperationsSubaru::read_rom_subaru_kline_02_32bit(FileActions::EcuCalDefStructure *ecuCalDef)
{
    send_log_window_message("Reading ROM from Subaru K-Line 02 32-bit", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    bool success = ecuOperations->read_mem_32bit(ecuCalDef, flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);

    return success;
}

int EcuOperationsSubaru::read_rom_subaru_kline_04_32bit(FileActions::EcuCalDefStructure *ecuCalDef)
{
    QByteArray received;

    send_log_window_message("Reading ROM from Subaru K-Line 04 32-bit", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    //ecuOperations->read_mem_32bit(ecuCalDef, flashdevices[mcu_type_index].fblocks[0].start, 0x001000);
    bool success = ecuOperations->read_mem_32bit(ecuCalDef, flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);

    return success;
}

int EcuOperationsSubaru::read_rom_subaru_kline_05_32bit(FileActions::EcuCalDefStructure *ecuCalDef)
{
    send_log_window_message("Reading ROM from Subaru K-Line 05 32-bit", true, true);

    return STATUS_ERROR;
}

int EcuOperationsSubaru::read_rom_subaru_can_05_32bit(FileActions::EcuCalDefStructure *ecuCalDef)
{
    send_log_window_message("Reading ROM from Subaru CAN 05 32-bit", true, true);

    return STATUS_ERROR;
}

int EcuOperationsSubaru::write_rom_subaru_kline_02_16bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{
    send_log_window_message("Writing ROM to Subaru K-Line 02 16-bit", true, true);

    return STATUS_ERROR;
}

int EcuOperationsSubaru::write_rom_subaru_kline_04_16bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{
    send_log_window_message("Writing ROM to Subaru K-Line 04 16-bit", true, true);

    return STATUS_ERROR;
}

int EcuOperationsSubaru::write_rom_subaru_kline_02_32bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{
    send_log_window_message("Writing ROM to Subaru K-Line 02 32-bit", true, true);

    return STATUS_ERROR;
}

int EcuOperationsSubaru::write_rom_subaru_kline_04_32bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{
    send_log_window_message("Writing ROM to Subaru K-Line 04 32-bit", true, true);

    bool success = ecuOperations->write_mem_32bit(ecuCalDef, test_write);

    return success;
}

int EcuOperationsSubaru::write_rom_subaru_kline_05_32bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{
    send_log_window_message("Writing ROM to Subaru K-Line 05 32-bit", true, true);

    return STATUS_ERROR;
}


int EcuOperationsSubaru::write_rom_subaru_can_05_32bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{
    send_log_window_message("Writing ROM to Subaru CAN 05 32-bit", true, true);

    return STATUS_ERROR;
}

/*
 * Read ECU ID
 *
 * @return ECU ID and capabilities
 */
QByteArray EcuOperationsSubaru::sub_sid_a8_read_mem()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    //qDebug() << "SID 0xA8";

    send_log_window_message("Read ECU ID", true, true);
    //qDebug() << "Read ECU ID";
    output.append((uint8_t)0xA8);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x03);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x04);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x05);

    serial->write_serial_data_echo_check(add_ssm_header(output, false));
    received = serial->read_serial_data(100, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next A8 loop";
        //qDebug() << "A8 received:" << parse_message_to_hex(received);
        send_log_window_message("Read ECU ID", true, true);
        //qDebug() << "Read ECU ID";
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(100, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
        //qDebug() << "0xA8 loop_cnt:" << loop_cnt;

    //qDebug() << "A8 received:" << parse_message_to_hex(received);

    return received;
}

/*
 * ECU init
 *
 * @return ECU ID and capabilities
 */
QByteArray EcuOperationsSubaru::sub_sid_bf_ssm_init()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start BF";
    send_log_window_message("SSM init", true, true);
    //qDebug() << "SSM init";
    output.append((uint8_t)0xBF);
    serial->write_serial_data_echo_check(add_ssm_header(output, false));
    received = serial->read_serial_data(100, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next BF loop";
        //qDebug() << "BF received:" << parse_message_to_hex(received);
        send_log_window_message("SSM init", true, true);
        //qDebug() << "SSM init";
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(100, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0xBF loop_cnt:" << loop_cnt;

    //qDebug() << "BF received:" << parse_message_to_hex(received);

    return received;
}

/*
 * Start diagnostic connection
 *
 * @return received response
 */
QByteArray EcuOperationsSubaru::sub_sid_81_start_communication()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start 81";
    output.append((uint8_t)0x81);
    serial->write_serial_data_echo_check(add_ssm_header(output, false));
    received = serial->read_serial_data(8, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next 81 loop";
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(8, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0x81 loop_cnt:" << loop_cnt;

    //qDebug() << "81 received:" << parse_message_to_hex(received);

    return received;
}

/*
 * Start diagnostic connection
 *
 * @return received response
 */
QByteArray EcuOperationsSubaru::sub_sid_83_request_timings()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start 83";
    output.append((uint8_t)0x83);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(add_ssm_header(output, false));
    received = serial->read_serial_data(12, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next 83 loop";
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(12, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0x83 loop_cnt:" << loop_cnt;

    //qDebug() << "83 received:" << parse_message_to_hex(received);

    return received;
}

/*
 * Request seed
 *
 * @return seed (4 bytes)
 */
QByteArray EcuOperationsSubaru::sub_sid_27_request_seed()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start 27 01";
    output.append((uint8_t)0x27);
    output.append((uint8_t)0x01);
    received = serial->write_serial_data_echo_check(add_ssm_header(output, false));
    //qDebug() << "27 01 send received:" << parse_message_to_hex(received);
    received = serial->read_serial_data(11, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next 27 01 loop";
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(11, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0x27 0x01 loop_cnt:" << loop_cnt;

    //qDebug() << "27 01 received:" << parse_message_to_hex(received);

    return received;
}

/*
 * Send seed key
 *
 * @return received response
 */
QByteArray EcuOperationsSubaru::sub_sid_27_send_seed_key(QByteArray seed_key)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start 27 02";
    output.append((uint8_t)0x27);
    output.append((uint8_t)0x02);
    output.append(seed_key);
    serial->write_serial_data_echo_check(add_ssm_header(output, false));
    received = serial->read_serial_data(8, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next 27 02 loop";
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(8, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0x27 0x02 loop_cnt:" << loop_cnt;

    //qDebug() << "27 02 received:" << parse_message_to_hex(received);

    return received;
}

/*
 * Request start diagnostic
 *
 * @return received response
 */
QByteArray EcuOperationsSubaru::sub_sid_10_start_diagnostic()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start 10";
    output.append((uint8_t)0x10);
    output.append((uint8_t)0x85);
    output.append((uint8_t)0x02);
    serial->write_serial_data_echo_check(add_ssm_header(output, false));
    received = serial->read_serial_data(7, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next 10 loop";
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(7, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0x10 loop_cnt:" << loop_cnt;

    //qDebug() << "10 received:" << parse_message_to_hex(received);

    return received;
}

/*
 * Request download (kernel)
 *
 * @return received response
 */
QByteArray EcuOperationsSubaru::sub_sid_34_request_upload(uint32_t dataaddr, uint32_t datalen)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start 34";
    output.append((uint8_t)0x34);
    output.append((uint8_t)(dataaddr >> 16) & 0xFF);
    output.append((uint8_t)(dataaddr >> 8) & 0xFF);
    output.append((uint8_t)dataaddr & 0xFF);
    output.append((uint8_t)0x04);
    output.append((uint8_t)(datalen >> 16) & 0xFF);
    output.append((uint8_t)(datalen >> 8) & 0xFF);
    output.append((uint8_t)datalen & 0xFF);

    //qDebug() << "34 message:" << parse_message_to_hex(add_ssm_header(output, false));

    serial->write_serial_data_echo_check(add_ssm_header(output, false));
    received = serial->read_serial_data(7, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next 34 loop";
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(7, receive_timeout);
        loop_cnt++;
        //qDebug() << parse_message_to_hex(received);
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0x34 loop_cnt:" << loop_cnt;

    //qDebug() << "34 received:" << parse_message_to_hex(received);

    return received;
}

QByteArray EcuOperationsSubaru::sub_sid_36_transferdata(uint32_t dataaddr, QByteArray buf, uint32_t len)
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

    //qDebug() << "Kernel upload address:" << dataaddr << "(" << hex << dataaddr << ")";
    //qDebug() << "Kernel buffer length:" << buf.length() << "(" << hex << buf.length() << ")";
    //qDebug() << "Kernel file len:" << len << "(" << hex << len << ")";

    for (blockno = 0; blockno <= maxblocks; blockno++)
    {
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

        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        received = serial->read_serial_data(6, receive_timeout);
        while (received == "" && loop_cnt < comm_try_count)
        {
            //qDebug() << "Next 36 loop";
            serial->write_serial_data_echo_check(add_ssm_header(output, false));
            delay(comm_try_timeout);
            received = serial->read_serial_data(6, receive_timeout);
            loop_cnt++;
            //qDebug() << parse_message_to_hex(received);
        }
        //if (loop_cnt > 0)
        //    qDebug() << "0x36 loop_cnt:" << loop_cnt;
    }

    //qDebug() << "36 received:" << parse_message_to_hex(received);

    return received;

}

QByteArray EcuOperationsSubaru::sub_sid_31_start_routine()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start 31";
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x01);
    serial->write_serial_data_echo_check(add_ssm_header(output, false));
    received = serial->read_serial_data(8, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next 31 loop";
        serial->write_serial_data_echo_check(add_ssm_header(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(8, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0x31 loop_cnt:" << loop_cnt;

    //qDebug() << "31 received:" << parse_message_to_hex(received);

    return received;
}

QByteArray EcuOperationsSubaru::add_ssm_header(QByteArray output, bool dec_0x100)
{
    uint8_t length = output.length();

    output.insert(0, (uint8_t)0x80);
    output.insert(1, (uint8_t)0x10);
    output.insert(2, (uint8_t)0xF0);
    output.insert(3, length);
    output.append(calculate_checksum(output, dec_0x100));

    //qDebug() << "Generated SSM message:" << parseMessageToHex(output);

    return output;
}

QString EcuOperationsSubaru::parse_message_to_hex(QByteArray received)
{
    QByteArray msg;

    for (unsigned long i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

int EcuOperationsSubaru::check_received_message(QByteArray msg, QByteArray received)
{
    for (int i = 0; i < msg.length(); i++)
    {
        if (received.length() > i - 1)
            if (msg.at(i) != received.at(i))
                return 0;
    }

    return 1;
}

void EcuOperationsSubaru::connect_bootloader_start_countdown(int timeout)
{
    for (int i = timeout; i > 0; i--)
    {
        send_log_window_message("Start in " + QString::number(i), true, true);
        //qDebug() << "Countdown:" << i;
        delay(1000);
    }
    send_log_window_message("Turn ignition on NOW!", true, true);
    delay(500);
}

void EcuOperationsSubaru::send_log_window_message(QString message, bool timestamp, bool linefeed)
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
    }
}

void EcuOperationsSubaru::delay(int n)
{
    QTime dieTime = QTime::currentTime().addMSecs(n);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

/*
 * Generate seed key from received seed bytes
 *
 * @return seed key (4 bytes)
 */
QByteArray EcuOperationsSubaru::sub_generate_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    uint32_t seed, index;
    uint16_t wordtogenerateindex, wordtobeencrypted, encryptionkey;
    int ki, n;

    const uint16_t keytogenerateindex[]={
        0x53DA, 0x33BC, 0x72EB, 0x437D,
        0x7CA3, 0x3382, 0x834F, 0x3608,
        0xAFB8, 0x503D, 0xDBA3, 0x9D34,
        0x3563, 0x6B70, 0x6E74, 0x88F0
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

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

uint8_t EcuOperationsSubaru::calculate_checksum(QByteArray output, bool dec_0x100)
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

QByteArray EcuOperationsSubaru::sub_encrypt_buf(QByteArray buf, uint32_t len)
{
    QByteArray encrypted;

    if (!buf.length() || !len) {
        return NULL;
    }

    len &= ~3;
    for (uint32_t i = 0; i < len; i += 4) {
        uint8_t tempbuf[4];
        tempbuf[0] = buf.at(i);
        tempbuf[1] = buf.at(i + 1);
        tempbuf[2] = buf.at(i + 2);
        tempbuf[3] = buf.at(i + 3);
        //memcpy(tempbuf, buf, 4);
        encrypted.append(sub_encrypt(tempbuf));
        //buf += 4;
    }
    return encrypted;
}

/** For Subaru, encrypts data for upload
 * writes 4 bytes in buffer *encrypteddata
 */
QByteArray EcuOperationsSubaru::sub_encrypt(const uint8_t *datatoencrypt)
{
    QByteArray encrypted;

    uint32_t datatoencrypt32, index;
    uint16_t wordtogenerateindex, wordtobeencrypted, encryptionkey;
    int ki, n;

    const uint16_t keytogenerateindex[]={
        0x7856, 0xCE22, 0xF513, 0x6E86
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    datatoencrypt32 = datatoencrypt[0] << 24 |datatoencrypt[1] << 16 |datatoencrypt[2] << 8 | datatoencrypt[3];

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

    return encrypted;
}

